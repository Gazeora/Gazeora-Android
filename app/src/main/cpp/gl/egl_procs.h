// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace gazeora {

// The EGL and GL ES extension functions the renderer uses, looked up once.
struct EglProcs {
    PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC getNativeClientBuffer = nullptr;
    PFNEGLCREATEIMAGEKHRPROC createImage = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC destroyImage = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture = nullptr;
    // Only where EGL_ANDROID_native_fence_sync is; null otherwise.
    PFNEGLCREATESYNCKHRPROC createSync = nullptr;
    PFNEGLDESTROYSYNCKHRPROC destroySync = nullptr;
    PFNEGLDUPNATIVEFENCEFDANDROIDPROC dupFence = nullptr;

    // False if hardware buffers can't be made into textures on `display`.
    bool Load(EGLDisplay display);
};

}  // namespace gazeora
