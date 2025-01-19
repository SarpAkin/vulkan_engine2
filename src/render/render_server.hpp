#pragma once

#include <memory>

#include <vke/fwd.hpp>
#include <vke/vke.hpp>

#include "entity/Entity.hpp"
#include "fwd.hpp"

#include "common.hpp" // IWYU pragma: export

namespace vke {
class RenderServer : protected vke::DeviceGetter {
public:
    RenderServer();
    ~RenderServer();

    void init();
    void run();

    IPipelineLoader* get_pipeline_loader() { return m_pipeline_loader.get(); }


private:
    void render(vke::CommandBuffer& cmd);

private:
    std::unique_ptr<vke::Window> m_window;
    std::unique_ptr<vke::Renderpass> m_window_renderpass;
    std::unique_ptr<vke::IPipelineLoader> m_pipeline_loader;
    std::unique_ptr<vke::Camera> m_camera;
    std::unique_ptr<vke::MeshRenderer> m_mesh_renderer;
    std::unique_ptr<vke::SceneData> m_scene_data;

    bool m_running = true;
    int m_frame_index = 0;
    float m_delta_time = NAN;

    struct FramelyData{
        std::unique_ptr<vke::CommandBuffer> cmd;
        std::unique_ptr<vke::Fence> fence;
    };

    std::vector<FramelyData> m_framely_data;

private://tmp stuff
    void populate_entities();

private:
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::unique_ptr<Mesh> m_cube_mesh;
};

} // namespace vke