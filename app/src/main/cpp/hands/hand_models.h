// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <android/asset_manager.h>

#include <memory>
#include <string>

namespace gazeora {

// Where the hand tracker's models are: the app's assets (hands/*.tflite, which the build
// takes from MediaPipe's hand landmarker), and a folder to keep what the GPU makes of them,
// so it's quicker the next time.
struct HandModelFiles {
    AAssetManager* assets = nullptr;
    std::string cacheDir;
};

// MediaPipe's two hand landmarker models (hand_detection.h), each with its own LiteRT
// interpreter, each on this phone's GPU or CPU, whichever runs it faster (timed as they load).
// Loading starts as the app does (Preload): on the CPU first, which is quick, then on the GPU
// where that's faster, which takes a while the first time (the GPU makes its programs then).
// A hand tracker borrows them (Take), changes over to faster ones once they're ready (Faster),
// and gives them back (Give). Used by one thread at a time; the GPU's is OpenCL, which any
// thread can use.
class HandModels {
public:
    ~HandModels();
    HandModels(const HandModels&) = delete;
    HandModels& operator=(const HandModels&) = delete;

    // Starts loading the models from `files` on a thread of its own; once.
    static void Preload(HandModelFiles files);
    // Whether they're loaded (or can't be).
    static bool Ready();
    // The models, once loaded; loading them now if Preload wasn't called. Null with `error`
    // where they can't be.
    static std::unique_ptr<HandModels> Take(const HandModelFiles& files, std::string& error);
    // Back, for the next to Take.
    static void Give(std::unique_ptr<HandModels> models);
    // `models` changed for faster ones, if they're ready; between pictures.
    static void Faster(std::unique_ptr<HandModels>& models);

    // Where each runs.
    const char* Where() const;
    // The palm detector: its input (hand_detection::kPalmSide squared, RGB), running it, and
    // its outputs.
    float* PalmInput();
    bool FindPalms();
    const float* Boxes() const;
    const float* Scores() const;
    // The landmark model, the same way.
    float* HandInput();
    bool FindHand();
    const float* Landmarks() const;
    const float* Shape() const;
    float Presence() const;
    float Handedness() const;

private:
    class Impl;
    explicit HandModels(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> m_impl;
};

}  // namespace gazeora
