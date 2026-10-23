// The Java side's way in: NativePlayer.java, NativeDiscovery.java and NativeUsbLink.java.

#include <android/native_window_jni.h>
#include <jni.h>

#include <memory>
#include <string>
#include <vector>

#include "discovery.h"
#include "player.h"
#include "sources/usb_link.h"

namespace gazeora {

namespace {

JavaVM* g_vm = nullptr;

// The JNIEnv of this thread, attached for as long as the thread runs if it's one of ours.
JNIEnv* Env() {
    JNIEnv* env = nullptr;
    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        return env;
    }
    struct Detacher {
        ~Detacher() { g_vm->DetachCurrentThread(); }
    };
    g_vm->AttachCurrentThread(&env, nullptr);
    thread_local Detacher detacher;
    return env;
}

std::string String(JNIEnv* env, jstring text) {
    if (!text) {
        return {};
    }
    const char* chars = env->GetStringUTFChars(text, nullptr);
    std::string copy(chars);
    env->ReleaseStringUTFChars(text, chars);
    return copy;
}

// Java's modified UTF-8 takes plain UTF-8 for everything but characters beyond the BMP and
// NUL, which a PC name or error message won't have.
jstring JavaString(JNIEnv* env, const std::string& text) { return env->NewStringUTF(text.c_str()); }

void ClearException(JNIEnv* env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

// Calls NativePlayer's onNative* methods, from the player's threads.
class JavaPlayerListener : public Player::Listener {
public:
    JavaPlayerListener(JNIEnv* env, jobject player) : m_player(env->NewGlobalRef(player)) {
        jclass type = env->GetObjectClass(player);
        m_onStatus = env->GetMethodID(type, "onNativeStatus", "(ILjava/lang/String;)V");
        m_onVideoSize = env->GetMethodID(type, "onNativeVideoSize", "(II)V");
        m_onFirstFrame = env->GetMethodID(type, "onNativeFirstFrame", "()V");
        m_onPicture = env->GetMethodID(type, "onNativePicture", "(ZII)V");
        env->DeleteLocalRef(type);
    }
    ~JavaPlayerListener() override { Env()->DeleteGlobalRef(m_player); }

    void OnStatus(PlayerStatus status, const std::string& detail) override {
        JNIEnv* env = Env();
        jstring text = detail.empty() ? nullptr : JavaString(env, detail);
        env->CallVoidMethod(m_player, m_onStatus, jint(status), text);
        ClearException(env);
        if (text) {
            env->DeleteLocalRef(text);
        }
    }

    void OnVideoSize(int width, int height) override {
        JNIEnv* env = Env();
        env->CallVoidMethod(m_player, m_onVideoSize, jint(width), jint(height));
        ClearException(env);
    }

    void OnFirstFrame() override {
        JNIEnv* env = Env();
        env->CallVoidMethod(m_player, m_onFirstFrame);
        ClearException(env);
    }

    void OnPicture(bool vr, int width, int height) override {
        JNIEnv* env = Env();
        env->CallVoidMethod(m_player, m_onPicture, jboolean(vr), jint(width), jint(height));
        ClearException(env);
    }

private:
    jobject m_player;
    jmethodID m_onStatus;
    jmethodID m_onVideoSize;
    jmethodID m_onFirstFrame;
    jmethodID m_onPicture;
};

// NativePlayer's pointer ids are the wire's (protocols/pointer_message.h).
bool ToPointerEvent(jint action, jint button, PointerEvent& event) {
    if (action < jint(PointerAction::Move) || action > jint(PointerAction::Click) ||
        button < jint(PointerButton::Left) || button > jint(PointerButton::Right)) {
        return false;
    }
    event.action = PointerAction(action);
    event.button = PointerButton(button);
    return true;
}

struct PlayerHandle {
    std::unique_ptr<JavaPlayerListener> listener;
    std::unique_ptr<Player> player;  // goes first: its threads call the listener
};

struct DiscoveryHandle {
    jobject discovery = nullptr;
    std::unique_ptr<Discovery> search;
};

}  // namespace

}  // namespace gazeora

