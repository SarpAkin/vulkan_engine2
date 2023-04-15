#pragma once

#include <memory>
#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "fwd.hpp"
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
    bool host_visible = false;
};

class Image : public Resource {
public:
    Image(const ImageArgs& args);
    ~Image();

    VkImage handle() const { return m_image; }
    VkImageView view() const { return m_view; }

    static std::unique_ptr<Image> buffer_to_image(CommandBuffer& cmd, IBufferSpan* buffer, const ImageArgs& args);
    // image_load.cpp
    static std::unique_ptr<Image> load_png(CommandBuffer& cmd, const char* path);

private:
    VkImage m_image;
    VkImageView m_view;
    VmaAllocation m_allocation;
    void* m_mapped_data = nullptr;
};

} // namespace vke