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

        init_descriptor_sets();
        init_pipeline();
        init_vertex_data();

        m_imgui_manager = std::make_unique<vke::ImguiManager>(core(), window(), m_renderpass.get(), 0);
    }

    void on_frame(vke::CommandBuffer& cmd) override {
        if (m_image == nullptr) {
            load_image(cmd);
        }
        m_renderpass->begin(cmd);

        cmd.bind_pipeline(m_pipeline.get());
        cmd.bind_descriptor_set(0, m_dset);
        cmd.bind_vertex_buffer({m_vertex_buffer.get()});
        Push push{
            .color = {1.0, 0.2, 0.4},
        };

        cmd.push_constant(&push);
        cmd.draw(3, 1, 0, 0);

        ImGui::ShowDemoWindow();

        m_imgui_manager->flush_frame(cmd);

        m_renderpass->end(cmd);
    }

    void init_pipeline() {
        vke::VertexInputDescriptionBuilder vertex_builder;
        vertex_builder.push_binding<Vertex>();
        vertex_builder.push_attribute<f32>(2);
        vertex_builder.push_attribute<f32>(3);

        vke::GPipelineBuilder pipeline_builder(core());
        pipeline_builder.set_renderpass(m_renderpass.get(), 0);
        pipeline_builder.set_vertex_input(&vertex_builder);

        auto frag_code = read_file_binary("res/spv/1.frag.spv");
        auto vert_code = read_file_binary("res/spv/1.vert.spv");

        pipeline_builder.add_shader_stage(cast_u8_to_span_u32(frag_code));
        pipeline_builder.add_shader_stage(cast_u8_to_span_u32(vert_code));

        m_pipeline = pipeline_builder.build();
    }

    void init_vertex_data() {
        Vertex verticies[] = {
            Vertex{.pos = {-0.5, -0.5}, .color = {1.0, 0.0, 0.0}},
            Vertex{.pos = {+0.5, -0.5}, .color = {0.0, 1.0, 0.0}},
            Vertex{.pos = {-0.0, +0.5}, .color = {0.0, 0.0, 1.0}},
        };

        m_vertex_buffer = core()->create_buffer_from_data(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, std::span(verticies));
    }

    void init_descriptor_sets() {
        m_dpool = std::make_unique<vke::DescriptorPool>(core());

        vke::DescriptorSetLayoutBuilder builder;
        builder.add_image_sampler(VK_SHADER_STAGE_FRAGMENT_BIT);
        m_dset_layout = builder.build(core());
    }

    void load_image(vke::CommandBuffer& cmd) {
        m_image = vke::Image::load_png(cmd, "res/grid.png");

        vke::DescriptorSetBuilder builder;
        builder.add_image_sampler(m_image.get(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, core()->get_sampler_manager()->default_sampler(), VK_SHADER_STAGE_FRAGMENT_BIT);
        m_dset = builder.build(m_dpool.get(), m_dset_layout);
    }

private:
    std::unique_ptr<vke::Renderpass> m_renderpass;
    std::unique_ptr<vke::Pipeline> m_pipeline;
    std::unique_ptr<vke::Buffer> m_vertex_buffer;
    std::unique_ptr<vke::DescriptorPool> m_dpool;
    std::unique_ptr<vke::Image> m_image;
    std::unique_ptr<vke::ImguiManager> m_imgui_manager;
    VkDescriptorSetLayout m_dset_layout = nullptr;
    VkDescriptorSet m_dset              = nullptr;
};

int main() {

    App engine;

    engine.run();
}