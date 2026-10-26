#pragma once

#include <cmath>

namespace gazeora {

struct Vec3 {
    float x = 0, y = 0, z = 0;

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 Cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float Length() const { return std::sqrt(Dot(*this)); }
    Vec3 Normalized() const {
        float length = Length();
        return length > 0 ? *this * (1 / length) : *this;
    }
};

// A rotation as a 3x3 matrix, row by row: Apply(v) is m * v.
struct Mat3 {
    float m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    static Mat3 FromRows(const Vec3& a, const Vec3& b, const Vec3& c) {
        return {{{a.x, a.y, a.z}, {b.x, b.y, b.z}, {c.x, c.y, c.z}}};
    }
    static Mat3 FromColumns(const Vec3& a, const Vec3& b, const Vec3& c) {
        return {{{a.x, b.x, c.x}, {a.y, b.y, c.y}, {a.z, b.z, c.z}}};
    }
    Vec3 Apply(const Vec3& v) const {
        return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
    }
    Mat3 operator*(const Mat3& o) const {
        Mat3 r;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
            }
        }
        return r;
    }
    // Column-major, as glUniformMatrix3fv takes it without transposing.
    void ToColumnMajor(float out[9]) const {
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) out[c * 3 + r] = m[r][c];
        }
    }
};

}  // namespace gazeora
