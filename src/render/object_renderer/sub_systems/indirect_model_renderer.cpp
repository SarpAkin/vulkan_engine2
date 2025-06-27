#include "indirect_model_renderer.hpp"

#include <vke/pipeline_loader.hpp>
#include <vke/vke_builders.hpp>

#include "render/object_renderer/render_state.hpp"
#include "render/render_server.hpp"

#include "render/object_renderer/object_renderer.hpp"
#include "render/object_renderer/resource_manager.hpp"
#include "render/object_renderer/scene_buffers_manager.hpp"

#include "render/debug/gpu_timing_system.hpp"
#include "render/shader/scene_data.h"

namespace vke {

IndirectModelRenderer::IndirectModelRenderer(ObjectRenderer* object_renderer) {
    m_object_renderer = object_renderer;
    m_render_server   = object_renderer->get_render_server();

    create_irb_set_layout(); // must be the first one to be created as it provides the scene set layout
    initialize_scene_data();
    initialize_pipelines();
}
IndirectModelRenderer::~IndirectModelRenderer() {}

void IndirectModelRenderer::register_render_target(const std::string& render_target_name) {
    initialize_irb(m_indirect_render_buffers[render_target_name]);
}

void IndirectModelRenderer::initialize_scene_data() {
    assert(m_scene_data == nullptr);

    m_scene_data = std::make_unique<SceneBuffersManager>(m_render_server, m_object_renderer->get_resource_manager());
}

void IndirectModelRenderer::render(RenderArguments& args) {
    bool mesh_shaders_enabled = false;

    auto ctx                        = VulkanContext::get_context();
    auto* timer                     = m_render_server->get_gpu_timing_system();
    auto* resource_manager          = m_object_renderer->get_resource_manager();
    auto& cmd                       = *args.subpass_cmd;
    auto& compute_cmd               = *args.compute_cmd;
    const RenderTargetInfo* rd_info = m_object_renderer->get_render_target_info(args.render_target_name);

    auto bind_state = resource_manager->create_bindstate(cmd, rd_info);

    timer->timestamp(cmd, std::format("rendering start for render target: {}", args.render_target_name), VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

    struct PartEntry {
        u32 part_id;
        MeshID mesh_id;
    };

    std::unordered_map<MaterialID, SmallVec<PartEntry>> material_part_ids;
    material_part_ids.reserve(100);

    auto* draw_data = &m_indirect_render_buffers.at(args.render_target_name);

    u32 total_instance_counter   = 0;
    auto allocate_instance_space = [&](u32 instance_count) {
        u32 index = total_instance_counter;
        total_instance_counter += instance_count;
        return glm::uvec2(index, instance_count);
    };

    auto instance_offsets = draw_data->instance_draw_parameter_location_buffer[m_render_server->get_frame_index()]->mapped_data_as_span<glm::uvec2>();

    for (auto& [model_id, instance_count] : m_scene_data->get_model_instance_counters()) {
        assert(instance_count > 0);
        if (instance_count <= 0) continue;

        auto* model = resource_manager->get_model(model_id);

        if (model == nullptr) {
            LOG_WARNING("couldn't find model with the id %d. skipping it\n", model_id.id);
            continue;
        }

        auto& model_parts = m_scene_data->get_model_part_sub_allocations().at(model_id);

        for (u32 i = 0; i < model->parts.size(); i++) {
            auto& part = model->parts[i];

            u32 part_id = model_parts.offset + i;

            material_part_ids[part.material_id].push_back({
                .part_id = part_id,
                .mesh_id = part.mesh_id,
            });

            instance_offsets[part_id] = allocate_instance_space(instance_count);
        }
    }

    struct Push {
        mat4 pad[2];
        uint32_t mode;
    };

    Push push = {
        .mode = 1,
    };

    //bind the sets for the subpass cmd
    cmd.bind_descriptor_set(rd_info->set_indices.view_set, rd_info->view_sets[m_render_server->get_frame_index()]);
    cmd.bind_descriptor_set(rd_info->set_indices.render_system_set, draw_data->indirect_render_sets[m_render_server->get_frame_index()]);

    auto indirect_draw_location = draw_data->part2indirect_draw_location[m_render_server->get_frame_index()]->mapped_data<u32>();
    for (auto& n : indirect_draw_location.subspan(0, m_scene_data->get_part_max_id())) {
        n = 0xFFFF'FFFF;
    }

    auto indirect_draw_buffer = draw_data->indirect_draw_buffer.get();

    u32 total_indirect_draws = 0;
    for (auto& [material_id, parts] : material_part_ids) {
        resource_manager->bind_material(&bind_state, material_id);

        cmd.push_constant(&push);

        for (u32 i = 0, parts_size = parts.size(); i < parts_size; i++) {
            auto& part              = parts[i];
            u32 part_id             = part.part_id;
            u32 indirect_draw_index = total_indirect_draws + i;

            resource_manager->bind_mesh(&bind_state, part.mesh_id);

            indirect_draw_location[part_id] = indirect_draw_index;

            cmd.draw_indexed_indirect(indirect_draw_buffer->subspan_item<VkDrawIndexedIndirectCommand>(indirect_draw_index, 1), 1);
        }

        total_indirect_draws += parts.size();
        assert(total_indirect_draws <= indirect_draw_capacity);
    }

    timer->timestamp(cmd, std::format("rendering end for render target: {}", args.render_target_name), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

#pragma region indirect_cull
    timer->timestamp(compute_cmd, std::format("cull start for render target: {}", args.render_target_name), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    compute_cmd.fill_buffer(*draw_data->instance_count_buffer, 0);

    VkBufferMemoryBarrier buffer_barriers0[] = {
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .buffer        = draw_data->instance_count_buffer->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
    };

    compute_cmd.pipeline_barrier({
        .src_stage_mask         = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dst_stage_mask         = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .buffer_memory_barriers = buffer_barriers0,
    });

    compute_cmd.bind_pipeline(m_cull_pipeline.get());

    //bind the sets for the compute cmd
    compute_cmd.bind_descriptor_set(rd_info->set_indices.view_set, rd_info->view_sets[m_render_server->get_frame_index()]);
    compute_cmd.bind_descriptor_set(rd_info->set_indices.render_system_set, draw_data->indirect_render_sets[m_render_server->get_frame_index()]);

    if (total_instance_counter > draw_data->instance_draw_parameters->item_size<InstanceDrawParameter>()) {
        draw_data->instance_draw_parameters->resize(total_instance_counter * sizeof(InstanceDrawParameter));
    }

    // assert(total_instance_counter <= instance_capacity);
    compute_cmd.push_constant(&total_instance_counter);
    compute_cmd.dispatch(calculate_dispatch_size(total_instance_counter, 128), 1, 1);

    VkBufferMemoryBarrier buffer_barriers[] = {
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .buffer        = draw_data->instance_count_buffer->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
    };

    compute_cmd.pipeline_barrier({
        .src_stage_mask         = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dst_stage_mask         = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .buffer_memory_barriers = buffer_barriers,
    });

    u32 part_count = m_scene_data->get_part_max_id();
    compute_cmd.push_constant(&part_count);

    compute_cmd.bind_pipeline(m_indirect_draw_command_gen_pipeline.get());
    compute_cmd.dispatch(calculate_dispatch_size(part_count, 128), 1, 1);

    VkBufferMemoryBarrier buffer_barriers2[] = {
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
            .buffer        = draw_data->instance_draw_parameters->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier{
            .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
            .buffer        = draw_data->indirect_draw_buffer->handle(),
            .offset        = 0,
            .size          = VK_WHOLE_SIZE,
        },
    };

    compute_cmd.pipeline_barrier({
        .src_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .dst_stage_mask = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT //
                          | (mesh_shaders_enabled ? VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT : 0u)     //
                          | (m_query_indirect_render_counters ? VK_PIPELINE_STAGE_TRANSFER_BIT : 0u),
        .buffer_memory_barriers = buffer_barriers2,
    });

    if (m_query_indirect_render_counters) {
        compute_cmd.copy_buffer(draw_data->instance_count_buffer->subspan(0), draw_data->host_instance_count_buffers[m_render_server->get_frame_index()]->subspan(0));
    }

    timer->timestamp(compute_cmd, std::format("cull end for render target: {}", args.render_target_name), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void IndirectModelRenderer::update(vke::CommandBuffer& cmd) {
    m_scene_data->updates_for_indirect_render(cmd);
}

void IndirectModelRenderer::create_irb_set_layout() {
    vke::DescriptorSetLayoutBuilder builder;

    builder.add_ssbo(VK_SHADER_STAGE_ALL);
    builder.add_ssbo(VK_SHADER_STAGE_ALL);

    builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT);
    builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT);
    builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT);
    builder.add_ssbo(VK_SHADER_STAGE_COMPUTE_BIT);

    builder.add_ssbo(VK_SHADER_STAGE_ALL);
    builder.add_ssbo(VK_SHADER_STAGE_ALL);
    builder.add_ssbo(VK_SHADER_STAGE_ALL);

    m_indirect_render_set_layout = builder.build();

    m_render_server->get_pipeline_loader()->get_pipeline_globals_provider()->set_layouts["vke::indirect_scene_set_layout"] = m_indirect_render_set_layout;
}

void IndirectModelRenderer::create_descriptor_set_for_irb(IndirectRenderBuffers& render_buffers) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        vke::DescriptorSetBuilder builder;

        builder.add_ssbo(m_scene_data->get_instance_data_buffer(), VK_SHADER_STAGE_ALL);
        builder.add_ssbo(render_buffers.instance_draw_parameters.get(), VK_SHADER_STAGE_ALL);                           // instance_draw_parameters
        builder.add_ssbo(render_buffers.instance_draw_parameter_location_buffer[i].get(), VK_SHADER_STAGE_COMPUTE_BIT); // instance_draw_parameter_locations
        builder.add_ssbo(render_buffers.instance_count_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT);                      // instance_counters
        builder.add_ssbo(render_buffers.part2indirect_draw_location[i].get(), VK_SHADER_STAGE_COMPUTE_BIT);             // indirect_draw_locations
        builder.add_ssbo(render_buffers.indirect_draw_buffer.get(), VK_SHADER_STAGE_COMPUTE_BIT);                       // draw_commands

        builder.add_ssbo(m_scene_data->get_model_info_buffer(), VK_SHADER_STAGE_ALL);
        builder.add_ssbo(m_scene_data->get_model_part_info_buffer(), VK_SHADER_STAGE_ALL);
        builder.add_ssbo(m_scene_data->get_mesh_info_buffer(), VK_SHADER_STAGE_ALL);

        render_buffers.indirect_render_sets[i] = builder.build(m_object_renderer->get_render_server()->get_descriptor_pool(), m_indirect_render_set_layout);
    }
}

void IndirectModelRenderer::initialize_irb(IndirectRenderBuffers& irb) {
    auto usage                   = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    irb.indirect_draw_buffer     = std::make_unique<vke::Buffer>(usage | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, sizeof(VkDrawIndexedIndirectCommand) * indirect_draw_capacity, false);
    irb.instance_count_buffer    = std::make_unique<vke::Buffer>(usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeof(u32) * part_capacity, false);
    irb.instance_draw_parameters = std::make_unique<vke::GrowableBuffer>(usage, sizeof(InstanceDrawParameter) * instance_capacity, false);

    vke::set_array(irb.part2indirect_draw_location, [&] {
        return std::make_unique<vke::Buffer>(usage, sizeof(u32) * part_capacity, true);
    });
    vke::set_array(irb.instance_draw_parameter_location_buffer, [&] {
        return std::make_unique<vke::Buffer>(usage, sizeof(glm::uvec2) * part_capacity, true);
    });
    vke::set_array(irb.host_instance_count_buffers, [&] {
        return std::make_unique<vke::Buffer>(usage, sizeof(uint) * part_capacity, true);
    });

    create_descriptor_set_for_irb(irb);
}
void IndirectModelRenderer::initialize_pipelines() {
    auto* pipeline_loader  = m_render_server->get_pipeline_loader();
    auto* resource_manager = m_object_renderer->get_resource_manager();

    m_cull_pipeline                      = pipeline_loader->load("vke::object_renderer::cull_shader");
    m_indirect_draw_command_gen_pipeline = pipeline_loader->load("vke::object_renderer::indirect_draw_gen");

    std::string pipelines[] = {"vke::default"};
    resource_manager->create_multi_target_pipeline(ObjectRenderer::pbr_pipeline_name, pipelines);
}

void IndirectModelRenderer::set_world(flecs::world* reg) {
    m_scene_data->set_world(reg);
};
} // namespace vke