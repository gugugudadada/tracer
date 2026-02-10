#pragma once

#include "global.hpp"
#include "stb_image.h"

enum class MaterialType {
    DIFFUSE,
    PHONG
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

    // Vector3f eval(const Vector3f& wi,
    //               const Vector3f& /*wo*/,
    //               const Vector3f& N,
    //               const Vector2f& uv) const
    // {
    //     if (m_type != MaterialType::DIFFUSE) return Vector3f(0.0f);
    //     float cos_theta = dot(N, wi);
    //     if (cos_theta <= 0.0f) return Vector3f(0.0f);

    //     Vector3f albedo = m_color;
    //     if (has_texture) {
    //         albedo = sampleTexture(uv.x, uv.y);
    //     }

    //     return albedo * (1.0f / PI);
    // }
    Vector3f eval(const Vector3f& wi,
              const Vector3f& wo,
              const Vector3f& N,
              const Vector2f& uv) const
    {
        if (dot(N, wi) <= 0.0f || dot(N, wo) <= 0.0f) {
            return Vector3f(0.0f);
        }

        // 漫反射部分（Lambert）
        Vector3f albedo = m_color;
        if (has_texture) {
            albedo = sampleTexture(uv.x, uv.y);
        }
        Vector3f f_diffuse = Vector3f(0.0f);
        if (m_type == MaterialType::DIFFUSE || m_type == MaterialType::PHONG) {
            f_diffuse = albedo * (1.0f / PI);
        }

        // 纯漫反射材质：只返回 diffuse
        if (m_type == MaterialType::DIFFUSE) {
            return f_diffuse;
        }

        // PHONG 高光部分
        if (m_type == MaterialType::PHONG) {
            // 这里使用 Blinn-Phong：半程向量 h = normalize(wi+wo)
            Vector3f h = (wi + wo).normalized();
            float nh = std::max(0.0f, dot(N, h));
            if (nh <= 0.0f || m_phong_exp <= 0.0f) {
                return f_diffuse;
            }

            // Blinn-Phong 的标准形式：ks * (n+2)/(2π) * (N·H)^n
            Vector3f f_spec = m_specular * ((m_phong_exp + 2.0f) / (2.0f * PI)) * std::pow(nh, m_phong_exp);

            // 简化：返回 diffuse + specular
            return f_diffuse + f_spec;
        }

        return Vector3f(0.0f);
    }


    // Vector3f sample(const Vector3f& N, float& pdf) const {
    //     float r1 = 2.0f * PI * randFloat();
    //     float r2 = randFloat();
    //     float r2s = std::sqrt(r2);

    //     float x = std::cos(r1) * r2s;
    //     float y = std::sin(r1) * r2s;
    //     float z = std::sqrt(1.0f - r2);

    //     Vector3f w = N;
    //     Vector3f a = (std::fabs(w.x) > 0.1f) ? Vector3f(0.0f, 1.0f, 0.0f) : Vector3f(1.0f, 0.0f, 0.0f);
    //     Vector3f v = cross(w, a).normalized();
    //     Vector3f u = cross(v, w);

    //     Vector3f dir = (u * x + v * y + w * z).normalized();
    //     pdf = dot(N, dir) / PI;
    //     return dir;
    // }
    Vector3f sample(const Vector3f& N, float& pdf) const {
        if (m_type == MaterialType::DIFFUSE) {
            // 余弦加权半球采样
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

        if (m_type == MaterialType::PHONG) {
            // 这里做一个简单版本：仍然用余弦半球采样
            // （严格来说应该对高光 lobe 采样，但为了先看到效果，先沿用 diffuse 的采样）
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

        pdf = 0.0f;
        return N;
    }


    // float pdf(const Vector3f& wi, const Vector3f& N) const {
    //     float cos_theta = dot(N, wi);
    //     if (cos_theta <= 0.0f) return 0.0f;
    //     return cos_theta / PI;
    // }
    float pdf(const Vector3f& wi, const Vector3f& N) const {
        float cos_theta = dot(N, wi);
        if (cos_theta <= 0.0f) return 0.0f;

        // 当前 sample 对 DIFFUSE 和 PHONG 都使用余弦半球采样
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
    // 高光参数（用于 PHONG）
    Vector3f m_specular = Vector3f(0.0f); // 高光颜色（来自 Ks）
    float    m_phong_exp = 0.0f;         // 高光指数（来自 Ns）
};
