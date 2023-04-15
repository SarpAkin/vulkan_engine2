#include "image.hpp"
#include <cstring>
#include <type_traits>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "commandbuffer.hpp"
#include "core.hpp"
#include "vk_resource.hpp"
#include "vkutil.hpp"

namespace vke {

std::unique_ptr<Image> Image::load_png(CommandBuffer& cmd, const char* path) {
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
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
            .width  = static_cast<u32>(tex_width),
            .height = static_cast<u32>(tex_height),
            .layers = 1,
        });
    
    cmd.add_execution_dependency(std::move(stencil));

    return image;
}

} // namespace vke