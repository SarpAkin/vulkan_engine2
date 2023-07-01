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
#include <vke/material_manager.hpp>
#include <vke/pipeline.hpp>
#include <vke/pipeline_builder.hpp>
#include <vke/renderpass.hpp>
#include <vke/util.hpp>
#include <vke/vertex_input_builder.hpp>
#include <vke/window_sdl.hpp>

#include <vke/material_loader/material_loader.hpp>

struct Vertex {
    f32 pos[2];
    f32 color[3];
};

class App : public vke::RenderEngine {
public:
    App() {
        m_renderpass = std::make_unique<vke::WindowRenderPass>(window());
    
        init_vertex_data();
    }

    void on_frame(vke::CommandBuffer& cmd) override {
        if (!m_default_material) init_material(cmd);

        m_renderpass->begin(cmd);

        cmd.bind_material(m_default_material);
        cmd.bind_vertex_buffer({m_vertex_buffer.get()});
        cmd.draw(3, 1, 0, 0);

        m_renderpass->end(cmd);
    }

    void init_material(vke::CommandBuffer& cmd) {
        auto* mat_man = get_material_manager();
        mat_man->register_render_target("default", m_renderpass.get(), 0);

        vke::VertexInputDescriptionBuilder builder;
        builder.push_binding<Vertex>();
        builder.push_attribute<f32>(2);
        builder.push_attribute<f32>(3);

        mat_man->register_vertex_input("shader1", std::move(builder));

        vke::MaterialLoader loader(mat_man);
        loader.load_material_file(cmd, "demos/2/res/materials.json");

        m_default_material = mat_man->get_material("material1");
    }

    void init_vertex_data() {
        Vertex verticies[] = {
            Vertex{.pos = {-0.5, -0.5}, .color = {1.0, 0.0, 0.0}},
            Vertex{.pos = {+0.5, -0.5}, .color = {0.0, 1.0, 0.0}},
            Vertex{.pos = {-0.0, +0.5}, .color = {0.0, 0.0, 1.0}},
        };

        m_vertex_buffer = core()->create_buffer_from_data(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, std::span(verticies));
    }

private:
    std::unique_ptr<vke::Renderpass> m_renderpass;
    std::unique_ptr<vke::Buffer> m_vertex_buffer;
    vke::Material* m_default_material = nullptr;
};

int main() {
    App engine;

    engine.run();
}