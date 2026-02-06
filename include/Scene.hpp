// #pragma once

// #include <vector>
// #include "Object.hpp"
// #include "Material.hpp"
// #include "Triangle.hpp" 

// class Scene {
// public:
//     Scene() = default;
//     struct LightSample {
//         Vector3f position;
//         Vector3f normal;
//         Vector3f emission;
//     };

//     // 简化版光源采样：
//     // 当前只支持 Triangle 类型的光源，并且只取几何中心作为采样点。
//     // 返回 true 表示成功采样到一个光源点。
//     bool sampleLight(LightSample& ls) const {
//         if (lights.empty()) return false;

//         int idx = static_cast<int>(randFloat() * lights.size());
//         if (idx >= static_cast<int>(lights.size())) idx = static_cast<int>(lights.size()) - 1;

//         Object* obj = lights[idx];

//         Triangle* tri = dynamic_cast<Triangle*>(obj);
//         if (!tri) {
//             return false;
//         }

//         Vector3f v0 = tri->getV0();
//         Vector3f v1 = tri->getV1();
//         Vector3f v2 = tri->getV2();

//         ls.position = (v0 + v1 + v2) / 3.0f;

//         Vector3f N = cross(v1 - v0, v2 - v0).normalized();
//         ls.normal = N;

//         Material* mat = tri->getMaterial();
//         if (mat && mat->isEmissive()) {
//             ls.emission = mat->emission();
//         } else {
//             ls.emission = Vector3f(0.0f);
//         }

//         return true;
//     }



//     void addObject(Object* obj) {
//         objects.push_back(obj);
//     }

//     // 新增：专门记录光源对象
//     void addLight(Object* light) {
//         lights.push_back(light);
//         // 同时作为普通几何也需要求交，所以也放进 objects
//         objects.push_back(light);
//     }

//     bool intersect(const Ray& ray, HitRecord& rec) const {
//         bool hit_anything = false;
//         for (const auto& obj : objects) {
//             if (obj->intersect(ray, rec)) {
//                 hit_anything = true;
//             }
//         }
//         return hit_anything;
//     }



//     Vector3f castRay(const Ray& ray, int depth) const {
//         if (depth <= 0) {
//             return Vector3f(0.0f);
//         }

//         HitRecord rec;
//         rec.t = std::numeric_limits<float>::max();

//         if (!intersect(ray, rec)) {
//             // 这里先恢复一个简简单单的背景（也可以继续用黑背景）
//             Vector3f unit_dir = ray.direction.normalized();
//             float t = 0.5f * (unit_dir.y + 1.0f);
//             return (1.0f - t) * Vector3f(1.0f, 1.0f, 1.0f) +
//                 t * Vector3f(0.5f, 0.7f, 1.0f);
//         }

//         Material* mat = rec.material;
//         if (!mat) {
//             mat = default_gray();
//         }

//         // 如果击中自发光材质（当前 Cornell 还没设置真正光源），直接返回 emission
//         if (mat->isEmissive()) {
//             return mat->emission();
//         }

//         Vector3f Le = mat->emission(); // 一般为 0

//         // 暂时先不加 L_dir（直接光照），只算间接光照路径
//         Vector3f L_dir(0.0f);

//         // 俄罗斯轮盘
//         float rr_prob = 0.8f;
//         if (randFloat() > rr_prob) {
//             return Le + L_dir;
//         }

//         float pdf = 0.0f;
//         Vector3f N = rec.N;
//         Vector3f wo = -ray.direction;
//         Vector3f wi = mat->sample(N, pdf);

//         if (pdf <= 0.0f) {
//             return Le + L_dir;
//         }

//         Ray new_ray(rec.p + N * EPSILON, wi);
//         Vector3f Li = castRay(new_ray, depth - 1);

//         Vector3f f_r = mat->eval(wi, wo, N, rec.uv);
//         float cos_theta = std::max(0.0f, dot(N, wi));

//         Vector3f L_indir = Li * f_r * (cos_theta / (pdf * rr_prob));

