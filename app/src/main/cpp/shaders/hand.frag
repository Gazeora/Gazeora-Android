// The viewer's rigged hands over Gazeora's window in a VR viewer (hand_renderer.cpp), drawn the
// way VR headsets draw tracked hands: one cool, light color, a little see-through, brighter
// where the surface turns away, inside a translucent light outline, both fading out toward
// the wrist, whose turn the tracker doesn't see. The thumb and index finger turn blue as they
// near a pinch, their outline too, brightest once they pinch. The room's own light, as the
// back camera sees it (hands/light_estimate.h), shines on it a little, so it sits in the room.
precision mediump float;
uniform vec3 lightFrom;   // toward the room's main light, in the camera's space
uniform vec3 lightColor;  // its color times its strength
uniform float ambient;    // the light all round
uniform float outline;    // nonzero for the outline
varying vec3 surface;
varying vec3 toEye;
varying float fade;
varying float glowing;

const vec3 kSkin = vec3(0.70, 0.80, 0.94);
const vec3 kRim = vec3(0.85, 0.93, 1.0);
const vec3 kOutline = vec3(0.85, 0.93, 1.0);
const vec3 kGlow = vec3(0.30, 0.75, 1.0);
const float kFillOpacity = 0.85;
const float kOutlineOpacity = 0.7;
// How much brighter it is where it turns away, and how much of the rim's light shows there.
const float kFacing = 0.75;
const float kTurned = 0.35;
const float kRimShare = 0.45;
// How much the room's light shades it: its main light, and that light's shine. In a dim room
// it's darker, by this much of how far the light all round is below the brightest.
const float kLit = 0.12;
const float kShine = 0.3;
const float kDims = 0.4;
const float kBrightest = 0.5;
// How far a pinching finger turns to the glow's color, and how much brighter it gets then.
const float kGlowTint = 0.75;
const float kGlowBrightens = 0.25;

void main() {
    vec3 n = normalize(surface);
    vec3 v = normalize(toEye);
    if (outline > 0.0) {
        // Only the far side of the pushed-out hand shows past the hand's edge.
        if (dot(n, v) > 0.0) discard;
        vec3 color = mix(kOutline, kGlow, glowing);
        gl_FragColor = vec4(color, kOutlineOpacity * fade * (0.7 + 0.3 * glowing));
        return;
    }
    if (dot(n, v) < 0.0) n = -n;  // a face seen from inside, at an edge
    float turned = 1.0 - max(dot(n, v), 0.0);
    turned *= turned;
    vec3 color = (kSkin * (kFacing + kTurned * turned) + kRim * (turned * kRimShare)) *
                 (1.0 - kDims * (kBrightest - ambient));
    vec3 l = normalize(lightFrom);
    float diffuse = max(dot(n, l), 0.0);
    float shine = pow(max(dot(n, normalize(l + v)), 0.0), 32.0);
    color += lightColor * (diffuse * kLit + shine * kShine);
    color = mix(color, kGlow * (1.0 + kGlowBrightens) + kRim * (turned * kRimShare), glowing * kGlowTint);
    gl_FragColor = vec4(min(color, 1.0), kFillOpacity * fade);
}
