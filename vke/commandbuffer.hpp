#pragma once

#include <initializer_list>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

class CommandBuffer : public Resource {
public:
    friend Fence;

    CommandBuffer(Core* core, bool is_primary);
    ~CommandBuffer();

    inline const VkCommandBuffer& handle() { return this->m_cmd; }

    inline void add_execution_dependency(std::unique_ptr<Resource> resource) { m_dependent_resources.push_back(std::move(resource)); }

    // vk stuff
    void begin();
    void end();
    void reset();

    // VkCmd* wrappers
    void cmd_begin_renderpass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
    void cmd_next_subpass(VkSubpassContents contents);
    void cmd_end_renderpass();

    void bind_pipeline(Pipeline* pipeline);
    void bind_vertex_buffer(const std::initializer_list<Buffer*>& buffer);
    void bind_descriptor_set(u32 index, VkDescriptorSet set);

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

private:
    VkCommandBuffer m_cmd;
    VkCommandPool m_cmd_pool;

    std::vector<std::unique_ptr<Resource>> m_dependent_resources;

    VkPipelineBindPoint m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
    Pipeline* m_current_pipeline                = nullptr;
};

} // namespace vke