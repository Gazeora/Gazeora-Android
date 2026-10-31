// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

#include "gl/hardware_buffer_textures.h"

namespace gazeora {

HardwareBufferTextures::HardwareBufferTextures(EGLDisplay display, const EglProcs& procs,
                                               size_t capacity)
    : m_display(display), m_procs(procs), m_capacity(capacity) {}

const HardwareBufferTextures::Texture* HardwareBufferTextures::Find(AHardwareBuffer* buffer) const {
    for (const Texture& texture : m_textures) {
        if (texture.buffer == buffer) return &texture;
    }
    return nullptr;
}

const HardwareBufferTextures::Texture* HardwareBufferTextures::Make(AHardwareBuffer* buffer) {
    if (const Texture* known = Find(buffer)) {
        return known;
    }
    if (!buffer) {
        return nullptr;
    }
    EGLClientBuffer client = m_procs.getNativeClientBuffer(buffer);
    const EGLint attributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLImageKHR image = client ? m_procs.createImage(m_display, EGL_NO_CONTEXT,
                                                     EGL_NATIVE_BUFFER_ANDROID, client, attributes)
                               : EGL_NO_IMAGE_KHR;
    if (image == EGL_NO_IMAGE_KHR) {
        return nullptr;
    }
    if (m_textures.size() >= m_capacity) {
        Delete(m_textures.front());
        m_textures.erase(m_textures.begin());
    }
    Texture texture;
    AHardwareBuffer_acquire(buffer);  // so a buffer seen before is always the same one
    texture.buffer = buffer;
    texture.image = image;
    AHardwareBuffer_Desc desc = {};
    AHardwareBuffer_describe(buffer, &desc);
    texture.width = desc.width;
    texture.height = desc.height;
    glGenTextures(1, &texture.texture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture.texture);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_procs.imageTargetTexture(GL_TEXTURE_EXTERNAL_OES, static_cast<GLeglImageOES>(image));
    m_textures.push_back(texture);
    return &m_textures.back();
}

void HardwareBufferTextures::Clear() {
    for (Texture& texture : m_textures) {
        Delete(texture);
    }
    m_textures.clear();
}

void HardwareBufferTextures::Delete(Texture& texture) {
    glDeleteTextures(1, &texture.texture);
    m_procs.destroyImage(m_display, texture.image);
    AHardwareBuffer_release(texture.buffer);
}

}  // namespace gazeora
