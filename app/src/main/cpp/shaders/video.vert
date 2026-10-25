// A quad over the viewport, with texture coordinates from the top left (video_renderer.cpp).
attribute vec2 position;
varying vec2 uv;
void main() {
    uv = vec2(position.x * 0.5 + 0.5, 0.5 - position.y * 0.5);  // 0, 0 at the top left
    gl_Position = vec4(position, 0.0, 1.0);
}
