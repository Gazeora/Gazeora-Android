// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "gl/egl_procs.h"

#include <cstring>

namespace gazeora {

namespace {

template <typename Fn>
Fn Proc(const char* name) {
    return reinterpret_cast<Fn>(eglGetProcAddress(name));
}

}  // namespace

bool EglProcs::Load(EGLDisplay display) {
    getNativeClientBuffer =
        Proc<PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC>("eglGetNativeClientBufferANDROID");
    createImage = Proc<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR");
    destroyImage = Proc<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR");
    imageTargetTexture = Proc<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>("glEGLImageTargetTexture2DOES");
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (extensions && strstr(extensions, "EGL_ANDROID_native_fence_sync")) {
        createSync = Proc<PFNEGLCREATESYNCKHRPROC>("eglCreateSyncKHR");
        destroySync = Proc<PFNEGLDESTROYSYNCKHRPROC>("eglDestroySyncKHR");
        dupFence = Proc<PFNEGLDUPNATIVEFENCEFDANDROIDPROC>("eglDupNativeFenceFDANDROID");
    }
    return getNativeClientBuffer && createImage && destroyImage && imageTargetTexture;
}

}  // namespace gazeora
