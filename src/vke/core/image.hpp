#pragma once

#include <memory>
#include <vulkan/vulkan_core.h>

#include "../common.hpp"
#include "../fwd.hpp"
#include "vk_resource.hpp"

typedef struct VmaAllocation_T* VmaAllocation;

namespace vke {

struct ImageArgs {
    Core* core = nullptr;
    VkFormat format;
    VkImageUsageFlags usage_flags;
    u32 width;
    u32 height;
    u32 layers        = 1;
    u32 mip_levels    = 1;
    bool host_visible = false;
};

class Image : public Resource {
public:
    Image(const ImageArgs& args);
    ~Image();

    VkImage handle() const { return m_image; }
    VkImageView view() const { return m_view; }
    VkFormat format() const { return m_format; }

    static std::unique_ptr<Image> buffer_to_image(CommandBuffer& cmd, IBufferSpan* buffer, const ImageArgs& args);
    // image_load.cpp
    static std::unique_ptr<Image> load_png(CommandBuffer& cmd, const char* path, u32 mip_levels = 1);

    // blocking
    void save_as_png(const char* path);

    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    u32 layer_count() const { return m_num_layers; }
    u32 miplevel_count() const { return m_num_mipmaps; }

private:
    VkImage m_image;
    VkImageView m_view;
    VkFormat m_format;
    VmaAllocation m_allocation;
    void* m_mapped_data = nullptr;

    u32 m_width, m_height, m_num_layers, m_num_mipmaps;
};

} // namespace vke