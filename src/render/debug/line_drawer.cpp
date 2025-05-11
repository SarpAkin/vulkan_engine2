#include "line_drawer.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke.hpp>

#include "render/render_server.hpp"
#include "scene/camera.hpp"

namespace vke {

LineDrawer::LineDrawer(RenderServer* render_server) {
    m_render_server          = render_server;
    m_buffer_vertex_capacity = 8192;

    auto* pg_provider = m_render_server->get_pipeline_loader()->get_pipeline_globals_provider();

    vke::VertexInputDescriptionBuilder vi_builder;
    vi_builder.push_binding<DebugLineVertex>();
    vi_builder.push_attribute<float>(3);
    vi_builder.push_attribute<u32>(1);

    pg_provider->vertex_input_descriptions["vke::debug_line_vertex"] = std::make_unique<vke::VertexInputDescriptionBuilder>(std::move(vi_builder));
}

LineDrawer::~LineDrawer() {
}

void LineDrawer::draw_transformed_box(const glm::mat4& model, u32 color) {
    glm::vec3 vertices[] = {
        {0, 0, 0},
        {0, 0, 1},
        {0, 1, 0},
        {0, 1, 1},
        {1, 0, 0},
        {1, 0, 1},
        {1, 1, 0},
        {1, 1, 1},
    };

    for (auto& v : vertices) {
        glm::vec4 pos4 = model * glm::vec4(v, 1.0);
        v              = glm::vec3(pos4) / pos4.w;
    }

    draw_line(vertices[0], vertices[1], color);
    draw_line(vertices[1], vertices[3], color);
    draw_line(vertices[3], vertices[2], color);
    draw_line(vertices[2], vertices[0], color);

    draw_line(vertices[4], vertices[5], color);
    draw_line(vertices[5], vertices[7], color);
    draw_line(vertices[7], vertices[6], color);
    draw_line(vertices[6], vertices[4], color);

    draw_line(vertices[0], vertices[4], color);
    draw_line(vertices[1], vertices[5], color);
    draw_line(vertices[2], vertices[6], color);
    draw_line(vertices[3], vertices[7], color);
}

void LineDrawer::draw_line(glm::vec3 a, glm::vec3 b, u32 color) {
    auto& framely = m_framely[m_render_server->get_frame_index()];

    if (m_buffer_vertex_count + 2 > m_buffer_vertex_capacity) {
        m_buffer_index++;
        m_buffer_vertex_count = 0;
    }

    if (m_buffer_index >= framely.vertex_buffers.size()) {
        // m_buffer_index should not be greater than vertex buffer size no matter what
        assert(m_buffer_index == framely.vertex_buffers.size());

        framely.vertex_buffers.push_back(std::make_unique<vke::Buffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(DebugLineVertex) * m_buffer_vertex_capacity, true));
    }

    auto vb_data = framely.vertex_buffers[m_buffer_index]->mapped_data_as_span<DebugLineVertex>();

    vb_data[m_buffer_vertex_count + 0] = DebugLineVertex{a, color};
    vb_data[m_buffer_vertex_count + 1] = DebugLineVertex{b, color};

    m_buffer_vertex_count += 2;
}

void LineDrawer::flush(vke::CommandBuffer& cmd, const Camera* camera, const std::string& renderpass_name) {
    // lazily load the pipeline as renderpass is not initialized in the creation of this class
    if (!m_line_drawer_pipeline) {
        m_line_drawer_pipeline = m_render_server->get_pipeline_loader()->load("vke::debug_line_draw_pipeline");
    }

    // return if vertex buffers are empty
    if (m_buffer_vertex_count == 0 && m_buffer_index == 0) return;

    cmd.bind_pipeline(m_line_drawer_pipeline.get());

    struct Push {
        glm::mat4 proj_view;
    };

    Push push = {
        .proj_view = camera->proj_view(),
    };

    cmd.push_constant(&push);

    auto& vertex_buffers = m_framely[m_render_server->get_frame_index()].vertex_buffers;

    for (int i = 0; i <= m_buffer_index; i++) {
        cmd.bind_vertex_buffer({vertex_buffers[i].get()});
        cmd.draw(i == m_buffer_index ? m_buffer_vertex_count : m_buffer_vertex_capacity, 1, 0, 0);
    }

    // reset counters for next frame
    m_buffer_vertex_count = 0;
    m_buffer_index        = 0;
}

void LineDrawer::draw_camera_frustum(const glm::mat4& model, u32 color) {
    glm::mat4 m = glm::mat4(
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1 //
    );

    draw_transformed_box(model * m, color);
}
} // namespace vke