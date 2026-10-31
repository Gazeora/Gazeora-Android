// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "gl/image_releaser.h"

namespace gazeora {

void ImageReleaser::Release(AImage* image) const {
    if (!image) {
        return;
    }
    EGLSyncKHR sync = m_procs.createSync
                          ? m_procs.createSync(m_display, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr)
                          : EGL_NO_SYNC_KHR;
    int fence = -1;
    if (sync != EGL_NO_SYNC_KHR) {
        glFlush();  // the fence is only made once the commands before it are submitted
        fence = m_procs.dupFence(m_display, sync);
        m_procs.destroySync(m_display, sync);
    }
    if (fence >= 0) {
        AImage_deleteAsync(image, fence);
    } else {
        glFinish();
        AImage_delete(image);
    }
}

}  // namespace gazeora
