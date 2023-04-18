#pragma once

#include <vke/fwd.hpp>
#include <vke/vk_system.hpp>

struct RaytracerFrameData {
    std::unique_ptr<vke::Buffer> config_ubo;
};

class Raytracer : public vke::System<RaytracerFrameData> {
public:
    Raytracer(vke::RenderEngine* engine);

    void init_object_buffer();

    void raytarce(vke::CommandBuffer& cmd);

    vke::Image* get_image(){return m_output_image.get();};
private:
    vke::Window* m_window;

    std::unique_ptr<vke::Image> m_output_image;
    std::unique_ptr<vke::Pipeline> m_pipeline;
    std::unique_ptr<vke::Buffer> m_object_buffer;
};

