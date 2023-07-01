#include "image.hpp"

#include <atomic>
#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "commandbuffer.hpp"
#include "core.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"

namespace vke {

Image::Image(const ImageArgs& args) : Resource(args.core) {
    m_width  = args.width;
    m_height = args.height;
    m_format = args.format;

    VkImageCreateInfo ic_info{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = args.format,
        .extent        = {.width = args.width, .height = args.height, .depth = 1},
        .mipLevels     = 1,
        .arrayLayers   = args.layers,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = args.host_visible ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL,
        .usage         = args.usage_flags,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo dimg_allocinfo = {
        .flags = args.host_visible ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT : 0u,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VK_CHECK(vmaCreateImage(core()->gpu_allocator(), &ic_info, &dimg_allocinfo, &m_image, &m_allocation, nullptr));
    vmaSetAllocationName(core()->gpu_allocator(), m_allocation, "image");

    // VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT

    VkImageViewCreateInfo ivc_info = {
        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image    = m_image,
        .viewType = args.layers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        .format   = args.format,

        .subresourceRange = {
            .aspectMask     = is_depth_format(args.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = args.layers,
        },
    };

    VK_CHECK(vkCreateImageView(device(), &ivc_info, nullptr, &m_view));

    if (args.host_visible) {
        VK_CHECK(vmaMapMemory(core()->gpu_allocator(), m_allocation, &m_mapped_data));
    }

#ifndef NDEBUG
    core()->image_counter++;
#endif
}

std::unique_ptr<Image> Core::create_image(ImageArgs args) {
    args.core = this;
    return std::make_unique<Image>(args);
}

Image::~Image() {
#ifndef NDEBUG
    core()->image_counter--;
#endif

    if (m_mapped_data) vmaUnmapMemory(core()->gpu_allocator(), m_allocation);

    vkDestroyImageView(device(), m_view, nullptr);
    vmaDestroyImage(core()->gpu_allocator(), m_image, m_allocation);
}

std::unique_ptr<Image> Image::buffer_to_image(CommandBuffer& cmd, IBufferSpan* buffer, const ImageArgs& _args) {
    ImageArgs args = _args;
    args.usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    args.core  = cmd.core();
    auto image = std::make_unique<Image>(args);

    VkImageMemoryBarrier image_transfer_barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask       = 0,
        .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = cmd.core()->queue_family(),
        .dstQueueFamilyIndex = cmd.core()->queue_family(),
        .image               = image->handle(),
        .subresourceRange    = VkImageSubresourceRange{
               .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
               .baseMipLevel   = 0,
               .levelCount     = 1,
               .baseArrayLayer = 0,
               .layerCount     = 1,
        },
    };

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .image_memory_barriers = std::span(&image_transfer_barrier, 1),
    });

    VkBufferImageCopy copy_region = {
        .bufferOffset      = 0,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = VkImageSubresourceLayers{
             .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
             .mipLevel       = 0,
             .baseArrayLayer = 0,
             .layerCount     = 1,
        },
        .imageExtent = VkExtent3D{
            .width  = static_cast<u32>(args.width),
            .height = static_cast<u32>(args.height),
            .depth  = 1,
        },
    };

    // copy the buffer into the image
    vkCmdCopyBufferToImage(cmd.handle(), buffer->handle(), image->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

    VkImageMemoryBarrier image_readable_barrier = image_transfer_barrier;
    image_readable_barrier.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_readable_barrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_readable_barrier.srcAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_readable_barrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .image_memory_barriers = std::span(&image_readable_barrier, 1),
    });

    return image;
}

} // namespace vke