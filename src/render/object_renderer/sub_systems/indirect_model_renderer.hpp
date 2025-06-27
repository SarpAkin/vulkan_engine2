#pragma once

#include "common.hpp"
#include "fwd.hpp"
#include "render/object_renderer/iobject_renderer_system.hpp"

namespace vke {

class IndirectModelRenderer : public IObjectRendererSystem {
public:
    IndirectModelRenderer(ObjectRenderer* object_renderer);
    ~IndirectModelRenderer();

private:
    struct IndirectRenderBuffers;

public:
    void register_render_target(const std::string& render_target_name) override;
    void render(RenderArguments&) override;
    void update(vke::CommandBuffer& cmd) override;
    void set_world(flecs::world* reg) override;

private:
    void create_descriptor_set_for_irb(IndirectRenderBuffers& irb);
    void create_irb_set_layout();
    void initialize_irb(IndirectRenderBuffers& irb);
    void initialize_scene_data();
    void initialize_pipelines();

private:
    struct IndirectRenderBuffers {
        std::unique_ptr<vke::Buffer> indirect_draw_buffer;
        // stores indirect draw commands

        // this buffer stores indexes to parts indirect draw arguments
        std::unique_ptr<vke::Buffer> part2indirect_draw_location[FRAME_OVERLAP];

        // it is a an array of uvec2. their indices correspond to their part id
        // it stores draw offsets & draw max_counts in x & y components respectively
        std::unique_ptr<vke::Buffer> instance_draw_parameter_location_buffer[FRAME_OVERLAP];
        std::unique_ptr<vke::Buffer> instance_count_buffer;

        std::unique_ptr<vke::Buffer> host_instance_count_buffers[FRAME_OVERLAP];

        std::unique_ptr<vke::GrowableBuffer> instance_draw_parameters;

        VkDescriptorSet indirect_render_sets[2];
    };

private:
    ObjectRenderer* m_object_renderer = nullptr;
    RenderServer* m_render_server     = nullptr;

    std::unique_ptr<SceneBuffersManager> m_scene_data;
    std::unordered_map<std::string, IndirectRenderBuffers> m_indirect_render_buffers;
    // render system set layout
    VkDescriptorSetLayout m_indirect_render_set_layout = VK_NULL_HANDLE;

    RCResource<vke::IPipeline> m_cull_pipeline;
    RCResource<vke::IPipeline> m_indirect_draw_command_gen_pipeline;

    bool m_query_indirect_render_counters = true;
};

} // namespace vke