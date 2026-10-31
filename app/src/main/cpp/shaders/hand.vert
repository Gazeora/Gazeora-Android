// The corners of the viewer's rigged hands (hands/rigged_hand.h), per eye: from the camera's
// space to where the eye's lens shows them on the screen, by the inverse of video.frag's lens
// warp (solved for the radius by Newton's method), with their distance for the depth test.
// For the outline they're pushed out along their normals, as far for their distance as the
// outline is wide: wider on a pinching finger.
attribute vec3 position;  // meters: x right, y down, z ahead
attribute vec3 normal;
attribute float shows;    // 1 on the hand, fading to 0 toward the wrist
attribute float glow;     // 0, rising to 1 as its finger pinches
uniform vec4 lens;        // k1, k2, unused, unused
uniform vec4 eyeLayout;   // unused, the lens center's x in the eye, eye aspect, tan per unit
uniform float outline;    // the outline's width, as a tangent; 0 for the hand itself
varying vec3 surface;     // the corner's normal
varying vec3 toEye;       // from the corner to the eye
varying float fade;       // how much of it shows
varying float glowing;

// The nearest and farthest the depth test tells apart, in meters.
const float kNear = 0.05;
const float kFar = 2.5;

// How far in, in the lens's units, the lens shows what's `bent` units out.
float Unbend(float bent) {
    float r = bent;
    for (int i = 0; i < 5; ++i) {
        float r2 = r * r;
        float f = r * (1.0 + lens.x * r2 + lens.y * r2 * r2) - bent;
        float slope = 1.0 + 3.0 * lens.x * r2 + 5.0 * lens.y * r2 * r2;
        r = clamp(r - f / max(slope, 0.1), 0.0, 3.0);
    }
    return r;
}

void main() {
    vec3 at = position + normal * (outline * position.z * (1.0 + glow));
    float z = max(at.z, kNear);
    vec2 t = at.xy / z / eyeLayout.w;
    float bent = length(t);
    vec2 p = bent > 0.0 ? t * (Unbend(bent) / bent) : t;
    vec2 uv = p / (vec2(eyeLayout.z, 1.0) * 2.0) + vec2(eyeLayout.y, 0.5);  // 0, 0 top left
    float depth = clamp((z - kNear) / (kFar - kNear), 0.0, 1.0) * 2.0 - 1.0;
    gl_Position = vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    surface = normal;
    toEye = -at;
    fade = shows;
    glowing = glow;
}
