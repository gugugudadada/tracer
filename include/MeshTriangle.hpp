#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "Object.hpp"
#include "Triangle.hpp"
#include "Material.hpp"
#include "tiny_obj_loader.h"

class MeshTriangle : public Object {
public:
    // MeshTriangle(const std::string& obj_path, const std::string& light_mtl_name, const Vector3f& light_radiance):light_mtl_name(light_mtl_name), light_radiance(light_radiance)
    // {
    //     loadObj(obj_path);
    // }

    MeshTriangle(const std::string& obj_path):obj_path_(obj_path)
    {
        loadObj(obj_path);
    }

    bool intersect(const Ray& ray, HitRecord& rec) const override {
        bool hit_anything = false;
        for (const auto& tri : triangles) {
            if (tri->intersect(ray, rec)) {
                hit_anything = true;
            }
        }
        return hit_anything;
    }

    bool isEmissive() const override {
        // 整个 mesh 不作为单独光源使用，光源由 emissive_tris 提供
        return false;
    }

    // 将内部所有发光三角形加入外部 lights 列表
    void collectEmissiveTriangles(std::vector<Object*>& out_lights) const {
        for (auto tri : emissive_tris) {
            out_lights.push_back(tri);
        }
    }

    ~MeshTriangle() override {
        for (auto tri : triangles) {
            delete tri;
        }
        for (auto m : materials) {
            delete m;
        }
    }

    // 调试用：打印 AABB
    void printAABB() const {
        Vector3f min_v( std::numeric_limits<float>::max());
        Vector3f max_v(-std::numeric_limits<float>::max());

        for (auto tri : triangles) {
            const Vector3f& a = tri->getV0();
            const Vector3f& b = tri->getV1();
            const Vector3f& c = tri->getV2();
            auto update = [&](const Vector3f& p) {
                min_v.x = std::min(min_v.x, p.x);
                min_v.y = std::min(min_v.y, p.y);
                min_v.z = std::min(min_v.z, p.z);
                max_v.x = std::max(max_v.x, p.x);
                max_v.y = std::max(max_v.y, p.y);
                max_v.z = std::max(max_v.z, p.z);
            };
            update(a); update(b); update(c);
        }

        std::cout << "Mesh AABB: min(" << min_v.x << ", " << min_v.y << ", " << min_v.z
                  << "), max(" << max_v.x << ", " << max_v.y << ", " << max_v.z << ")\n";
    }

    // 用于采样光源时的总面积
    float emissiveAreaSum() const { return total_emissive_area; }

    const std::vector<Triangle*>& getEmissiveTris() const { return emissive_tris; }

private:
    std::vector<Triangle*> triangles;
    std::vector<Material*> materials;
    std::unordered_map<std::string, int> mtlname_to_id;

    std::vector<Triangle*> emissive_tris;
    float total_emissive_area = 0.0f;

    std::string obj_path_;
    // std::string light_mtl_name;
    // Vector3f light_radiance;

