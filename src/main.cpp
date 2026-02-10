#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

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
        cfg.vfov   = 35.0f; // 简化使用 fovx
    } else if (type == SceneType::LivingRoom) {
        cfg.obj_path = "../scene/living-room/scene.obj";
        cfg.eye    = Vector3f(5.10518f, 0.731065f, -2.31789f);
        cfg.lookat = Vector3f(4.143388f, 0.805472f, -2.054414f);
        cfg.up     = Vector3f(0.071763f, 0.997228f, -0.019659f);
        cfg.vfov   = 90.0f;
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

    const int image_width  = 256;
    const int image_height = 256;
    const int samples_per_pixel = 16;
    const int max_depth = 5;

    const int total_pixels = image_width * image_height;
    const long long total_samples = static_cast<long long>(total_pixels) * samples_per_pixel;

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

    std::vector<Vector3f> framebuffer(total_pixels);
    std::atomic<int> lines_done{0};

    auto render_range = [&](int y_start, int y_end) {
        for (int j = y_start; j < y_end; ++j) {
            for (int i = 0; i < image_width; ++i) {
                Vector3f pixel_color(0.0f);

                for (int s = 0; s < samples_per_pixel; ++s) {
                    float u = (i + randFloat()) / static_cast<float>(image_width);
                    float v = (j + randFloat()) / static_cast<float>(image_height);
                    Ray r = camera.generateRay(u, v);
                    pixel_color += scene.castRay(r, max_depth);
                }

                pixel_color /= static_cast<float>(samples_per_pixel);
                framebuffer[j * image_width + i] = pixel_color;
            }
            lines_done.fetch_add(1, std::memory_order_relaxed);
        }
    };

    int num_threads = std::thread::hardware_concurrency();
    if (num_threads <= 0) num_threads = 4;

    std::cerr << "Scene: ";
    if (scene_type == SceneType::CornellBox) std::cerr << "CornellBox";
    else if (scene_type == SceneType::VeachMIS) std::cerr << "VeachMIS";
    else if (scene_type == SceneType::LivingRoom) std::cerr << "LivingRoom";
    std::cerr << "\n";

    std::cerr << "Resolution: " << image_width << " x " << image_height
              << ", SPP = " << samples_per_pixel
              << ", MaxDepth = " << max_depth << "\n";
    std::cerr << "Total samples (primary rays): " << total_samples << "\n";
    std::cerr << "Using " << num_threads << " threads.\n";

    auto t_start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    int rows_per_thread = image_height / num_threads;
    int row_start = 0;
    for (int t = 0; t < num_threads; ++t) {
        int row_end = (t == num_threads - 1) ? image_height : (row_start + rows_per_thread);
        threads.emplace_back(render_range, row_start, row_end);
        row_start = row_end;
    }

    // 主线程打印进度
    while (lines_done.load(std::memory_order_relaxed) < image_height) {
        int done = lines_done.load(std::memory_order_relaxed);
        std::cerr << "\rScanlines done: " << done << "/" << image_height << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = t_end - t_start;
    double seconds = elapsed.count();

    std::cerr << "\nDone rendering.\n";
    std::cerr << "Total time: " << seconds << " s\n";
    if (seconds > 0.0) {
        double samples_per_sec = static_cast<double>(total_samples) / seconds;
        std::cerr << "Throughput: " << samples_per_sec << " samples/s (primary rays)\n";
    }

    // 输出 PPM
    std::ofstream ofs("output.ppm");
    if (!ofs) {
        std::cerr << "Failed to open output.ppm for writing\n";
        return 1;
    }

    ofs << "P3\n" << image_width << " " << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            Vector3f pixel_color = framebuffer[j * image_width + i];

            float r_col = std::sqrt(clamp01(pixel_color.x));
            float g_col = std::sqrt(clamp01(pixel_color.y));
            float b_col = std::sqrt(clamp01(pixel_color.z));

            int ir = static_cast<int>(255.999f * r_col);
            int ig = static_cast<int>(255.999f * g_col);
            int ib = static_cast<int>(255.999f * b_col);

            ofs << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    ofs.close();

    return 0;
}
