#pragma once

#include "global.hpp"

// 前向声明，避免头文件循环依赖
class Material;

struct HitRecord {
    Vector3f p;           // 交点位置
    Vector3f N;           // 交点处法线（朝外方向）
    Vector2f uv;          // 纹理坐标
    Material* material;   // 指向命中的材质

    float t;              // 光线参数 t（Ray(origin + t * dir)）
    bool front_face;      // 是否是从物体外部射入（true: 正面）

    HitRecord()
        : p(), N(), uv(), material(nullptr),
          t(std::numeric_limits<float>::max()), front_face(true) {}

    // 设定法线方向，使其总是与光线方向相反（这在路径追踪中很常用）
    inline void set_face_normal(const Ray& r, const Vector3f& outward_normal) {
        front_face = dot(r.direction, outward_normal) < 0;
        N = front_face ? outward_normal : -outward_normal;
    }
};