using namespace gazeora;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    g_vm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT jlong JNICALL Java_com_gazeora_gazeora_feature_player_data_remote_NativePlayer_nativeStart(
    JNIEnv* env, jobject self, jobject surface, jstring protocol, jstring host, jint port,
    jstring path, jstring avcDecoder, jstring hevcDecoder, jboolean avcLowLatency,
    jboolean hevcLowLatency) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        return 0;
    }
    DecoderChoice decoders;
    decoders.avc = String(env, avcDecoder);
    decoders.hevc = String(env, hevcDecoder);
    decoders.avcLowLatency = avcLowLatency;
    decoders.hevcLowLatency = hevcLowLatency;

    auto* handle = new PlayerHandle();
    handle->listener = std::make_unique<JavaPlayerListener>(env, self);
    handle->player = std::make_unique<Player>(window, std::move(decoders), *handle->listener);
    if (!handle->player->Start(String(env, protocol), String(env, host), uint16_t(port),
                               String(env, path))) {
        handle->player.reset();
        delete handle;
        return 0;
    }
    return reinterpret_cast<jlong>(handle);
}

JNIEXPORT void JNICALL Java_com_gazeora_gazeora_feature_player_data_remote_NativePlayer_nativeRelease(
    JNIEnv*, jclass, jlong pointer) {
    auto* handle = reinterpret_cast<PlayerHandle*>(pointer);
    if (handle) {
        handle->player.reset();
        delete handle;
    }
}

JNIEXPORT void JNICALL Java_com_gazeora_gazeora_feature_player_data_remote_NativePlayer_nativeSendPointer(
    JNIEnv*, jclass, jlong pointer, jint action, jint button, jfloat x, jfloat y,
    jint wheelVertical, jint wheelHorizontal) {
    auto* handle = reinterpret_cast<PlayerHandle*>(pointer);
    PointerEvent event;
    if (!handle || !ToPointerEvent(action, button, event)) {
        return;
    }
    event.x = x;
    event.y = y;
    event.wheelVertical = wheelVertical;
    event.wheelHorizontal = wheelHorizontal;
    handle->player->SendPointer(event);
}

JNIEXPORT jboolean JNICALL Java_com_gazeora_gazeora_core_usb_data_NativeUsbLink_nativeIsOpen(
    JNIEnv*, jclass) {
    return UsbLink::Get().Open();
}

JNIEXPORT void JNICALL Java_com_gazeora_gazeora_core_usb_data_NativeUsbLink_nativeAttach(
    JNIEnv*, jclass, jint fd) {
    UsbLink::Get().Attach(fd);
}

JNIEXPORT jlong JNICALL Java_com_gazeora_gazeora_core_discovery_data_remote_NativeDiscovery_nativeStart(
    JNIEnv* env, jobject self, jboolean emulator) {
    auto* handle = new DiscoveryHandle();
    handle->discovery = env->NewGlobalRef(self);
    jclass type = env->GetObjectClass(self);
    jmethodID onPcs = env->GetMethodID(type, "onNativePcs", "([Ljava/lang/String;)V");
    env->DeleteLocalRef(type);
    jobject discovery = handle->discovery;
    // Six strings a PC: name, host, protocol, port, path, codec.
    handle->search = std::make_unique<Discovery>(
        emulator, [discovery, onPcs](const std::vector<Discovery::Pc>& pcs) {
            JNIEnv* env = Env();
            jclass stringType = env->FindClass("java/lang/String");
            jobjectArray fields = env->NewObjectArray(jsize(pcs.size() * 6), stringType, nullptr);
            jsize at = 0;
            for (const Discovery::Pc& pc : pcs) {
                for (const std::string& field :
                     {pc.reply.name, pc.host, pc.reply.protocol, std::to_string(pc.reply.port),
                      pc.reply.path, pc.reply.codec}) {
                    jstring text = JavaString(env, field);
                    env->SetObjectArrayElement(fields, at++, text);
                    env->DeleteLocalRef(text);
                }
            }
            env->CallVoidMethod(discovery, onPcs, fields);
            ClearException(env);
            env->DeleteLocalRef(fields);
            env->DeleteLocalRef(stringType);
        });
    handle->search->Start();
    return reinterpret_cast<jlong>(handle);
}

JNIEXPORT void JNICALL Java_com_gazeora_gazeora_core_discovery_data_remote_NativeDiscovery_nativeStop(
    JNIEnv* env, jclass, jlong pointer) {
    auto* handle = reinterpret_cast<DiscoveryHandle*>(pointer);
    if (handle) {
        handle->search.reset();
        env->DeleteGlobalRef(handle->discovery);
        delete handle;
    }
}

}  // extern "C"
