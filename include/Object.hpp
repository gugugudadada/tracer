#pragma once

#include "global.hpp"
#include "HitRecord.hpp"

// 所有可被光线求交的几何体的抽象基类
class Object {
public:
    virtual ~Object() = default;

    // 射线与物体相交
    // 若有交点，返回 true，并填充 rec（且 rec.t 要是当前最近的交点）
    // hit_t_min 可用于限制最小 t（防止自交），我们暂时先不传，在 Scene 里统一处理
    virtual bool intersect(const Ray& ray, HitRecord& rec) const = 0;

    // 某些对象可能需要知道自己是否是发光体，这里先留个接口（可选）
    virtual bool isEmissive() const { return false; }
};
