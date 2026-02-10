#pragma once

#include <vector>
#include "Object.hpp"
#include "Material.hpp"
#include "Triangle.hpp"

class Scene {
public:
    Scene() = default;

    void addObject(Object* obj) {
        objects.push_back(obj);
    }

    // 光源统一用 Triangle* 存
    void addLight(Object* light) {
        lights.push_back(light);
        objects.push_back(light);
    }

    // 允许直接从 MeshTriangle 收集 emissive tris
    void addLightsFromMesh(const std::vector<Object*>& mesh_lights) {
        for (auto l : mesh_lights) {
            lights.push_back(l);
        }
    }

    bool intersect(const Ray& ray, HitRecord& rec) const {
        bool hit_anything = false;
        for (const auto& obj : objects) {
            if (obj->intersect(ray, rec)) {
                hit_anything = true;
            }
        }
        return hit_anything;
    }

    struct LightSample {
        Vector3f position;
        Vector3f normal;
        Vector3f emission;
        float pdf; // area pdf
    };

    // 对所有 Triangle 光源按面积采样
    bool sampleLight(LightSample& ls) const {
        if (lights.empty()) return false;

        // 首先收集所有光源三角形及其面积和总面积
        std::vector<Triangle*> tris;
        tris.reserve(lights.size());
        float total_area = 0.0f;

        for (auto obj : lights) {
            Triangle* tri = dynamic_cast<Triangle*>(obj);
            if (!tri) continue;
            float a = tri->area();
            if (a <= 0.0f) continue;
            tris.push_back(tri);
            total_area += a;
        }

        if (tris.empty() || total_area <= 0.0f) return false;

        float r = randFloat() * total_area;
        Triangle* chosen = nullptr;
        for (auto tri : tris) {
            float a = tri->area();
            if (r <= a) {
                chosen = tri;
                break;
            }
            r -= a;
        }
        if (!chosen) chosen = tris.back();

        Vector3f v0 = chosen->getV0();
        Vector3f v1 = chosen->getV1();
        Vector3f v2 = chosen->getV2();

        float r1 = randFloat();
        float r2 = randFloat();
        float sqrt_r1 = std::sqrt(r1);

        float u = 1.0f - sqrt_r1;
        float v = r2 * sqrt_r1;
        float w = 1.0f - u - v;

        Vector3f p = u * v0 + v * v1 + w * v2;

        Vector3f N = cross(v1 - v0, v2 - v0).normalized();
        Material* mat = chosen->getMaterial();
        Vector3f Le(0.0f);
        if (mat && mat->isEmissive()) {
            Le = mat->emission();
        }

        ls.position = p;
        ls.normal = N;
        ls.emission = Le;
        ls.pdf = 1.0f / total_area;
        return true;
    }

    Vector3f castRay(const Ray& ray, int depth) const {
        if (depth <= 0) {
            return Vector3f(0.0f);
        }

        HitRecord rec;
        rec.t = std::numeric_limits<float>::max();

        if (!intersect(ray, rec)) {
            // 对标准 Cornell，一般用黑背景，这里先用黑
            return Vector3f(0.0f);
        }

        Material* mat = rec.material;
        if (!mat) {
            mat = default_gray();
        }

        if (mat->isEmissive()) {
            // 只在直接命中时返回 Le
            return mat->emission();
        }

        Vector3f Le = mat->emission();  // 一般为 0

        // --- 直接光照 L_dir ---
        Vector3f L_dir(0.0f);
        LightSample ls;
        if (sampleLight(ls) && (ls.emission.x > 0.0f || ls.emission.y > 0.0f || ls.emission.z > 0.0f)) {
            Vector3f light_dir = ls.position - rec.p;
            float dist2 = light_dir.length2();
            float dist = std::sqrt(dist2);
            light_dir /= dist;

            // 阴影检测
            Ray shadow_ray(rec.p + rec.N * EPSILON, light_dir);
            HitRecord shadow_rec;
            shadow_rec.t = dist - EPSILON;
            if (!intersect(shadow_ray, shadow_rec)) {
                Vector3f N = rec.N;
                Vector3f wo = -ray.direction;
                Vector3f wi = light_dir;

                Vector3f f_r = mat->eval(wi, wo, N, rec.uv);
                float cos_theta = std::max(0.0f, dot(N, wi));
                float cos_theta_light = std::max(0.0f, dot(ls.normal, -wi));

                if (ls.pdf > 0.0f && cos_theta_light > 0.0f) {
                    L_dir = ls.emission * f_r * cos_theta * cos_theta_light / (ls.pdf * dist2);
                }
            }
        }

        // --- 间接光照 L_indir ---
        float rr_prob = 0.8f;
        if (randFloat() > rr_prob) {
            return Le + L_dir;
        }

        float pdf = 0.0f;
        Vector3f N = rec.N;
        Vector3f wo = -ray.direction;
        Vector3f wi = mat->sample(N, pdf);

        if (pdf <= 0.0f) {
            return Le + L_dir;
        }

        Ray new_ray(rec.p + N * EPSILON, wi);
        Vector3f Li = castRay(new_ray, depth - 1);

        Vector3f f_r = mat->eval(wi, wo, N, rec.uv);
        float cos_theta = std::max(0.0f, dot(N, wi));

        Vector3f L_indir = Li * f_r * (cos_theta / (pdf * rr_prob));

        // return Le + L_dir;
        return Le + L_dir + L_indir;
    }

    ~Scene() {
        for (auto obj : objects) {
            delete obj;
        }
    }

private:
    std::vector<Object*> objects;
    std::vector<Object*> lights;

    static Material* default_gray() {
        static Material gray(Vector3f(0.8f, 0.8f, 0.8f),
                             Vector3f(0.0f),
                             MaterialType::DIFFUSE);
        return &gray;
    }
};
