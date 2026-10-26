#pragma once

#include <media/NdkImage.h>

#include "gl/egl_procs.h"

namespace gazeora {

// Gives images back to their reader once the GPU is done with everything drawn from them so
// far, so the producer (decoder, camera) can write them again. On the render thread.
class ImageReleaser {
public:
    ImageReleaser(EGLDisplay display, const EglProcs& procs) : m_display(display), m_procs(procs) {}

    void Release(AImage* image) const;

private:
    EGLDisplay m_display;
    const EglProcs& m_procs;
};

}  // namespace gazeora
