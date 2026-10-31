// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "hands/hand_models.h"

#include <android/log.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

#include "hands/hand_detection.h"
#include "tflite/c/c_api.h"
#include "tflite/delegates/gpu/delegate.h"

namespace gazeora {

namespace {

constexpr char kTag[] = "GazeoraHands";
constexpr char kPalmModel[] = "hands/hand_detector.tflite";
constexpr char kHandModel[] = "hands/hand_landmarks_detector.tflite";
// On the CPU, where that's faster or the GPU can't run them.
constexpr int kCpuThreads = 4;
// Each model is timed this many runs on the GPU and the CPU, after this many to warm up.
constexpr int kTimedRuns = 5;
constexpr int kWarmUpRuns = 2;

double Seconds(std::chrono::steady_clock::time_point from) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - from).count();
}

}  // namespace

// The two models, each with its own interpreter, each where it runs fastest on this phone:
// its GPU is not always faster than its CPU.
class HandModels::Impl {
public:
    // On the CPU only, which is quick to load; or where each runs fastest, timed, which takes
    // as long as the GPU needs to make its programs.
    static std::unique_ptr<Impl> Load(const HandModelFiles& files, bool gpu, std::string& error) {
        std::unique_ptr<Impl> models(new Impl());
        const char *palmWhere = "", *handWhere = "";
        models->m_palm = Fastest(files, kPalmModel, gpu, palmWhere, error);
        models->m_hand = Fastest(files, kHandModel, gpu, handWhere, error);
        models->m_onGpu = std::strcmp(palmWhere, "GPU") == 0 || std::strcmp(handWhere, "GPU") == 0;
        if (!models->m_palm || !models->m_hand || !models->Outputs(error)) {
            return nullptr;
        }
        models->m_where = std::string("finding hands on the ") + palmWhere +
                          ", following them on the " + handWhere;
        return models;
    }

    const char* Where() const { return m_where.c_str(); }
    bool OnGpu() const { return m_onGpu; }
    float* PalmInput() { return m_palm->Input(); }
    float* HandInput() { return m_hand->Input(); }
    bool FindPalms() { return m_palm->Run(); }
    bool FindHand() { return m_hand->Run(); }
    const float* Boxes() const { return m_boxes; }
    const float* Scores() const { return m_scores; }
    const float* Landmarks() const { return m_landmarks; }
    const float* Shape() const { return m_shape; }
    float Presence() const { return *m_presence; }
    float Handedness() const { return *m_handedness; }

private:
    class Net {
    public:
        ~Net() { Unload(); }

        bool Load(const HandModelFiles& files, const char* name, bool gpu, std::string& error) {
            m_asset = AAssetManager_open(files.assets, name, AASSET_MODE_BUFFER);
            const void* bytes = m_asset ? AAsset_getBuffer(m_asset) : nullptr;
            if (!bytes) {
                error = std::string("no ") + name;
                return false;
            }
            const size_t size = size_t(AAsset_getLength(m_asset));
            m_model = TfLiteModelCreate(bytes, size);
            m_options = TfLiteInterpreterOptionsCreate();
            if (!m_model || !m_options) {
                error = std::string("can't read ") + name;
                return false;
            }
            if (gpu) {
                TfLiteGpuDelegateOptionsV2 options = TfLiteGpuDelegateOptionsV2Default();
                options.is_precision_loss_allowed = 1;
                options.inference_preference = TFLITE_GPU_INFERENCE_PREFERENCE_SUSTAINED_SPEED;
                options.inference_priority1 = TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY;
                // OpenCL, which any thread can use: they're loaded on one and run on another.
                options.experimental_flags |= TFLITE_GPU_EXPERIMENTAL_FLAGS_CL_ONLY;
                // What the GPU makes of the model, kept for next time: named for the model.
                m_token = std::string(std::strrchr(name, '/') + 1) + "-" + std::to_string(size);
                if (!files.cacheDir.empty()) {
                    options.experimental_flags |= TFLITE_GPU_EXPERIMENTAL_FLAGS_ENABLE_SERIALIZATION;
                    options.serialization_dir = files.cacheDir.c_str();
                    options.model_token = m_token.c_str();
                }
                m_gpu = TfLiteGpuDelegateV2Create(&options);
                if (!m_gpu) {
                    error = "no GPU";
                    return false;
                }
                TfLiteInterpreterOptionsAddDelegate(m_options, m_gpu);
            } else {
                TfLiteInterpreterOptionsSetNumThreads(m_options, kCpuThreads);
            }
            m_interpreter = TfLiteInterpreterCreate(m_model, m_options);
            if (!m_interpreter || TfLiteInterpreterAllocateTensors(m_interpreter) != kTfLiteOk) {
                error = std::string("can't run ") + name + (gpu ? " on the GPU" : "");
                return false;
            }
            return true;
        }

