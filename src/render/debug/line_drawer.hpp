#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <vke/fwd.hpp>

#include "common.hpp"
#include "fwd.hpp"
#include "glm/ext/matrix_float4x4.hpp"

namespace vke {

struct DebugLineVertex {
    glm::vec3 pos;
    u32 color; // RGBA from most significant to least significant
};

class LineDrawer {
public:
    LineDrawer(RenderServer* render_server);
    ~LineDrawer();

public: // draw
    void draw_line(glm::vec3 a, glm::vec3 b, u32 color);

    void draw_transformed_box(const glm::mat4& model, u32 color);
    void draw_camera_frustum(const glm::mat4& inv_proj_view, u32 color);

public:
    void flush(vke::CommandBuffer& cmd,const Camera* camera,const std::string& renderpass);

private:

private:
    struct FramelyData {
        std::vector<std::unique_ptr<vke::Buffer>> vertex_buffers;
    };

private:
    std::unique_ptr<vke::IPipeline> m_line_drawer_pipeline;
    RenderServer* m_render_server;
    FramelyData m_framely[FRAME_OVERLAP];
    u32 m_buffer_vertex_count = 0, m_buffer_index = 0, m_buffer_vertex_capacity = 0;
};

} // namespace vke