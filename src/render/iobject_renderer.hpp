#pragma once

#include "glm/ext/vector_float3.hpp"
#include <entt/fwd.hpp>
#include <vke/fwd.hpp>

#include <cstdint>
namespace vke {

namespace impl {
template <class Type>
struct GenericID {
    uint32_t id;
    GenericID() { id = 0; }
    GenericID(uint32_t _id) : id(_id) {}

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

// Component for rendering
struct Renderable {
    RenderModelID model_id;
};

struct CPointLight{
    glm::vec3 color;
    float range;
};

struct CDirectionalLight{
    glm::vec3 direction;
    glm::vec3 color;
};

class RenderServer;

// This class is responsible for managing :
//  - Meshes
//  - Materials
//  - objects
//  and rendering
//  It is not CONCURRENT
class IObjectRenderer {
public:
    virtual void set_entt_registery(entt::registry* registery)  = 0;
    virtual void render(vke::CommandBuffer& cmd)                = 0;

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
