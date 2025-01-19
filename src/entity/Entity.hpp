#pragma once

#include <glm/mat4x4.hpp>
#include <vector>

namespace vke {

class Entity {
public:
    Entity(glm::vec3 pos) { position = pos; }

public:
    glm::vec3 position;

private:
};

} // namespace vke