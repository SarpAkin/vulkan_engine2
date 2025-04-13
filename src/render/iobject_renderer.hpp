#pragma once

#include "glm/ext/vector_float3.hpp"
#include <entt/fwd.hpp>
#include <vke/fwd.hpp>
#include <vke/util.hpp>

#include <cstdint>
namespace vke {

namespace impl {
template <class Type>
struct GenericID {
    using IDIntegerType = uint32_t;

    IDIntegerType id;
    GenericID() { id = 0; }
    GenericID(IDIntegerType _id) : id(_id) {}

public:
    auto operator<=>(const GenericID<Type>& other) const = default;
};

struct ModelObject {};
struct Mesh {};
struct Material {};

} // namespace impl

using RenderModelID = impl::GenericID<impl::ModelObject>;
using MeshID        = impl::GenericID<impl::Mesh>;
using MaterialID    = impl::GenericID<impl::Material>;
using ImageID       = impl::GenericID<vke::Image>;
using RenderPassID  = impl::GenericID<vke::ISubpass>;
using InstanceID    = impl::GenericID<struct ObjectInstance>;

template <class T>
class GenericIDManager {
public:
    using GIDType = T;

    GIDType new_id() { return GIDType(id_manager.new_id()); }
    void free_id(GIDType id) { id_manager.free_id(id.id); }

public:
    vke::IDManager<uint32_t> id_manager = vke::IDManager<uint32_t>(1);
};

// Component for rendering
struct Renderable {
    const RenderModelID model_id;
};

struct CPointLight {
    glm::vec3 color;
    float range;
};

struct CDirectionalLight {
    glm::vec3 direction;
    glm::vec3 color;
};

class RenderServer;
class Camera;

struct RenderArguments {
    vke::CommandBuffer* subpass_cmd;
    // executed before subpass is started
    vke::CommandBuffer* compute_cmd;
    std::string render_target_name;
    vke::IImageView* hzb_buffer         = nullptr;
    VkDescriptorSet render_pipeline_set = VK_NULL_HANDLE;
};

struct SetIndices {
    // int scene_set           = 0;
    int view_set            = 0;
    int material_set        = 1;
    int render_pipeline_set = 3;
};

// This class is responsible for managing :
//  - Meshes
//  - Materials
//  - objects
//  and rendering
//  It is not CONCURRENT
class IObjectRenderer {
public:
    virtual void set_entt_registry(entt::registry* registry) = 0;
    virtual void render(const RenderArguments& cmd)          = 0;

public:
    virtual ~IObjectRenderer() = default;
};

} // namespace vke

template <class Type>
struct std::hash<vke::impl::GenericID<Type>> {
    std::size_t operator()(const vke::impl::GenericID<Type>& key) const noexcept {
        // Use a simple hash of `id`
        return std::hash<uint32_t>{}(key.id);
    }
};
