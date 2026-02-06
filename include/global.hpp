#pragma once

#include <cmath>
#include <algorithm>
#include <random>
#include <limits>

// 常量
constexpr float PI = 3.14159265358979323846f;
constexpr float EPSILON = 1e-4f;

// 2D 向量
struct Vector2f {
    float x, y;

    Vector2f() : x(0), y(0) {}
    Vector2f(float _x, float _y) : x(_x), y(_y) {}
};

// 3D 向量
struct Vector3f {
    float x, y, z;

    Vector3f() : x(0), y(0), z(0) {}
    Vector3f(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // 一些便捷构造
    explicit Vector3f(float v) : x(v), y(v), z(v) {}

    // 运算符重载
    Vector3f operator + (const Vector3f& v) const {
        return Vector3f(x + v.x, y + v.y, z + v.z);
    }

    Vector3f operator - (const Vector3f& v) const {
        return Vector3f(x - v.x, y - v.y, z - v.z);
    }

    Vector3f operator * (float s) const {
        return Vector3f(x * s, y * s, z * s);
    }

    Vector3f operator / (float s) const {
        float inv = 1.0f / s;
        return Vector3f(x * inv, y * inv, z * inv);
    }

    Vector3f& operator += (const Vector3f& v) {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }

    Vector3f& operator -= (const Vector3f& v) {
        x -= v.x; y -= v.y; z -= v.z;
        return *this;
    }

    Vector3f& operator *= (float s) {
        x *= s; y *= s; z *= s;
        return *this;
    }

    Vector3f& operator /= (float s) {
        float inv = 1.0f / s;
        x *= inv; y *= inv; z *= inv;
        return *this;
    }

    Vector3f operator - () const {
        return Vector3f(-x, -y, -z);
    }

    float length() const {
        return std::sqrt(x*x + y*y + z*z);
    }

    float length2() const {
        return x*x + y*y + z*z;
    }

    Vector3f normalized() const {
        float len = length();
        if (len > 0) {
            float inv = 1.0f / len;
            return Vector3f(x * inv, y * inv, z * inv);
        }
        return *this;
    }
};

// 标量 * 向量
inline Vector3f operator * (float s, const Vector3f& v) {
    return Vector3f(v.x * s, v.y * s, v.z * s);
}

// 分量相乘（Hadamard）
inline Vector3f operator * (const Vector3f& a, const Vector3f& b) {
    return Vector3f(a.x * b.x, a.y * b.y, a.z * b.z);
}

// 点积
inline float dot(const Vector3f& a, const Vector3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// 叉积
inline Vector3f cross(const Vector3f& a, const Vector3f& b) {
    return Vector3f(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

// clamp 一个 float 到 [0,1]
inline float clamp01(float x) {
    return std::max(0.0f, std::min(1.0f, x));
}

// Ray 定义
struct Ray {
    Vector3f origin;
    Vector3f direction; // 应该是归一化的

    Ray() = default;
    Ray(const Vector3f& o, const Vector3f& d) : origin(o), direction(d) {}

    Vector3f at(float t) const {
        return origin + direction * t;
    }
};

// 随机数工具：全局使用一个随机引擎
inline float randFloat() {
    // 使用 thread_local 避免多线程冲突（目前我们先单线程，可以先这样写）
    static thread_local std::mt19937 generator(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    return distribution(generator);
}
