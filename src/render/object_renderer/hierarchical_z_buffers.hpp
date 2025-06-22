#pragma once

#include <vke/fwd.hpp>
#include <vke/vke.hpp>

#include <memory>
#include <vector>

#include "common.hpp"
#include "fwd.hpp"
#include "glm/ext/matrix_float4x4.hpp"

namespace vke {

class HierarchicalZBuffers : public Resource {
public:
    HierarchicalZBuffers(RenderServer* rd, IImageView* target);
    ~HierarchicalZBuffers();

    void update_mips(vke::CommandBuffer& cmd);
    void update_hzb_proj_view(const glm::mat4& m) { m_hzb_proj_view = m; }

    glm::mat4 get_hzb_proj_view() const { return m_hzb_proj_view; }

    vke::IImageView* get_mips() { return m_depth_chain.get(); }
    VkSampler get_sampler() { return m_shared_data->cull_sampler; }

private:
    void get_or_create_shared_data();
    void create_sets();

private:
    struct SharedData {
        vke::RCResource<vke::IPipeline> mip_pipeline;
        VkSampler min_sampler;
        VkSampler cull_sampler;
        VkDescriptorSetLayout mip_chain_set_layout;
        vke::VulkanContext* context = nullptr;
        ~SharedData();
    };

    vke::RenderServer* m_render_server;
    std::unique_ptr<vke::Image> m_depth_chain;
    vke::IImageView* m_target_depth_buffer;

    // this is to avoid duplicating shareable data across HZB's
    std::shared_ptr<SharedData> m_shared_data;

    std::vector<VkDescriptorSet> m_mip_sets;
    std::vector<std::unique_ptr<vke::IImageView>> m_mip_views;


    glm::mat4 m_hzb_proj_view;

    bool m_are_images_new = false;
};

} // namespace vke