//         return Le + L_dir + L_indir;
//     }



//     // Vector3f castRay(const Ray& ray, int depth) const {
//     //     if (depth <= 0) {
//     //         return Vector3f(0.0f);
//     //     }

//     //     HitRecord rec;
//     //     rec.t = std::numeric_limits<float>::max();

//     //     if (!intersect(ray, rec)) {
//     //         return Vector3f(0.0f);
//     //     }

//     //     Material* mat = rec.material;
//     //     if (!mat) {
//     //         mat = default_gray();
//     //     }

//     //     if (mat->isEmissive()) {
//     //         return mat->emission();
//     //     }

//     //     Vector3f Le = mat->emission(); // 一般为 0

//     //     Vector3f L_dir(0.0f);

//     //     // 简化直接光照：从光源取一个采样点（几何中心），发出 shadow ray
//     //     LightSample ls;
//     //     if (sampleLight(ls) && (ls.emission.x > 0.0f || ls.emission.y > 0.0f || ls.emission.z > 0.0f)) {
//     //         Vector3f light_dir = (ls.position - rec.p);
//     //         float dist2 = light_dir.length2();
//     //         float dist = std::sqrt(dist2);
//     //         light_dir /= dist; // 归一化

//     //         // 阴影检测：从当前点稍微偏移法线方向，向光源方向投射
//     //         Ray shadow_ray(rec.p + rec.N * EPSILON, light_dir);
//     //         HitRecord shadow_rec;
//     //         shadow_rec.t = dist - EPSILON;  // 只关心光源之前是否有遮挡物

//     //         if (!intersect(shadow_ray, shadow_rec)) {
//     //             // 没有遮挡，看作直接可见光源
//     //             Vector3f N = rec.N;
//     //             Vector3f wo = -ray.direction;  // 出射方向
//     //             Vector3f wi = light_dir;       // 来自光源的方向

//     //             Vector3f f_r = mat->eval(wi, wo, N, rec.uv);
//     //             float cos_theta = std::max(0.0f, dot(N, wi));

//     //             // 注意：这里我们没有正确除以面积 pdf 和 cosθ_light / distance^2，只是先给一个简单版本
//     //             // 先写成：L_dir = Le_light * f_r * cos_theta
//     //             L_dir = ls.emission * f_r * cos_theta;
//     //         }
//     //     }


//     //     float rr_prob = 0.8f;
//     //     if (randFloat() > rr_prob) {
//     //         return Le + L_dir;
//     //     }

//     //     float pdf = 0.0f;
//     //     Vector3f N = rec.N;
//     //     Vector3f wo = -ray.direction;
//     //     Vector3f wi = mat->sample(N, pdf);

//     //     if (pdf <= 0.0f) {
//     //         return Le;
//     //     }

//     //     Ray new_ray(rec.p + N * EPSILON, wi);
//     //     Vector3f Li = castRay(new_ray, depth - 1);

//     //     Vector3f f_r = mat->eval(wi, wo, N, rec.uv);
//     //     float cos_theta = std::max(0.0f, dot(N, wi));

//     //     Vector3f L_indir = Li * f_r * (cos_theta / (pdf * rr_prob));

//     //     return Le + L_dir + L_indir;
//     // }

//     ~Scene() {
//         // 由于 lights 里的指针与 objects 共享，不要 double delete。
//         // 简单策略：只 delete objects 中独有的那一份。
//         // 为避免重复，我们只 delete objects，且保证 addLight 不把同一指针多次加入 objects。
//         // 当前实现中：addObject 不会插入光源，addLight 会插入光源一次，所以 objects 集合里每个 ptr 只有一次。

//         for (auto obj : objects) {
//             delete obj;
//         }
//     }

// private:
//     std::vector<Object*> objects;
//     std::vector<Object*> lights;  // emitters 列表

//     static Material* default_gray() {
//         static Material gray(Vector3f(0.8f, 0.8f, 0.8f),
//                              Vector3f(0.0f),
//                              MaterialType::DIFFUSE);
//         return &gray;
//     }
// };

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