        void Unload() {
            if (m_interpreter) TfLiteInterpreterDelete(m_interpreter);
            if (m_gpu) TfLiteGpuDelegateV2Delete(m_gpu);
            if (m_options) TfLiteInterpreterOptionsDelete(m_options);
            if (m_model) TfLiteModelDelete(m_model);
            if (m_asset) AAsset_close(m_asset);
            m_interpreter = nullptr;
            m_gpu = nullptr;
            m_options = nullptr;
            m_model = nullptr;
            m_asset = nullptr;
        }

        float* Input() {
            return static_cast<float*>(
                TfLiteTensorData(TfLiteInterpreterGetInputTensor(m_interpreter, 0)));
        }
        bool Run() { return TfLiteInterpreterInvoke(m_interpreter) == kTfLiteOk; }

        // How long a run takes, in seconds, on a gray picture; forever if it fails.
        double Time() {
            const TfLiteTensor* input = TfLiteInterpreterGetInputTensor(m_interpreter, 0);
            std::fill_n(Input(), TfLiteTensorByteSize(input) / sizeof(float), 0.5f);
            for (int i = 0; i < kWarmUpRuns; ++i) {
                if (!Run()) return 1e9;
            }
            const auto started = std::chrono::steady_clock::now();
            for (int i = 0; i < kTimedRuns; ++i) {
                if (!Run()) return 1e9;
            }
            return Seconds(started) / kTimedRuns;
        }

        // Its output called `name`, if it has `count` floats.
        const float* Output(const char* name, size_t count) const {
            for (int32_t i = 0; i < TfLiteInterpreterGetOutputTensorCount(m_interpreter); ++i) {
                const TfLiteTensor* tensor = TfLiteInterpreterGetOutputTensor(m_interpreter, i);
                if (std::strcmp(TfLiteTensorName(tensor), name) == 0 &&
                    TfLiteTensorType(tensor) == kTfLiteFloat32 &&
                    TfLiteTensorByteSize(tensor) == count * sizeof(float)) {
                    return static_cast<const float*>(TfLiteTensorData(tensor));
                }
            }
            return nullptr;
        }

    private:
        AAsset* m_asset = nullptr;
        TfLiteModel* m_model = nullptr;
        TfLiteInterpreterOptions* m_options = nullptr;
        TfLiteDelegate* m_gpu = nullptr;
        TfLiteInterpreter* m_interpreter = nullptr;
        std::string m_token;
    };

    // Model `name` on the GPU or the CPU, whichever runs it faster (or on the CPU, without
    // `gpu`); null if neither can.
    static std::unique_ptr<Net> Fastest(const HandModelFiles& files, const char* name,
                                        bool tryGpu, const char*& where, std::string& error) {
        std::unique_ptr<Net> fastest;
        double fastestSeconds = 0;
        for (bool gpu : {true, false}) {
            if (gpu && !tryGpu) continue;
            auto net = std::make_unique<Net>();
            if (!net->Load(files, name, gpu, error)) continue;
            if (!tryGpu) {
                where = "CPU";
                return net;
            }
            const double seconds = net->Time();
            __android_log_print(ANDROID_LOG_INFO, kTag, "%s: %.0f ms a run on the %s", name,
                                1000 * seconds, gpu ? "GPU" : "CPU");
            if (!fastest || seconds < fastestSeconds) {
                fastest = std::move(net);
                fastestSeconds = seconds;
                where = gpu ? "GPU" : "CPU";
            }
        }
        return fastest;
    }

    bool Outputs(std::string& error) {
        using namespace hand_detection;
        m_boxes = m_palm->Output("Identity", kAnchors * kBoxValues);
        m_scores = m_palm->Output("Identity_1", kAnchors);
        m_landmarks = m_hand->Output("Identity", kLandmarkValues);
        m_presence = m_hand->Output("Identity_1", 1);
        m_handedness = m_hand->Output("Identity_2", 1);
        m_shape = m_hand->Output("Identity_3", kLandmarkValues);
        if (!m_boxes || !m_scores || !m_landmarks || !m_presence || !m_handedness || !m_shape) {
            error = "the models aren't the ones expected";
            return false;
        }
        return true;
    }

    std::unique_ptr<Net> m_palm;
    std::unique_ptr<Net> m_hand;
    std::string m_where;
    bool m_onGpu = false;
    const float* m_boxes = nullptr;
    const float* m_scores = nullptr;
    const float* m_landmarks = nullptr;
    const float* m_presence = nullptr;
    const float* m_handedness = nullptr;
    const float* m_shape = nullptr;
};

