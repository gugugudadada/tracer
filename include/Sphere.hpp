#pragma once

#include "Object.hpp"

// 前向声明 Material
class Material;

class Sphere : public Object {
public:
    Sphere(const Vector3f& center, float radius, Material* mat = nullptr)
        : center(center), radius(radius), material(mat) {}

    bool intersect(const Ray& ray, HitRecord& rec) const override {
        Vector3f oc = ray.origin - center;
        float a = ray.direction.length2();
        float half_b = dot(oc, ray.direction);
        float c = oc.length2() - radius * radius;

        float discriminant = half_b * half_b - a * c;
        if (discriminant < 0.0f) {
            return false;
        }
        float sqrt_discriminant = std::sqrt(discriminant);

        // 找最近的 t
        float t = (-half_b - sqrt_discriminant) / a;
        if (t < EPSILON) {
            t = (-half_b + sqrt_discriminant) / a;
            if (t < EPSILON) {
                return false;
            }
        }

        if (t >= rec.t) {
            return false;
        }

        rec.t = t;
        rec.p = ray.at(t);

        // 外法线
        Vector3f outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(ray, outward_normal);

        // 简单球体 UV（可选，用于后面贴图）
        // u: [0,1] 从 +x 轴绕 y 轴旋转
        // v: [0,1] 从南极到北极
        float theta = std::acos(clamp01(-outward_normal.y)); // polar angle
        float phi = std::atan2(-outward_normal.z, outward_normal.x) + PI;

        float u = phi / (2.0f * PI);
        float v = theta / PI;

        rec.uv = Vector2f(u, v);
        rec.material = material;

        return true;
    }

private:
    Vector3f center;
    float radius;
    Material* material;
};
