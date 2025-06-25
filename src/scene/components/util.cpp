#include "util.hpp"

#include "components.hpp"
#include "render/iobject_renderer.hpp"

namespace vke {

template <class T>
void copy_components(entt::registry& scene, const entt::registry& prefab, const std::unordered_map<entt::entity, entt::entity>& map) {
    auto prefab_view = prefab.view<T>();
    auto scene_view  = scene.view<T>();

    prefab_view.each([&](entt::entity e, const T& c) {
        scene.emplace<T>(map.at(e), c);
    });
}

entt::entity instantiate_prefab(entt::registry& scene, const entt::registry& prefab, const std::optional<Transform>& t) {
    SmallVec<entt::entity> scene_entities;
    std::unordered_map<entt::entity, entt::entity> entity_map;

    entt::entity root = scene.create();

    prefab.view<entt::entity>().each([&](auto entity) {
        entt::entity new_entity = scene.create();
        scene_entities.push_back(new_entity);
        // entity_map.push_back(std::make_tuple(entity, new_entity));
        entity_map[entity] = new_entity;
    });

    copy_components<Transform>(scene, prefab, entity_map);
    copy_components<Renderable>(scene, prefab, entity_map);

    scene.insert<CParent>(scene_entities.begin(), scene_entities.end(), CParent{root});

    scene.emplace<CChilds>(root, CChilds{.children = std::move(scene_entities)});

    if (t.has_value()) {
        scene.emplace<Transform>(root, t.value());
    }

    return root;
}






} // namespace vke