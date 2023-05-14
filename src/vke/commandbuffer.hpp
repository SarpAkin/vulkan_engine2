#pragma once

#include <initializer_list>
#include <memory>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
#include "vk_resource.hpp"

namespace vke {

struct PipelineBarrierArgs {
    VkPipelineStageFlags src_stage_mask, dst_stage_mask;
    VkDependencyFlags dependency_flags                      = 0;
    std::span<VkMemoryBarrier> memory_barriers              = std::span((VkMemoryBarrier*)nullptr, 0);
    std::span<VkBufferMemoryBarrier> buffer_memory_barriers = std::span((VkBufferMemoryBarrier*)nullptr, 0);
    std::span<VkImageMemoryBarrier> image_memory_barriers   = std::span((VkImageMemoryBarrier*)nullptr, 0);
};

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

    void cmd_begin_renderpass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
    void cmd_next_subpass(VkSubpassContents contents);
    void cmd_end_renderpass();

    // VkCmd* wrappers
    void begin_renderpass(Renderpass* renderpass, VkSubpassContents contents);
    void next_subpass(VkSubpassContents contents) { cmd_next_subpass(contents); };
    void end_renderpass() { cmd_end_renderpass(); }

    //definition at material_manager.cpp
    void bind_material(Material* material);

    void bind_pipeline(Pipeline* pipeline);
    void bind_vertex_buffer(const std::initializer_list<Buffer*>& buffer);
    void bind_descriptor_set(u32 index, VkDescriptorSet set);
    void push_constant(u32 size, const void* pValues);
    template <typename T>
    void push_constant(const T* push) { push_constant(sizeof(T), push); }

    void draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance);

    void dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z);

    void pipeline_barrier(const PipelineBarrierArgs& args);

private:
    VkCommandBuffer m_cmd;
    VkCommandPool m_cmd_pool;

    std::vector<std::unique_ptr<Resource>> m_dependent_resources;

    VkPipelineBindPoint m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
    Pipeline* m_current_pipeline                 = nullptr;
};

} // namespace vke
