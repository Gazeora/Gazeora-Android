#pragma once

#include <cmath>

namespace gazeora {

// A point or direction in the camera's space: x right, y down, z ahead.
struct Vec3 {
    float x = 0;
    float y = 0;
    float z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float Length(Vec3 a) { return std::sqrt(Dot(a, a)); }
inline Vec3 Normalize(Vec3 a, Vec3 fallback) {
    float length = Length(a);
    return length > 1e-9f ? a * (1 / length) : fallback;
}
// a without its part along the unit `axis`.
inline Vec3 Flatten(Vec3 a, Vec3 axis) { return a - axis * Dot(a, axis); }

}  // namespace gazeora
