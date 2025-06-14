#pragma once

#include <vke/util.hpp>
#include <vke/vke.hpp>

#include <memory>

#include "fwd.hpp"
#include "render/iobject_renderer.hpp"

namespace vke {

constexpr u32 part_capacity          = 1 << 12;
constexpr u32 model_capacity         = 1 << 10;
constexpr u32 material_capacity      = 1 << 10;
constexpr u32 instance_capacity      = 1 << 15;
constexpr u32 mesh_capacity          = 1 << 10;
constexpr u32 indirect_draw_capacity = 1 << 10;

// this class manages buffers for indirect rendering data
class SceneBuffersManager {
public:
    SceneBuffersManager(RenderServer* render_server, ResourceManager* resource_manager);
    ~SceneBuffersManager();

public:
    void updates_for_indirect_render(vke::CommandBuffer& compute_cmd);

    void set_registry(entt::registry* registry);

public: // getters
    vke::IBuffer* get_model_info_buffer() { return m_model_info_buffer.get(); }
    vke::IBuffer* get_model_part_info_buffer() { return m_model_part_info_buffer.get(); }
    vke::IBuffer* get_material_info_buffer() { return m_material_info_buffer.get(); }
    vke::IBuffer* get_mesh_info_buffer() { return m_mesh_info_buffer.get(); }
    vke::IBuffer* get_instance_data_buffer() { return m_instance_buffer.get(); }

    u32 get_part_max_id() const { return m_model_part_buffer_sub_allocator.max_id(); }

    const std::unordered_map<RenderModelID, i32>& get_model_instance_counters() const { return m_model_instance_counters; }
    const auto& get_model_part_sub_allocations() const { return m_model_part_sub_allocations; }

    entt::registry* get_registry() const { return m_registry; }
private:
    void flush_pending_entities(vke::CommandBuffer& cmd, StencilBuffer& stencil);

private:
    void connect_registry_callbacks();
    void disconnect_registry_callbacks();

    void renderable_component_creation_callback(entt::registry&, entt::entity e);
    void renderable_component_update_callback(entt::registry&, entt::entity e);
    void renderable_component_destroy_callback(entt::registry&, entt::entity e);

private:
    std::unique_ptr<vke::Buffer> m_model_info_buffer;
    std::unique_ptr<vke::Buffer> m_model_part_info_buffer;
    // allocates sub ranges from model_part_info_buffer.
    // allocations are done in parts not bytes
    VirtualAllocator m_model_part_buffer_sub_allocator;
    std::unordered_map<RenderModelID, VirtualAllocator::Allocation> m_model_part_sub_allocations;

    std::unique_ptr<vke::Buffer> m_material_info_buffer;

    // stores instance specific data
    std::unique_ptr<vke::GrowableBuffer> m_instance_buffer;

    std::unique_ptr<vke::Buffer> m_mesh_info_buffer;

    GenericIDManager<InstanceID> m_instance_id_manager = GenericIDManager<InstanceID>(0);
    vke::SlimVec<entt::entity> m_pending_entities_for_register;
    vke::SlimVec<InstanceID> m_pending_instances_for_destruction;

    std::unordered_map<RenderModelID, i32> m_model_instance_counters;

    entt::registry* m_registry          = nullptr;
    RenderServer* m_render_server       = nullptr;
    ResourceManager* m_resource_manager = nullptr;
};

} // namespace vke