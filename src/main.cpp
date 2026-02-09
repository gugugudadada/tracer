#include <iostream>
#include <fstream>
#include "global.hpp"
#include "camera.hpp"
#include "HitRecord.hpp"
#include "Scene.hpp"
#include "MeshTriangle.hpp"
#include "Material.hpp"

enum class SceneType {
    CornellBox,
    VeachMIS,
    LivingRoom
};

struct SceneConfig {
    std::string obj_path;
    Vector3f    eye;
    Vector3f    lookat;
    Vector3f    up;
    float       vfov;
};


SceneConfig makeSceneConfig(SceneType type) {
    SceneConfig cfg{};

    if (type == SceneType::CornellBox) {
        cfg.obj_path = "../scene/cornell-box/scene.obj";

        cfg.eye    = Vector3f(0.0f, 1.0f, 6.8f);
        cfg.lookat = Vector3f(0.0f, 1.0f, 5.8f);
        cfg.up     = Vector3f(0.0f, 1.0f, 0.0f);
        cfg.vfov   = 19.5f;
    } else if (type == SceneType::VeachMIS) {
        cfg.obj_path = "../scene/veach-mis/scene.obj";

        cfg.eye    = Vector3f(28.2792f, 3.5f, 0.000001f);
        cfg.lookat = Vector3f(27.2792f, 3.5f, 0.000001f);
        cfg.up     = Vector3f(0.0f,     1.0f, 0.0f);

        // fovx = 35°, aspect = 1280/720 ≈ 16/9
        // 简化：直接使用 35° 作为 vfov，实际略紧/略宽问题不大
        cfg.vfov   = 35.0f;
    } else if (type == SceneType::LivingRoom) {
        cfg.obj_path = "../scene/living-room/scene.obj";
        cfg.eye    = Vector3f(5.10518f, 0.731065f, -2.31789f);
        cfg.lookat = Vector3f(4.143388f, 0.805472f, -2.054414f);
        cfg.up     = Vector3f(0.071763f, 0.997228f, -0.019659f);
        cfg.vfov   = 90.0f; // fovx=90，aspect≈16/9，vfov 略小但可以先用 90 简化
    }

    return cfg;
}


int main(int argc, char** argv) {
    SceneType scene_type = SceneType::CornellBox;
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "cornell") {
            scene_type = SceneType::CornellBox;
        } else if (arg == "veach") {
            scene_type = SceneType::VeachMIS;
        } else if (arg == "living") {
            scene_type = SceneType::LivingRoom;
        } else {
            std::cerr << "Unknown scene type: " << arg << "\n";
            return 1;
        }
    }

    SceneConfig cfg = makeSceneConfig(scene_type);

    const int image_width  = 200;
    const int image_height = 200;
    const int samples_per_pixel = 4;
    const int max_depth = 5;

    float aspect_ratio = static_cast<float>(image_width) / image_height;

    Camera camera(
        cfg.eye,
        cfg.lookat,
        cfg.up,
        cfg.vfov,
        aspect_ratio
    );

    Scene scene;

    MeshTriangle* mesh = new MeshTriangle(cfg.obj_path);
    scene.addObject(mesh);

    // 从 mesh 把发光三角形收集到 Scene 的 lights
    {
        std::vector<Object*> lights_from_mesh;
        const auto& emissive_tris = mesh->getEmissiveTris();
        for (auto tri : emissive_tris) {
            lights_from_mesh.push_back(tri);
        }
        scene.addLightsFromMesh(lights_from_mesh);
    }

    std::ofstream ofs("output.ppm");
    if (!ofs) {
        std::cerr << "Failed to open output.ppm for writing\n";
        return 1;
    }

    ofs << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            Vector3f pixel_color(0.0f);

            for (int s = 0; s < samples_per_pixel; ++s) {
                float u = (i + randFloat()) / static_cast<float>(image_width);
                float v = (j + randFloat()) / static_cast<float>(image_height);
                Ray r = camera.generateRay(u, v);
                pixel_color += scene.castRay(r, max_depth);
            }

            pixel_color /= static_cast<float>(samples_per_pixel);

            float r_col = std::sqrt(clamp01(pixel_color.x));
            float g_col = std::sqrt(clamp01(pixel_color.y));
            float b_col = std::sqrt(clamp01(pixel_color.z));

            int ir = static_cast<int>(255.999f * r_col);
            int ig = static_cast<int>(255.999f * g_col);
            int ib = static_cast<int>(255.999f * b_col);

            ofs << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    std::cerr << "\nDone.\n";
    ofs.close();

    return 0;
}
