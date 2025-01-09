#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include <vke/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>


namespace vke {

struct Mesh{
    struct Vertex{
        glm::vec3 pos;
        u32 color;
        
    };

    std::unique_ptr<vke::Buffer> vertex_buffer,index_buffer;
    VkIndexType index_type;
    uint32_t index_count;

public:
};


}