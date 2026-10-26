// Gazeora's window floating in the room (room_renderer.cpp), per eye. Each pixel goes through
// the same lens warp as video.frag, then becomes a ray from the eye: through the window it
// shows the stream; past it, what's behind: the back camera, which turns with the head, or
// the 360 scene, which stays with the room.
#extension GL_OES_EGL_image_external : require
precision highp float;
uniform samplerExternalOES picture;  // the stream
uniform samplerExternalOES camera;
uniform sampler2D panorama;          // equirectangular, the middle straight ahead
uniform vec4 lens;       // k1, k2, unused, chroma correction
uniform vec4 eyeLayout;  // unused, the lens center's x in the eye, eye aspect, tan per unit
uniform float pixelTan;  // one screen pixel's angle, as a tangent
uniform vec4 crop;       // the picture in its buffer: left, top, width, height (0..1)
uniform vec2 pictureTap; // the window's 4 taps' offsets in the buffer, against shimmer
uniform mat3 screenToRoom;
uniform vec3 eye;        // this eye in the room, meters
uniform vec4 panel;      // the window's half width, half height, distance, corner radius (m)
uniform vec4 cameraCrop;
uniform mat2 viewToSensor;
uniform vec2 cameraTan;  // tangents of half the camera's view, along the sensor
uniform float backdrop;  // 0 black, 1 the camera, 2 the 360 scene
varying vec2 uv;

const float kPi = 3.14159265;
const float kShadow = 0.4;        // how dark the shadow around the window gets
const float kShadowReach = 0.14;  // meters
const float kRim = 0.1;           // a faint light edge, so the window reads as a pane
const float kRimWidth = 0.008;

// Stretched to fill the eye (room_geometry.h, FillEye); the far corners, which the lens barely
// shows, repeat the picture's edge.
vec3 Camera(vec3 view) {
    vec2 upright = vec2(view.x, -view.y) / -view.z;
    vec2 at = (viewToSensor * upright) / cameraTan * 0.5 + 0.5;
    return texture2D(camera, cameraCrop.xy + clamp(at, 0.0, 1.0) * cameraCrop.zw).rgb;
}

vec3 Scenery(vec3 room) {
    float longitude = atan(room.x, -room.z);
    float latitude = asin(clamp(room.y, -1.0, 1.0));
    return texture2D(panorama, vec2(0.5 + longitude / (2.0 * kPi), 0.5 - latitude / kPi)).rgb;
}

vec3 Backdrop(vec3 view, vec3 room) {
    if (backdrop > 1.5) return Scenery(room);
    if (backdrop > 0.5) return Camera(view);
    return vec3(0.0);
}

vec3 Picture(vec2 at) {
    vec2 p = crop.xy + at * crop.zw;
    vec2 low = crop.xy + pictureTap, high = crop.xy + crop.zw - pictureTap;
    vec3 color = texture2D(picture, clamp(p + vec2(-pictureTap.x, -pictureTap.y), low, high)).rgb;
    color += texture2D(picture, clamp(p + vec2(pictureTap.x, -pictureTap.y), low, high)).rgb;
    color += texture2D(picture, clamp(p + vec2(-pictureTap.x, pictureTap.y), low, high)).rgb;
    color += texture2D(picture, clamp(p + vec2(pictureTap.x, pictureTap.y), low, high)).rgb;
    return color * 0.25;
}

vec3 Scene(vec2 q) {
    vec3 view = normalize(vec3(q.x * eyeLayout.w, -q.y * eyeLayout.w, -1.0));
    vec3 room = screenToRoom * view;
    vec3 color = Backdrop(view, room);
    if (room.z > -0.001) return color;  // looking away from the window's wall

    // Where the ray meets the window's plane, and how far that is outside its rounded edge.
    float along = (-panel.z - eye.z) / room.z;
    vec2 at = eye.xy + room.xy * along;
    vec2 d = abs(at) - (panel.xy - panel.w);
    float outside = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - panel.w;
    float edge = along * pixelTan;
    float cover = 1.0 - smoothstep(-edge, edge, outside);

    color *= 1.0 - kShadow * (1.0 - smoothstep(0.0, kShadowReach, max(outside, 0.0)));
    vec3 pane = Picture(vec2(at.x / panel.x, -at.y / panel.y) * 0.5 + 0.5);
    pane += kRim * (1.0 - smoothstep(0.0, kRimWidth, -outside));
    return mix(color, pane, cover);
}

void main() {
    vec2 p = (uv - vec2(eyeLayout.y, 0.5)) * vec2(eyeLayout.z, 1.0) * 2.0;
    float r2 = dot(p, p);
    vec2 q = p * (1.0 + lens.x * r2 + lens.y * r2 * r2);
    if (lens.w == 0.0) {
        gl_FragColor = vec4(Scene(q), 1.0);
        return;
    }
    gl_FragColor = vec4(Scene(q * (1.0 - lens.w)).r, Scene(q).g, Scene(q * (1.0 + lens.w)).b,
                        1.0);
}
