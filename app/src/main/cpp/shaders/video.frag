// Copyright (C) 2026 Nithin Aashik
// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-Gazeora-Commercial

// The PC's warp (frame_dump.cpp in Gazeora), per eye: each pixel measured from its lens center
// in units of half the eye's height, pushed outwards by the distortion polynomial, then looked
// up in the picture; red and blue a little smaller and larger against the lenses' color
// fringes. In a pair of views, each eye looks in its own half.
#extension GL_OES_EGL_image_external : require
precision highp float;
uniform samplerExternalOES picture;
uniform vec4 lens;    // k1, k2, picture scale, chroma correction
// Not "layout": Adreno drivers (Redmi Y3) reject it as a keyword, and VR then can't draw.
uniform vec4 eyeLayout;  // VR (0/1), the lens center's x in the eye, eye aspect, picture aspect
uniform vec4 crop;    // the picture in the buffer: left, top, width, height (0..1)
uniform float pairView;  // the view of a pair this eye shows (0 left, 1 right); -1 no pair
uniform vec2 bufferSize;  // in texels
// The viewer's hand pointing at the window (hands/hand_pointer.h): where in the picture (0..1),
// the ring's radius as a share of its height, and how near a pinch (0..1; 2 pinched; below 0
// no cursor).
uniform vec4 cursor;
uniform float pictureAspect;
varying vec2 uv;

// `color` at `at` in the picture, with the cursor over it: a light ring with a dark edge, so it
// shows on light and dark alike, closing in as a pinch nears, and filled once it pinches.
vec3 WithCursor(vec3 color, vec2 at) {
    if (cursor.w < 0.0) return color;
    float r = length((at - cursor.xy) * vec2(pictureAspect, 1.0)) / cursor.z;
    float pinched = step(1.5, cursor.w);
    float ring = 1.0 - 0.4 * min(cursor.w, 1.0);
    float edge = 1.0 - smoothstep(0.12, 0.3, abs(r - ring));
    float line = 1.0 - smoothstep(0.06, 0.16, abs(r - ring));
    float fill = pinched * (1.0 - smoothstep(ring - 0.1, ring, r));
    color = mix(color, vec3(0.0), 0.5 * max(edge, fill));
    return mix(color, vec3(1.0), max(line, fill * 0.9));
}

// Catmull-Rom bicubic in 9 bilinear taps (the middle two of each row and column merged into
// one). Sharper than bilinear when the picture is scaled to the screen, and exact at 1:1: a
// screen pixel on a texel's center takes that texel. Taps stay inside the picture, as the
// buffer may have padding beyond it.
vec3 Bicubic(vec2 at) {
    vec2 position = at * bufferSize;
    vec2 center1 = floor(position - 0.5) + 0.5;
    vec2 f = position - center1;
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12 = w1 + w2;
    vec2 low = crop.xy + 0.5 / bufferSize, high = crop.xy + crop.zw - 0.5 / bufferSize;
    vec2 p0 = clamp((center1 - 1.0) / bufferSize, low, high);
    vec2 p12 = clamp((center1 + w2 / w12) / bufferSize, low, high);
    vec2 p3 = clamp((center1 + 2.0) / bufferSize, low, high);
    vec3 color = (texture2D(picture, vec2(p0.x, p0.y)).rgb * w0.x +
                  texture2D(picture, vec2(p12.x, p0.y)).rgb * w12.x +
                  texture2D(picture, vec2(p3.x, p0.y)).rgb * w3.x) * w0.y;
    color += (texture2D(picture, vec2(p0.x, p12.y)).rgb * w0.x +
              texture2D(picture, vec2(p12.x, p12.y)).rgb * w12.x +
              texture2D(picture, vec2(p3.x, p12.y)).rgb * w3.x) * w12.y;
    color += (texture2D(picture, vec2(p0.x, p3.y)).rgb * w0.x +
              texture2D(picture, vec2(p12.x, p3.y)).rgb * w12.x +
              texture2D(picture, vec2(p3.x, p3.y)).rgb * w3.x) * w3.y;
    return clamp(color, 0.0, 1.0);
}

vec2 InBuffer(vec2 q) {
    if (pairView >= 0.0) q.x = (q.x + pairView) * 0.5;
    return crop.xy + q * crop.zw;
}
vec2 ThroughLens(vec2 q, float channelScale) {
    return 0.5 + q * channelScale / (2.0 * lens.z) * vec2(1.0 / eyeLayout.w, 1.0);
}
float Channel(vec2 q, int channel) {
    if (q.x < 0.0 || q.y < 0.0 || q.x > 1.0 || q.y > 1.0) return 0.0;
    vec3 color = texture2D(picture, InBuffer(q)).rgb;
    return channel == 0 ? color.r : channel == 1 ? color.g : color.b;
}
void main() {
    if (eyeLayout.x == 0.0) {
        gl_FragColor = vec4(WithCursor(Bicubic(InBuffer(uv)), uv), 1.0);
        return;
    }
    vec2 p = (uv - vec2(eyeLayout.y, 0.5)) * vec2(eyeLayout.z, 1.0) * 2.0;
    float r2 = dot(p, p);
    vec2 q = p * (1.0 + lens.x * r2 + lens.y * r2 * r2);
    vec2 at = ThroughLens(q, 1.0);
    vec3 color = vec3(Channel(ThroughLens(q, 1.0 - lens.w), 0), Channel(at, 1),
                      Channel(ThroughLens(q, 1.0 + lens.w), 2));
    if (at.x >= 0.0 && at.y >= 0.0 && at.x <= 1.0 && at.y <= 1.0) color = WithCursor(color, at);
    gl_FragColor = vec4(color, 1.0);
}
