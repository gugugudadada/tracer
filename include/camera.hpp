#pragma once

#include "global.hpp"

class Camera {
public:
    Camera(
        const Vector3f& lookfrom,
        const Vector3f& lookat,
        const Vector3f& vup,
        float vfov,      // 垂直视角，单位：度
        float aspect
    ) {
        // 将角度转为弧度
        float theta = vfov * PI / 180.0f;
        float h = std::tan(theta / 2.0f);
        float viewport_height = 2.0f * h;
        float viewport_width = aspect * viewport_height;

        origin = lookfrom;

        // 相机坐标系
        Vector3f w = (lookfrom - lookat).normalized();  // 朝后
        Vector3f u = cross(vup, w).normalized();        // 右
        Vector3f v = cross(w, u);                       // 上

        horizontal = viewport_width * u;
        vertical   = viewport_height * v;
        lower_left_corner = origin - horizontal / 2 - vertical / 2 - w;
    }

    Ray generateRay(float s, float t) const {
        // s, t 一般是 [0,1] 像素归一化坐标
        Vector3f dir = (lower_left_corner + s * horizontal + t * vertical - origin).normalized();
        return Ray(origin, dir);
    }

private:
    Vector3f origin;
    Vector3f lower_left_corner;
    Vector3f horizontal;
    Vector3f vertical;
};
