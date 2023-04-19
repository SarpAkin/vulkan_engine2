#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <vke/buffer.hpp>
#include <vke/commandbuffer.hpp>
#include <vke/core.hpp>
#include <vke/descriptor_pool.hpp>
#include <vke/descriptor_set_builder.hpp>
#include <vke/descriptor_set_layout_builder.hpp>
#include <vke/engine.hpp>
#include <vke/fwd.hpp>
#include <vke/image.hpp>
#include <vke/pipeline.hpp>
#include <vke/pipeline_builder.hpp>
#include <vke/renderpass.hpp>
#include <vke/util.hpp>
#include <vke/vertex_input_builder.hpp>
#include <vke/window_sdl.hpp>

#include <vke/imgui/imgui_manager.hpp>

#include <imgui.h>

#include "camera.hpp"
#include "raytracer.hpp"

struct Vertex {
    f32 pos[2];
    f32 color[3];
};

struct Push {
    f32 color[4];
};

class App : public vke::RenderEngine {
public:
    App() {
        m_renderpass = std::make_unique<vke::WindowRenderPass>(window());

        init_pipeline();

        m_imgui_manager = std::make_unique<vke::ImguiManager>(core(), window(), m_renderpass.get(), 0);
        m_camera        = std::make_unique<Camera>(this);
        m_raytracer     = std::make_unique<Raytracer>(this);
        m_raytracer->set_camera(m_camera.get());
    }

    void on_frame(vke::CommandBuffer& cmd) override {
        m_camera->free_move();

        m_raytracer->raytarce(cmd);

        m_renderpass->begin(cmd);

        cmd.bind_pipeline(m_pipeline.get());
        cmd.bind_descriptor_set(0, create_descriptor_set());
        Push push{
            .color = {1.0, 0.2, 0.4},
        };

        cmd.draw(3, 1, 0, 0);

        // ImGui::ShowDemoWindow();

        m_imgui_manager->flush_frame(cmd);

        m_renderpass->end(cmd);
    }

    VkDescriptorSet create_descriptor_set() {
        vke::DescriptorSetBuilder builder;
        builder.add_image_sampler(m_raytracer->get_image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, core()->get_sampler_manager()->nearest_sampler(), VK_SHADER_STAGE_FRAGMENT_BIT);
        return builder.build(get_framely_pool(), m_pipeline->get_descriptor_layout(0));
    }

    void init_pipeline() {

        vke::GPipelineBuilder pipeline_builder(core());
        pipeline_builder.set_renderpass(m_renderpass.get(), 0);

        auto frag_code = read_file_binary("demos/raytrace/spirv/screen_quad.spv");
        auto vert_code = read_file_binary("demos/raytrace/spirv/post_process.spv");

        pipeline_builder.add_shader_stage(cast_u8_to_span_u32(frag_code));
        pipeline_builder.add_shader_stage(cast_u8_to_span_u32(vert_code));

        m_pipeline = pipeline_builder.build();
    }

private:
    std::unique_ptr<vke::Renderpass> m_renderpass;
    std::unique_ptr<vke::Pipeline> m_pipeline;
    std::unique_ptr<vke::ImguiManager> m_imgui_manager;
    std::unique_ptr<Raytracer> m_raytracer;
    std::unique_ptr<Camera> m_camera;
};

// https://en.wikipedia.org/wiki/Single-precision_floating-point_format
float fast_pow2(i32 a) {
    i32 val = (a + 127) << 23;
    return *reinterpret_cast<float*>(&val);
}



int main() {
    App engine;

    engine.run();
}
