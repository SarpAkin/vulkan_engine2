#include "commandbuffer.hpp"

#include <algorithm>
#include <alloca.h>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "core.hpp"
#include "fwd.hpp"
#include "pipeline.hpp"
#include "util.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"

namespace vke {

CommandBuffer::CommandBuffer(Core* core, bool is_primary) : Resource(core) {
    assert(is_primary == true && "secondry command buffer is not handled");

    VkCommandPoolCreateInfo p_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = core->queue_family(),
    };

    VK_CHECK(vkCreateCommandPool(device(), &p_info, nullptr, &m_cmd_pool));

    VkCommandBufferAllocateInfo alloc_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = m_cmd_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_CHECK(vkAllocateCommandBuffers(device(), &alloc_info, &m_cmd));
}

std::unique_ptr<CommandBuffer> Core::create_cmd(bool is_primary) {
    return std::make_unique<CommandBuffer>(this, is_primary);
}

CommandBuffer::~CommandBuffer() {
    vkDestroyCommandPool(device(), m_cmd_pool, nullptr);
}

void CommandBuffer::begin() {
    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    VK_CHECK(vkBeginCommandBuffer(m_cmd, &begin_info));
}

void CommandBuffer::end() {
    VK_CHECK(vkEndCommandBuffer(m_cmd));
}

void CommandBuffer::reset() {
    VK_CHECK(vkResetCommandBuffer(m_cmd, 0));
}

// VkCmd** wrappers
void CommandBuffer::cmd_begin_renderpass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    vkCmdBeginRenderPass(handle(), pRenderPassBegin, contents);
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void CommandBuffer::cmd_next_subpass(VkSubpassContents contents) {
    vkCmdNextSubpass(handle(), contents);
}

void CommandBuffer::cmd_end_renderpass() {
    vkCmdEndRenderPass(handle());
    m_current_pipeline_state = VK_PIPELINE_BIND_POINT_COMPUTE;
}

void CommandBuffer::bind_pipeline(Pipeline* pipeline) {
    m_current_pipeline = pipeline;

    vkCmdBindPipeline(handle(), m_current_pipeline_state, pipeline->handle());
}

void CommandBuffer::bind_vertex_buffer(const std::initializer_list<Buffer*>& buffer) {
    auto handles = MAP_VEC_ALLOCA(buffer, [](Buffer* buffer) { return buffer->handle(); });
    auto offsets = MAP_VEC_ALLOCA(buffer, [](Buffer*) { return (VkDeviceSize)0; });

    vkCmdBindVertexBuffers(handle(), 0, buffer.size(), handles.data(), offsets.data());
}

void CommandBuffer::bind_descriptor_set(u32 index, VkDescriptorSet set) {
    assert(m_current_pipeline != nullptr && "a pipeline must be bound first before binding a set");

    vkCmdBindDescriptorSets(handle(), m_current_pipeline_state, m_current_pipeline->layout(), index, 1, &set, 0, nullptr);
}

void CommandBuffer::push_constant(u32 size, const void* pValues) {
    assert(m_current_pipeline != nullptr && "a pipeline must be bound first before binding a set");

    vkCmdPushConstants(handle(), m_current_pipeline->layout(), m_current_pipeline->data().push_stages, 0, size, pValues);
}

// draw calls
void CommandBuffer::draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) {
    vkCmdDraw(handle(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z) {
    vkCmdDispatch(handle(), group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::pipeline_barrier(const PipelineBarrierArgs& args) {
    vkCmdPipelineBarrier(handle(),
        args.src_stage_mask, args.dst_stage_mask, args.dependency_flags,
        static_cast<uint32_t>(args.memory_barriers.size()), args.memory_barriers.data(),
        static_cast<uint32_t>(args.buffer_memory_barriers.size()), args.buffer_memory_barriers.data(),
        static_cast<uint32_t>(args.image_memory_barriers.size()), args.image_memory_barriers.data());
}

} // namespace vke
