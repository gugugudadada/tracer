// #pragma once

// #include "global.hpp"

// // 一个简单的材质类型枚举，目前只用 DIFFUSE，后面可扩展
// enum class MaterialType {
//     DIFFUSE
// };

// class Material {
// public:
//     Material(
//         const Vector3f& color = Vector3f(0.8f, 0.8f, 0.8f),
//         const Vector3f& emission = Vector3f(0.0f),
//         MaterialType type = MaterialType::DIFFUSE
//     )
//         : m_color(color)
//         , m_emission(emission)
//         , m_type(type)
//         , m_two_sided(false)
//     {}

//     // 自发光（直接作为辐射亮度 Le，用于光源）
//     Vector3f emission() const {
//         return m_emission;
//     }

//     bool isEmissive() const {
//         return (m_emission.x > 0.0f || m_emission.y > 0.0f || m_emission.z > 0.0f);
//     }

//     // 简单 Lambert BRDF：f(wi, wo) = albedo / PI
//     // 参数：
//     //  wi: 入射方向（指向表面）
//     //  wo: 出射方向（指向摄像机）
//     //  N : 表面法线（指向外侧）
//     //  uv: 纹理坐标（现在先不用）
//     Vector3f eval(const Vector3f& wi,
//                   const Vector3f& wo,
//                   const Vector3f& N,
//                   const Vector2f& /*uv*/) const
//     {
//         if (m_type != MaterialType::DIFFUSE) {
//             return Vector3f(0.0f);
//         }

//         float cos_theta = dot(N, wi);
//         if (cos_theta <= 0.0f) {
//             return Vector3f(0.0f);
//         }

//         // Lambert: constant albedo / PI
//         return m_color * (1.0f / PI);
//     }

//     // 从以法线为中心的半球上做余弦加权采样，返回方向 wo（指向半球外）
//     // 同时返回该方向的 pdf（在半球上）
//     Vector3f sample(const Vector3f& N, float& pdf) const {
//         // 采样局部坐标系中的方向 (x,y,z)，z 为法线方向
//         float r1 = 2.0f * PI * randFloat();
//         float r2 = randFloat();
//         float r2s = std::sqrt(r2);

//         float x = std::cos(r1) * r2s;
//         float y = std::sin(r1) * r2s;
//         float z = std::sqrt(1.0f - r2);

//         // 构建以 N 为 z 轴的正交基 (u, v, w)
//         Vector3f w = N;
//         Vector3f a = (std::fabs(w.x) > 0.1f) ? Vector3f(0.0f, 1.0f, 0.0f) : Vector3f(1.0f, 0.0f, 0.0f);
//         Vector3f v = cross(w, a).normalized();
//         Vector3f u = cross(v, w);

//         // 从局部坐标系变换到世界坐标系
//         Vector3f dir = (u * x + v * y + w * z).normalized();

//         pdf = dot(N, dir) / PI;  // 余弦加权半球采样的 pdf
//         return dir;
//     }

//     // 半球采样的 pdf（给定一个方向），以后可能会用到
//     float pdf(const Vector3f& wi, const Vector3f& N) const {
//         float cos_theta = dot(N, wi);
//         if (cos_theta <= 0.0f) return 0.0f;
//         return cos_theta / PI;
//     }

//     // 成员公开访问（简单起见）
//     Vector3f m_color;     // 漫反射颜色
//     Vector3f m_emission;  // 自发光（光源用）
//     MaterialType m_type;
//     bool m_two_sided;     // 双面材质，后续用于灯光

//     // 纹理相关的成员先留空，后面会加入：
//     // bool has_texture;
//     // unsigned char* texture_data;
//     // int tex_width, tex_height, tex_channels;
// };
#pragma once

#include "global.hpp"
#include "stb_image.h"

enum class MaterialType {
    DIFFUSE
};

class Material {
public:
    Material(
        const Vector3f& color = Vector3f(0.8f, 0.8f, 0.8f),
        const Vector3f& emission = Vector3f(0.0f),
        MaterialType type = MaterialType::DIFFUSE
    )
        : m_color(color), m_emission(emission), m_type(type), m_two_sided(false)
    {}

    Vector3f emission() const { return m_emission; }

    bool isEmissive() const {
        return (m_emission.x > 0.0f || m_emission.y > 0.0f || m_emission.z > 0.0f);
    }

    Vector3f eval(const Vector3f& wi,
                  const Vector3f& /*wo*/,
                  const Vector3f& N,
                  const Vector2f& uv) const
    {
        if (m_type != MaterialType::DIFFUSE) return Vector3f(0.0f);
        float cos_theta = dot(N, wi);
        if (cos_theta <= 0.0f) return Vector3f(0.0f);

        Vector3f albedo = m_color;
        if (has_texture) {
            albedo = sampleTexture(uv.x, uv.y);
        }

        return albedo * (1.0f / PI);
    }

    Vector3f sample(const Vector3f& N, float& pdf) const {
        float r1 = 2.0f * PI * randFloat();
        float r2 = randFloat();
        float r2s = std::sqrt(r2);

        float x = std::cos(r1) * r2s;
        float y = std::sin(r1) * r2s;
        float z = std::sqrt(1.0f - r2);

        Vector3f w = N;
        Vector3f a = (std::fabs(w.x) > 0.1f) ? Vector3f(0.0f, 1.0f, 0.0f) : Vector3f(1.0f, 0.0f, 0.0f);
        Vector3f v = cross(w, a).normalized();
        Vector3f u = cross(v, w);

        Vector3f dir = (u * x + v * y + w * z).normalized();
        pdf = dot(N, dir) / PI;
        return dir;
    }

    float pdf(const Vector3f& wi, const Vector3f& N) const {
        float cos_theta = dot(N, wi);
        if (cos_theta <= 0.0f) return 0.0f;
        return cos_theta / PI;
    }

    bool loadTexture(const std::string& path) {
        if (tex_data) {
            stbi_image_free(tex_data);
            tex_data = nullptr;
        }
        tex_data = stbi_load(path.c_str(), &tex_width, &tex_height, &tex_channels, 0);
        if (!tex_data) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            has_texture = false;
            return false;
        }
        has_texture = true;
        tex_path = path;
        return true;
    }

    Vector3f sampleTexture(float u, float v) const {
        if (!has_texture || !tex_data || tex_width <= 0 || tex_height <= 0) {
            return m_color;
        }

        u = u - std::floor(u);
        v = v - std::floor(v);
        if (u < 0) u += 1.0f;
        if (v < 0) v += 1.0f;

        int x = static_cast<int>(u * tex_width);
        int y = static_cast<int>((1.0f - v) * tex_height);
        if (x >= tex_width) x = tex_width - 1;
        if (y >= tex_height) y = tex_height - 1;

        int idx = (y * tex_width + x) * tex_channels;
        float r = tex_data[idx] / 255.0f;
        float g = tex_data[idx + 1] / 255.0f;
        float b = tex_data[idx + 2] / 255.0f;
        return Vector3f(r, g, b);
    }

    Vector3f m_color;
    Vector3f m_emission;
    MaterialType m_type;
    bool m_two_sided;

    bool has_texture = false;
    unsigned char* tex_data = nullptr;
    int tex_width = 0, tex_height = 0, tex_channels = 0;
    std::string tex_path;
};
