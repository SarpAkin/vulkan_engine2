#include "scene_buffers_manager.hpp"

#include <entt/entt.hpp>

#include <flecs.h>
#include <flecs/addons/flecs_cpp.h>

#include "scene/components/components.hpp"

#include "render/shader/scene_data.h"
#include "resource_manager.hpp"

namespace vke {

SceneBuffersManager::SceneBuffersManager(RenderServer* render_server, ResourceManager* resource_manager) : m_model_part_buffer_sub_allocator(part_capacity) {
    m_render_server    = render_server;
    m_resource_manager = resource_manager;

    auto buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    m_model_info_buffer      = std::make_unique<vke::Buffer>(buffer_usage, sizeof(ModelData) * model_capacity, false);
    m_model_part_info_buffer = std::make_unique<vke::Buffer>(buffer_usage, sizeof(PartData) * part_capacity, false);
    m_material_info_buffer   = std::make_unique<vke::Buffer>(buffer_usage, sizeof(MaterialData) * material_capacity, false);
    m_instance_buffer        = std::make_unique<vke::GrowableBuffer>(buffer_usage, sizeof(InstanceData) * instance_capacity, false);
    m_mesh_info_buffer       = std::make_unique<vke::Buffer>(buffer_usage, sizeof(MeshData) * mesh_capacity, false);
}

SceneBuffersManager::~SceneBuffersManager() {
}

auto create_model_matrix_getter(flecs::world* registry) {
    return vke::make_y_combinator([=](auto&& self, flecs::entity e) -> glm::mat4 {
        auto transform = e.get<Transform>();
        if(transform){
            return transform->local_model_matrix();
        } 
        
        glm::mat4 model_mat;

        auto* relative_transform = e.get<RelativeTransform>();
        if(relative_transform){
            model_mat = relative_transform->get_model_matrix();
        }else{
            model_mat = glm::mat4(1);
        }

        auto parent = e.parent();

        if (parent.is_valid()) {
            model_mat = self(parent) * model_mat;
        }

        return model_mat;
    });
}

static glm::vec4 quat2vec4(const glm::quat& q) { return glm::vec4(q.x, q.y, q.z, q.w); }

void SceneBuffersManager::flush_pending_entities(vke::CommandBuffer& cmd, StencilBuffer& stencil) {
    auto model_matrix_getter = create_model_matrix_getter(m_world);

    auto instance_capacity = m_instance_buffer->item_size<InstanceData>();

    m_handle_manager->flush_and_register_handles([&](flecs::entity entity,InstanceHandleID instance_id){
        auto model_id   = entity.get<Renderable>()->model_id;
        m_model_instance_counters[model_id] += 1;

        auto model_matrix = model_matrix_getter(entity);
        auto t            = Transform::decompose_from_matrix(model_matrix);

        InstanceData instance_data = {
            .world_position = glm::dvec4(t.position, 0.0),
            .rotation       = glm::vec4(quat2vec4(t.rotation)),
            .size           = t.scale,
            .model_id       = model_id.id,
        };
        
        if (instance_id.id >= instance_capacity) {
            auto* instance_buffer = m_instance_buffer.get();
            instance_buffer->resize((instance_buffer->byte_size() * 3) / 2);
            instance_capacity = instance_buffer->item_size<InstanceData>();
        }

        stencil.copy_data(m_instance_buffer->subspan_item<InstanceData>(instance_id.id, 1), &instance_data, 1);
    });
}

void SceneBuffersManager::updates_for_indirect_render(vke::CommandBuffer& cmd) {
    assert(m_world != nullptr && "registry can not be null");
    
    auto& resource_updates = m_resource_manager->get_updated_resource();

    StencilBuffer stencil;

    for (auto model_id : resource_updates.model_updates) {
        auto* model = m_resource_manager->get_model(model_id);

        auto allocation = m_model_part_buffer_sub_allocator.allocate(model->parts.size()).value();

        m_model_part_sub_allocations[model_id] = allocation;

        ModelData model_data = {
            .aabb_half_size = model->boundary.half_size(),
            .part_index     = allocation.offset,
            .aabb_offset    = model->boundary.mip_point(),
            .part_count     = allocation.size,
        };

        stencil.copy_data(m_model_info_buffer->subspan_item<ModelData>(model_id.id, 1), &model_data, 1);

        auto parts = map_vec2small_vec(model->parts, [&](const ResourceManager::RenderModel::Part& part) {
            return PartData{
                .mesh_id     = part.mesh_id.id,
                .material_id = part.material_id.id,
            };
        });

        assert(model_id <= model_capacity);
        assert(allocation.size + allocation.offset <= part_capacity);
        stencil.copy_data(m_model_part_info_buffer->subspan_item<PartData>(allocation.offset, allocation.size), parts.as_const_span());
    }

    // for (auto material_id : resource_updates.material_updates) {
    //     auto* material = m_resource_manager->get_material(material_id);
    // }

    for (auto mesh_id : resource_updates.mesh_updates) {
        auto* mesh = m_resource_manager->get_mesh(mesh_id);

        MeshData mesh_data = {
            .index_offset = 0,
            .index_count  = mesh->index_count,
        };

        stencil.copy_data(m_mesh_info_buffer->subspan_item<MeshData>(mesh_id.id, 1), &mesh_data, 1);
    }

    flush_pending_entities(cmd, stencil);

    resource_updates.reset();

    stencil.flush_copies(cmd);

    VkBufferMemoryBarrier barriers[] = {
        VkBufferMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = m_mesh_info_buffer->handle(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = m_model_info_buffer->handle(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = m_material_info_buffer->handle(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = m_instance_buffer->handle(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
        VkBufferMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = m_model_part_info_buffer->handle(),
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        },
    };

    cmd.pipeline_barrier(PipelineBarrierArgs{
        .src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dst_stage_mask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .buffer_memory_barriers = barriers,
    });
}

void SceneBuffersManager::set_world(flecs::world* world) {
    m_world = world;

    m_handle_manager = std::make_unique<GPUHandleIDManager<Renderable>>(world);
}
} // namespace vke