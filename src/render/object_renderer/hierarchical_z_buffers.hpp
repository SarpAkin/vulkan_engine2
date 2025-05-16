#pragma once

#include <vke/fwd.hpp>
#include <vke/vke.hpp>

#include <vector>
#include <memory>

#include "common.hpp"
#include "fwd.hpp"

namespace vke{

class HierarchicalZBuffers{
public:
    HierarchicalZBuffers(RenderServer* rd, IImageView* target);

    void update_mips(vke::CommandBuffer& cmd);

private:
    void get_or_create_shared_data();
    void create_sets();

private:
    struct SharedData{
        vke::RCResource<vke::IPipeline> mip_pipeline;
        VkSampler min_sampler;
        VkDescriptorSetLayout mip_chain_set_layout;
    };

    vke::RenderServer* m_render_server;
    std::unique_ptr<vke::Image> m_depth_chain;
    vke::IImageView* m_target_depth_buffer;

    //this is to avoid duplicating shareable data across HZB's
    std::shared_ptr<SharedData> m_shared_data;

    std::vector<VkDescriptorSet> m_mip_sets; 
    std::vector<std::unique_ptr<vke::IImageView>> m_mip_views;
};

}