    void loadObj(const std::string& obj_path) {
        tinyobj::ObjReaderConfig reader_config;
        // 让 tinyobj 去 obj 所在目录找 mtl
        std::string basedir = ".";
        auto slash_pos = obj_path.find_last_of("/\\");
        if (slash_pos != std::string::npos) {
            basedir = obj_path.substr(0, slash_pos);
        }
        reader_config.mtl_search_path = basedir;

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(obj_path, reader_config)) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
            }
            return;
        }

        if (!reader.Warning().empty()) {
            std::cerr << "TinyObjReader: " << reader.Warning() << std::endl;
        }

        const tinyobj::attrib_t& attrib = reader.GetAttrib();
        const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
        const std::vector<tinyobj::material_t>& obj_materials = reader.GetMaterials();

        // materials.reserve(obj_materials.size());
        // for (size_t i = 0; i < obj_materials.size(); ++i) {
        //     const auto& m = obj_materials[i];
        //     Vector3f kd(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
        //     if (obj_path_.find("veach-mis") != std::string::npos) {
        //         bool is_specular_like = (m.specular[0] > 0.0f || m.specular[1] > 0.0f || m.specular[2] > 0.0f)
        //                                 && (kd.x == 0.0f && kd.y == 0.0f && kd.z == 0.0f);
        //         if (is_specular_like) {
        //             kd = Vector3f(0.5f, 0.5f, 0.5f); // 或者更亮一点
        //         }
        //     }
        //     Material* mat = new Material(kd, Vector3f(0.0f), MaterialType::DIFFUSE);

        //     // 根据场景和材质名设置 emission
        //     if (obj_path_.find("cornell-box") != std::string::npos) {
        //         if (m.name == "Light1") {
        //             mat->m_emission = Vector3f(34.0f, 24.0f, 8.0f);
        //             mat->m_two_sided = true;
        //         }
        //     } else if (obj_path_.find("veach-mis") != std::string::npos) {
        //         if (m.name == "Light1") {
        //             mat->m_emission = Vector3f(2.0f, 2.0f, 5.0f);
        //             mat->m_two_sided = true;
        //         } else if (m.name == "Light2") {
        //             mat->m_emission = Vector3f(40.0f, 50.0f, 20.0f);
        //             mat->m_two_sided = true;
        //         } else if (m.name == "Light3") {
        //             mat->m_emission = Vector3f(500.0f, 200.0f, 200.0f);
        //             mat->m_two_sided = true;
        //         }
        //     } else if (obj_path_.find("living-room") != std::string::npos) {
        //         if (m.name == "Light1") {
        //             mat->m_emission = Vector3f(10.0f, 8.0f, 5.0f);
        //             mat->m_two_sided = true;
        //         }
        //     }

        //     if (!m.diffuse_texname.empty()) {
        //         std::string tex_path = basedir + "/" + m.diffuse_texname;
        //         mat->loadTexture(tex_path);
        //     }

        //     mtlname_to_id[m.name] = static_cast<int>(i);
        //     materials.push_back(mat);
        // }
        materials.reserve(obj_materials.size());
        for (size_t i = 0; i < obj_materials.size(); ++i) {
            const auto& m = obj_materials[i];
            Vector3f kd(m.diffuse[0], m.diffuse[1], m.diffuse[2]);

            MaterialType mat_type = MaterialType::DIFFUSE;

            Material* mat = nullptr;

            if (obj_path_.find("veach-mis") != std::string::npos) {
                // veach-mis: Smooth/Glossy/Rough/SuperRough 是镜面材质
                bool has_specular = (m.specular[0] > 0.0f || m.specular[1] > 0.0f || m.specular[2] > 0.0f);
                bool is_diffuse   = (kd.x > 0.0f || kd.y > 0.0f || kd.z > 0.0f);

                if (has_specular && !is_diffuse) {
                    // 镜面类：用 PHONG 模型
                    mat_type = MaterialType::PHONG;
                    // 给一个微弱漫反射成分（避免完全黑）
                    kd = Vector3f(0.02f, 0.02f, 0.02f);
                }
            }

            mat = new Material(kd, Vector3f(0.0f), mat_type);

            // 对 PHONG 材质设置 specular 和 exponent
            if (mat_type == MaterialType::PHONG) {
                mat->m_specular = Vector3f(m.specular[0], m.specular[1], m.specular[2]);
                // Ns 直接用作指数会很大，适当压缩一下
                float Ns = m.shininess; // tinyobj 里 Ns 在 shininess 字段
                mat->m_phong_exp = std::max(1.0f, Ns * 0.25f);
            }

            // 根据场景和材质名设置 emission（你原来的逻辑）
            if (obj_path_.find("cornell-box") != std::string::npos) {
                if (m.name == "Light1") {
                    mat->m_emission = Vector3f(34.0f, 24.0f, 8.0f);
                    mat->m_two_sided = true;
                }
            } else if (obj_path_.find("veach-mis") != std::string::npos) {
                if (m.name == "Light1") {
                    mat->m_emission = Vector3f(2.0f, 2.0f, 5.0f);
                    mat->m_two_sided = true;
                } else if (m.name == "Light2") {
                    mat->m_emission = Vector3f(40.0f, 50.0f, 20.0f);
                    mat->m_two_sided = true;
                } else if (m.name == "Light3") {
                    mat->m_emission = Vector3f(500.0f, 200.0f, 200.0f);
                    mat->m_two_sided = true;
                }
            } else if (obj_path_.find("living-room") != std::string::npos) {
                if (m.name == "Light1") {
                    mat->m_emission = Vector3f(10.0f, 8.0f, 5.0f);
                    mat->m_two_sided = true;
                }
            }

            if (!m.diffuse_texname.empty()) {
                std::string tex_path = basedir + "/" + m.diffuse_texname;
                mat->loadTexture(tex_path);
            }

            mtlname_to_id[m.name] = static_cast<int>(i);
            materials.push_back(mat);
        }


        // 2) 构建三角形，并绑定正确的材质
        for (size_t s = 0; s < shapes.size(); ++s) {
            size_t index_offset = 0;
            const auto& mesh = shapes[s].mesh;

            for (size_t f = 0; f < mesh.num_face_vertices.size(); ++f) {
                size_t fv = static_cast<size_t>(mesh.num_face_vertices[f]);
                if (fv != 3) {
                    index_offset += fv;
                    continue;
                }

                Vector3f v[3];
                Vector2f uv[3];
                for (size_t k = 0; k < 3; ++k) {
                    tinyobj::index_t idx = mesh.indices[index_offset + k];
                    float vx = attrib.vertices[3 * idx.vertex_index + 0];
                    float vy = attrib.vertices[3 * idx.vertex_index + 1];
                    float vz = attrib.vertices[3 * idx.vertex_index + 2];
                    v[k] = Vector3f(vx, vy, vz);

                    if (idx.texcoord_index >= 0) {
                        float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                        float w = attrib.texcoords[2 * idx.texcoord_index + 1];
                        uv[k] = Vector2f(u, w);
                    } else {
                        uv[k] = Vector2f(0.0f, 0.0f);
                    }
                }

                int mat_id = -1;
                if (!mesh.material_ids.empty()) {
                    mat_id = mesh.material_ids[f];
                }

                Material* face_mat = nullptr;
                if (mat_id >= 0 && mat_id < (int)materials.size()) {
                    face_mat = materials[mat_id];
                }

                // Triangle* tri = new Triangle(v[0], v[1], v[2], face_mat);
                Triangle* tri;
                bool has_uv = (mesh.indices[index_offset + 0].texcoord_index >= 0 &&
                               mesh.indices[index_offset + 1].texcoord_index >= 0 &&
                               mesh.indices[index_offset + 2].texcoord_index >= 0);
                if (has_uv) {
                    tri = new Triangle(v[0], v[1], v[2], uv[0], uv[1], uv[2], face_mat);
                } else {
                    tri = new Triangle(v[0], v[1], v[2], face_mat);
                }
                triangles.push_back(tri);

                if (face_mat && face_mat->isEmissive()) {
                    emissive_tris.push_back(tri);
                    total_emissive_area += tri->area();
                }

                index_offset += fv;
            }
        }

        std::cout << "Loaded OBJ: " << obj_path
                  << " with " << triangles.size() << " triangles." << std::endl;
        printAABB();
        std::cout << "Emissive tris: " << emissive_tris.size()
                  << ", total emissive area: " << total_emissive_area << std::endl;
    }
};
