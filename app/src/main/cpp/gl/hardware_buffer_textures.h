// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <android/hardware_buffer.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gl/egl_procs.h"

namespace gazeora {

// The buffers an image reader cycles through, each made into an external texture once.
// Used on the thread whose context is current.
class HardwareBufferTextures {
public:
    struct Texture {
        AHardwareBuffer* buffer = nullptr;  // a reference of ours
        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        GLuint texture = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    // Keeps up to `capacity` textures, dropping the oldest; the reader cycles through fewer.
    HardwareBufferTextures(EGLDisplay display, const EglProcs& procs, size_t capacity);
    ~HardwareBufferTextures() { Clear(); }
    HardwareBufferTextures(const HardwareBufferTextures&) = delete;
    HardwareBufferTextures& operator=(const HardwareBufferTextures&) = delete;

    // The buffer's texture; null if it has none yet.
    const Texture* Find(AHardwareBuffer* buffer) const;
    // The buffer's texture, made if it's new; null if it can't be.
    const Texture* Make(AHardwareBuffer* buffer);
    // Before the context goes.
    void Clear();

private:
    void Delete(Texture& texture);

    EGLDisplay m_display;
    const EglProcs& m_procs;
    size_t m_capacity;
    std::vector<Texture> m_textures;  // oldest first
};

}  // namespace gazeora