HandModels::HandModels(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
HandModels::~HandModels() = default;

const char* HandModels::Where() const { return m_impl->Where(); }
float* HandModels::PalmInput() { return m_impl->PalmInput(); }
bool HandModels::FindPalms() { return m_impl->FindPalms(); }
const float* HandModels::Boxes() const { return m_impl->Boxes(); }
const float* HandModels::Scores() const { return m_impl->Scores(); }
float* HandModels::HandInput() { return m_impl->HandInput(); }
bool HandModels::FindHand() { return m_impl->FindHand(); }
const float* HandModels::Landmarks() const { return m_impl->Landmarks(); }
const float* HandModels::Shape() const { return m_impl->Shape(); }
float HandModels::Presence() const { return m_impl->Presence(); }
float HandModels::Handedness() const { return m_impl->Handedness(); }

namespace {

// The models between trackers: loading, loaded, or lent out.
struct Store {
    std::mutex mutex;
    std::condition_variable loaded;
    bool started = false;
    bool done = false;
    std::unique_ptr<HandModels> models;  // null while lent, or if they can't load
    std::unique_ptr<HandModels> faster;  // made while the others were lent
    std::string error;
};

Store& TheStore() {
    static Store* store = new Store();  // for the whole process: its thread may outlive main
    return *store;
}

}  // namespace

void HandModels::Preload(HandModelFiles files) {
    Store& store = TheStore();
    {
        std::lock_guard<std::mutex> lock(store.mutex);
        if (store.started) return;
        store.started = true;
    }
    // On the CPU first, which is quick, so hands are tracked at once; then, where the GPU runs
    // them faster, on the GPU, which the first time takes a while: trackers change over to
    // those as they're ready (Faster).
    std::thread([files = std::move(files)] {
        const auto started = std::chrono::steady_clock::now();
        Store& store = TheStore();
        std::string error;
        std::unique_ptr<Impl> cpu = Impl::Load(files, false, error);
        {
            std::lock_guard<std::mutex> lock(store.mutex);
            if (cpu) {
                store.models.reset(new HandModels(std::move(cpu)));
                __android_log_print(ANDROID_LOG_INFO, kTag, "hand models loaded in %.1f s: %s",
                                    Seconds(started), store.models->Where());
            } else {
                store.error = error;
            }
            store.done = true;
            store.loaded.notify_all();
        }
        std::unique_ptr<Impl> fastest = Impl::Load(files, true, error);
        if (!fastest || !fastest->OnGpu()) {
            if (!fastest) {
                __android_log_print(ANDROID_LOG_WARN, kTag, "hand models only on the CPU: %s",
                                    error.c_str());
            }
            return;
        }
        std::lock_guard<std::mutex> lock(store.mutex);
        __android_log_print(ANDROID_LOG_INFO, kTag, "faster hand models ready in %.1f s: %s",
                            Seconds(started), fastest->Where());
        store.faster.reset(new HandModels(std::move(fastest)));
        store.error.clear();
        if (store.models) {
            store.models = std::move(store.faster);  // not lent: the faster at once
        }
        store.loaded.notify_all();
    }).detach();
}

bool HandModels::Ready() {
    Store& store = TheStore();
    std::lock_guard<std::mutex> lock(store.mutex);
    return store.done;
}

std::unique_ptr<HandModels> HandModels::Take(const HandModelFiles& files, std::string& error) {
    Preload(files);
    Store& store = TheStore();
    std::unique_lock<std::mutex> lock(store.mutex);
    store.loaded.wait(lock, [&] { return store.done && (store.models || store.faster ||
                                                        !store.error.empty()); });
    if (store.faster) {
        return std::move(store.faster);
    }
    if (!store.models) {
        error = store.error.empty() ? "the hand models are in use" : store.error;
    }
    return std::move(store.models);
}

void HandModels::Give(std::unique_ptr<HandModels> models) {
    if (!models) return;
    Store& store = TheStore();
    std::lock_guard<std::mutex> lock(store.mutex);
    // The faster ones made meanwhile take their place.
    store.models = store.faster ? std::move(store.faster) : std::move(models);
}

void HandModels::Faster(std::unique_ptr<HandModels>& models) {
    Store& store = TheStore();
    std::lock_guard<std::mutex> lock(store.mutex);
    if (store.faster) {
        models = std::move(store.faster);
        __android_log_print(ANDROID_LOG_INFO, kTag, "hand tracking: %s", models->Where());
    }
}

}  // namespace gazeora
