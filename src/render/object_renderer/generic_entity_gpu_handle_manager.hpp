#pragma once

#include <flecs.h>

#include "../iobject_renderer.hpp"
#include "scene/components/transform.hpp"

#include <vke/fwd.hpp>
#include <vke/util.hpp>

namespace vke {

// creates a handle for every entity with that has the following component
template <class TargetComponent>
class GPUHandleIDManager {
public:
    using HandleID = impl::GenericID<TargetComponent>;
private:
    struct HandleComponent {
        HandleID id;
    };

public:
    GPUHandleIDManager(flecs::world* world) {
        m_world = world;

        m_observers.push_back(m_world->observer<TargetComponent>().event(flecs::OnAdd).each([&](flecs::entity e, const auto& c) {
            m_new_entities.push_back(e.id());
        }));

        m_observers.push_back(m_world->observer<TargetComponent>().event(flecs::OnRemove).each([&](flecs::entity e, const auto& c) {
            m_destroyed_entity_handles.push_back(e.get<const HandleComponent>()->id);
        }));

    }

    ~GPUHandleIDManager(){
        for(auto& o : m_observers){
            o.destruct();
        }
    }

    void flush_and_register_handles(auto&& func) {
        for (auto& ent_id : m_new_entities) {
            auto e = flecs::entity(*m_world, ent_id);

            auto handle_id = id_manager.new_id();

            e.set(HandleComponent{
                .id = handle_id,
            });

            func(e, handle_id);
        }

        m_new_entities.clear();
    }

    void flush_destroyed_handles() {
        for (auto id : m_destroyed_entity_handles) {
            id_manager.free_id(id);
        }

        m_destroyed_entity_handles.clear();
    }

public:
    const auto& get_destroyed_entities() { return m_destroyed_entity_handles; }
    auto& get_id_manager() { return id_manager; }

private:
    GenericIDManager<HandleID> id_manager = GenericIDManager<HandleID>(0);
    flecs::world* m_world;
    vke::SlimVec<flecs::observer> m_observers;
    SlimVec<flecs::entity_t> m_new_entities;
    SlimVec<HandleID> m_destroyed_entity_handles;
};

struct Ass {
    flecs::entity e;
};

} // namespace vke