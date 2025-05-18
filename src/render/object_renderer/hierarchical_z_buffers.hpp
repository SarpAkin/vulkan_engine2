#pragma once

#include <vke/fwd.hpp>
#include <vke/vke.hpp>

#include <memory>
#include <vector>

#include "common.hpp"
#include "fwd.hpp"

namespace vke {

class HierarchicalZBuffers : public Resource {
public:
    HierarchicalZBuffers(RenderServer* rd, IImageView* target);

    void update_mips(vke::CommandBuffer& cmd);

    vke::IImageView* get_mips() { return m_depth_chain.get(); }
    VkSampler get_sampler() { return m_shared_data->min_sampler; }

private:
    void get_or_create_shared_data();
    void create_sets();

private:
    struct SharedData {
        vke::RCResource<vke::IPipeline> mip_pipeline;
        VkSampler min_sampler;
        VkDescriptorSetLayout mip_chain_set_layout;
    };

    vke::RenderServer* m_render_server;
    std::unique_ptr<vke::Image> m_depth_chain;
    vke::IImageView* m_target_depth_buffer;

    // this is to avoid duplicating shareable data across HZB's
    std::shared_ptr<SharedData> m_shared_data;

    std::vector<VkDescriptorSet> m_mip_sets;
    std::vector<std::unique_ptr<vke::IImageView>> m_mip_views;

    bool m_are_images_new = false;
};

} // namespace vke