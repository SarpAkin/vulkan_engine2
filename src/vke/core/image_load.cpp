#include "image.hpp"
#include <cstring>
#include <type_traits>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "commandbuffer.hpp"
#include "core.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"

namespace vke {

static void generate_miplevels(CommandBuffer& cmd, vke::Image* image) {
    int mip_width  = image->width();
    int mip_height = image->height();

    for (uint32_t i = 1; i < image->miplevel_count(); i++) {
        VkImageMemoryBarrier barriers[] = {
            VkImageMemoryBarrier{
                .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .image            = image->handle(),
                .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = i - 1,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = image->layer_count(),
                },
            },
            VkImageMemoryBarrier{
                .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .image            = image->handle(),
                .subresourceRange = {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = i,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = image->layer_count(),
                },
            },
        };

        cmd.pipeline_barrier(PipelineBarrierArgs{
            .src_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .dst_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .image_memory_barriers = barriers,
        });

        VkImageBlit blit{
            .srcSubresource = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = i - 1,
                .baseArrayLayer = 0,
                .layerCount     = image->layer_count(),
            },
            .srcOffsets = {
                {0, 0, 0},
                {mip_width, mip_height, 1},
            },
            .dstSubresource = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = i,
                .baseArrayLayer = 0,
                .layerCount     = image->layer_count(),
            },
            .dstOffsets = {
                {0, 0, 0},
                {std::max(mip_width / 2, 1), std::max(mip_height / 2, 1), 1},
            },
        };

        vkCmdBlitImage(cmd.handle(), image->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        mip_width  = std::max(mip_width / 2, 1);
        mip_height = std::max(mip_height / 2, 1);
    }

    VkImageMemoryBarrier barriers[] = {
        VkImageMemoryBarrier{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image            = image->handle(),
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = image->miplevel_count() - 1,
                .baseArrayLayer = 0,
                .layerCount     = image->layer_count(),
            },
        },
        VkImageMemoryBarrier{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image            = image->handle(),
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = image->miplevel_count() - 1,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = image->layer_count(),
            },
        },
    };

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .image_memory_barriers = barriers,
    });
}

std::unique_ptr<Image> Image::load_png(CommandBuffer& cmd, const char* path, u32 mip_levels) {
    auto core = cmd.core();

    int tex_width, tex_height, tex_channels;

    stbi_uc* pixels = stbi_load(path, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error(fmt::format("failed to load file: {}", path));
    }

    size_t buf_size = tex_width * tex_height * 4;

    // might leak pixels if an error is thrown
    auto stencil = std::make_unique<Buffer>(core, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, buf_size, true);
    memcpy(stencil->mapped_data_ptr(), pixels, buf_size);

    stbi_image_free(pixels);

    auto image = Image::buffer_to_image(cmd, stencil.get(),
        ImageArgs{
            .format      = VK_FORMAT_R8G8B8A8_SRGB,
            .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .width       = static_cast<u32>(tex_width),
            .height      = static_cast<u32>(tex_height),
            .layers      = 1,
            .mip_levels  = mip_levels,

        },
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    cmd.add_execution_dependency(std::move(stencil));

    generate_miplevels(cmd, image.get());

    return image;
}

void Image::save_as_png(const char* path) {
    int comp = 0;
    switch (m_format) {
    case VK_FORMAT_R8G8B8A8_SRGB:
        comp = 4;
        break;
    case VK_FORMAT_R8G8B8_SRGB:
        comp = 3;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
        comp = 4;
        break;
    default:
        throw std::runtime_error("unsupported format");
        break;
    }

    if (m_mapped_data) {
        stbi_write_png(path, m_height, m_height, comp, m_mapped_data, 4 * m_width);
        return;
    }

    auto buffer = core()->create_buffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, m_width * m_height * 4, true);

    core()->immediate_submit([&](CommandBuffer& cmd) {
        VkImageMemoryBarrier image_transfer_barrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = core()->queue_family(),
            .dstQueueFamilyIndex = core()->queue_family(),
            .image               = handle(),
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
                .width  = m_width,
                .height = m_height,
                .depth  = 1,
            },
        };

        vkCmdCopyImageToBuffer(cmd.handle(), handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->handle(), 1, &copy_region);
    });

    stbi_write_png(path, m_height, m_height, comp, buffer->mapped_data_ptr(), 4);
}

} // namespace vke