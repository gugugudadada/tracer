// #pragma once

// #include "Object.hpp"

// // 前向声明 Material（避免循环引用）
// class Material;

// class Triangle : public Object {
// public:
//     Triangle(
//         const Vector3f& v0,
//         const Vector3f& v1,
//         const Vector3f& v2,
//         Material* mat = nullptr
//     )
//         : v0(v0), v1(v1), v2(v2),
//           uv0(0.0f, 0.0f), uv1(0.0f, 0.0f), uv2(0.0f, 0.0f),
//           material(mat),
//           has_uv(false)
//     {}

//     Triangle(
//         const Vector3f& v0,
//         const Vector3f& v1,
//         const Vector3f& v2,
//         const Vector2f& uv0,
//         const Vector2f& uv1,
//         const Vector2f& uv2,
//         Material* mat = nullptr
//     )
//         : v0(v0), v1(v1), v2(v2),
//           uv0(uv0), uv1(uv1), uv2(uv2),
//           material(mat),
//           has_uv(true)
//     {}

//     const Vector3f& getV0() const { return v0; }
//     const Vector3f& getV1() const { return v1; }
//     const Vector3f& getV2() const { return v2; }

//     Material* getMaterial() const { return material; }

//     // 射线 - 三角形相交（Möller–Trumbore）
//     bool intersect(const Ray& ray, HitRecord& rec) const override {
//         const float EPS = 1e-6f;

//         Vector3f edge1 = v1 - v0;
//         Vector3f edge2 = v2 - v0;

//         Vector3f pvec = cross(ray.direction, edge2);
//         float det = dot(edge1, pvec);

//         if (std::fabs(det) < EPS) {
//             return false; // 射线与三角形平行
//         }

//         float invDet = 1.0f / det;

//         Vector3f tvec = ray.origin - v0;
//         float u = dot(tvec, pvec) * invDet;
//         if (u < 0.0f || u > 1.0f) {
//             return false;
//         }

//         Vector3f qvec = cross(tvec, edge1);
//         float v = dot(ray.direction, qvec) * invDet;
//         if (v < 0.0f || u + v > 1.0f) {
//             return false;
//         }

//         float t = dot(edge2, qvec) * invDet;
//         if (t < EPS) {
//             return false; // 在射线起点后方，或太近
//         }

//         // 如果这个交点比当前记录的更远，就不更新
//         if (t >= rec.t) {
//             return false;
//         }

//         // 更新 HitRecord
//         rec.t = t;
//         rec.p = ray.at(t);

//         // 几何法线（未归一化 -> 归一化）
//         Vector3f outward_normal = cross(edge1, edge2).normalized();
//         rec.set_face_normal(ray, outward_normal);

//         // UV 重心插值（如果有）
//         if (has_uv) {
//             float w = 1.0f - u - v;
//             rec.uv = Vector2f(
//                 w * uv0.x + u * uv1.x + v * uv2.x,
//                 w * uv0.y + u * uv1.y + v * uv2.y
//             );
//         } else {
//             rec.uv = Vector2f(0.0f, 0.0f);
//         }

//         rec.material = material;

//         return true;
//     }

// private:
//     Vector3f v0, v1, v2;
//     Vector2f uv0, uv1, uv2;
//     Material* material;
//     bool has_uv;
// };
#pragma once

#include "Object.hpp"

// 前向声明 Material
class Material;

class Triangle : public Object {
public:
    Triangle(
        const Vector3f& v0,
        const Vector3f& v1,
        const Vector3f& v2,
        Material* mat = nullptr
    )
        : v0(v0), v1(v1), v2(v2),
          uv0(0.0f, 0.0f), uv1(0.0f, 0.0f), uv2(0.0f, 0.0f),
          material(mat),
          has_uv(false)
    {
        updateArea();
    }

    Triangle(
        const Vector3f& v0,
        const Vector3f& v1,
        const Vector3f& v2,
        const Vector2f& uv0,
        const Vector2f& uv1,
        const Vector2f& uv2,
        Material* mat = nullptr
    )
        : v0(v0), v1(v1), v2(v2),
          uv0(uv0), uv1(uv1), uv2(uv2),
          material(mat),
          has_uv(true)
    {
        updateArea();
    }

    bool intersect(const Ray& ray, HitRecord& rec) const override {
        const float EPS = 1e-6f;

        Vector3f edge1 = v1 - v0;
        Vector3f edge2 = v2 - v0;

        Vector3f pvec = cross(ray.direction, edge2);
        float det = dot(edge1, pvec);

        if (std::fabs(det) < EPS) {
            return false;
        }

        float invDet = 1.0f / det;

        Vector3f tvec = ray.origin - v0;
        float u = dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f) {
            return false;
        }

        Vector3f qvec = cross(tvec, edge1);
        float v = dot(ray.direction, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f) {
            return false;
        }

        float t = dot(edge2, qvec) * invDet;
        if (t < EPS || t >= rec.t) {
            return false;
        }

        rec.t = t;
        rec.p = ray.at(t);

        Vector3f outward_normal = cross(edge1, edge2).normalized();
        rec.set_face_normal(ray, outward_normal);

        if (has_uv) {
            float w = 1.0f - u - v;
            rec.uv = Vector2f(
                w * uv0.x + u * uv1.x + v * uv2.x,
                w * uv0.y + u * uv1.y + v * uv2.y
            );
        } else {
            rec.uv = Vector2f(0.0f, 0.0f);
        }

        rec.material = material;
        return true;
    }

    // --- 新增的一些 getter，用于 Mesh / Light 采样 ---
    const Vector3f& getV0() const { return v0; }
    const Vector3f& getV1() const { return v1; }
    const Vector3f& getV2() const { return v2; }
    Material* getMaterial() const { return material; }

    float area() const { return m_area; }

private:
    Vector3f v0, v1, v2;
    Vector2f uv0, uv1, uv2;
    Material* material;
    bool has_uv;
    float m_area = 0.0f;

    void updateArea() {
        m_area = 0.5f * cross(v1 - v0, v2 - v0).length();
    }
};
