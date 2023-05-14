#include "raytracer.hpp"

#include <vke/buffer.hpp>
#include <vke/commandbuffer.hpp>
#include <vke/descriptor_set_builder.hpp>
#include <vke/engine.hpp>
#include <vke/image.hpp>
#include <vke/pipeline_builder.hpp>
#include <vke/util.hpp>
#include <vke/window.hpp>

#include "camera.hpp"
#include "shaders/device_buffers.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Raytracer::Raytracer(vke::RenderEngine* engine) : vke::System<RaytracerFrameData>(engine) {
    m_window = engine->window();

    m_output_image = core()->create_image(vke::ImageArgs{
        .format       = VK_FORMAT_R8G8B8A8_UNORM,
        .usage_flags  = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .width        = m_window->width(),
        .height       = m_window->height(),
        .host_visible = false,
    });

    init_frame_datas([&](u32 index) {
        return RaytracerFrameData{
            .config_ubo = core()->create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(RayTracerConfigUBO), true),
        };
    });

    vke::CPipelineBuilder pipeline_builder(core());
    auto comp_code = read_file_binary("demos/raytrace/spirv/raytrace.spv");
    pipeline_builder.add_shader_stage(cast_u8_to_span_u32(comp_code));
    m_pipeline = pipeline_builder.build();

    init_object_buffer();
}

void Raytracer::init_object_buffer() {
    m_object_buffer             = core()->create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(ObjectBuffer), true);
    auto* object_buffer         = m_object_buffer->mapped_data_ptr<ObjectBuffer>();
    object_buffer->sphere_count = 6;
    object_buffer->box_count    = 1;

    object_buffer->spheres[0] = Sphere{
        .pos       = vec3(0.0, 0.0, 5.0),
        .radius    = 0.5f,
        .color     = vec3(0.0, 1.0, 0.0),
        .roughness = 0.4,
    };
    object_buffer->spheres[1] = Sphere{
        .pos       = vec3(1.5, 0.5, 4.0),
        .radius    = 0.5f,
        .color     = vec3(0.0, 1.0, 1.0),
        .roughness = 0.2,

    };
    object_buffer->spheres[2] = Sphere{
        .pos       = vec3(-1.5, -0.5, 4.0),
        .radius    = 1.0f,
        .color     = vec3(1.0, 1.0, 0.0),
        .roughness = 0.2,

    };
    object_buffer->spheres[3] = Sphere{
        .pos       = vec3(0.0, -6, 1.0),
        .radius    = 4.5f,
        .color     = vec3(1.0, 0.0, 0.7),
        .roughness = 0.0,
    };
    object_buffer->spheres[4] = Sphere{
        .pos       = vec3(1.0, 1, -5.0),
        .radius    = 3.5f,
        .color     = vec3(0.3, 0.3, 0.3),
        .roughness = 0.0,
    };
    object_buffer->boxes[0] = AABB{
        .pos         = vec3(0, 0, 0),
        .size        = vec3(5.0, 5.0, 5.0),
        .color       = vec3(0, 1, 0),
        .filled_bits = 0b1101'1011,
    };
}

void Raytracer::raytarce(vke::CommandBuffer& cmd) {
    float width  = m_window->width();
    float height = m_window->height();

    mat4 inv_proj_view = glm::inverse(m_camera->proj_view);

    auto& config_buffer = get_frame_data().config_ubo;
    auto* data          = config_buffer->mapped_data_ptr<RayTracerConfigUBO>();

    data->cam.tan_half_fovy = std::tan(glm::radians(m_camera->fov) / 2.0);
    data->cam.tan_half_fovx = data->cam.tan_half_fovy * m_camera->aspect_ratio;

    data->cam.right = glm::normalize(glm::cross(m_camera->dir, m_camera->up));
    data->cam.up    = -glm::normalize(glm::cross(data->cam.right, m_camera->dir)); // negative for the vulkan flipped y viewport

    data->cam.dir           = m_camera->dir;
    data->cam.pos           = m_camera->pos;
    data->inv_proj_view     = inv_proj_view;
    data->screen_pixel_size = vec2(width, height);
    data->sun_dir           = glm::normalize(vec3(0.2, -1.0, 0.0));
    data->sun_color         = vec3(0.8, 0.9, 0.5);
    data->ambient_lightning = 0.2;

    VkImageMemoryBarrier pre_barrier{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask    = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask    = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = m_output_image->handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    cmd.pipeline_barrier(vke::PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .image_memory_barriers = std::span(&pre_barrier, 1),
    });

    vke::DescriptorSetBuilder dset_builder;
    dset_builder.add_storage_image(m_output_image.get(), VK_IMAGE_LAYOUT_GENERAL, VK_SHADER_STAGE_COMPUTE_BIT);
    dset_builder.add_ssbo(m_object_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT);
    dset_builder.add_ubo(config_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT);
    VkDescriptorSet dset = dset_builder.build(get_framely_descriptor_pool(), m_pipeline->get_descriptor_layout(0));

    cmd.bind_pipeline(m_pipeline.get());
    cmd.bind_descriptor_set(0, dset);

    cmd.dispatch(width / SUB_GROUB_SIZE_XY, height / SUB_GROUB_SIZE_XY, 1);

    VkImageMemoryBarrier barrier{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask    = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask    = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image            = m_output_image->handle(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    cmd.pipeline_barrier(vke::PipelineBarrierArgs{
        .src_stage_mask        = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dst_stage_mask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .image_memory_barriers = std::span(&barrier, 1),
    });
}