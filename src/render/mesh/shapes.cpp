#include "shapes.hpp"

namespace vke {

std::unique_ptr<Mesh> make_cube(float size) {

    uint32_t color = 0xFFFFFFFF;
    std::vector<Mesh::Vertex> verticies;
    verticies.push_back({.pos = {0, 0, 0}, .color = color});
    verticies.push_back({.pos = {0, 0, 1}, .color = color});

    verticies.push_back({.pos = {0, 1, 0}, .color = color});
    verticies.push_back({.pos = {0, 1, 1}, .color = color});

    verticies.push_back({.pos = {1, 0, 0}, .color = color});
    verticies.push_back({.pos = {1, 0, 1}, .color = color});

    verticies.push_back({.pos = {1, 1, 0}, .color = color});
    verticies.push_back({.pos = {1, 1, 1}, .color = color});

    std::vector<uint16_t> indicies;
    indicies.insert(indicies.end(), {0, 1, 2});
    indicies.insert(indicies.end(), {3, 1, 2});

    indicies.insert(indicies.end(), {4, 5, 6});
    indicies.insert(indicies.end(), {7, 5, 6});

    indicies.insert(indicies.end(), {2, 3, 6});
    indicies.insert(indicies.end(), {3, 6, 7});

    indicies.insert(indicies.end(), {0, 1, 4});
    indicies.insert(indicies.end(), {1, 4, 5});

    indicies.insert(indicies.end(), {0, 2, 4});
    indicies.insert(indicies.end(), {2, 4, 6});

    indicies.insert(indicies.end(), {1, 3, 5});
    indicies.insert(indicies.end(), {3, 5, 7});

    return nullptr;

    // return Mesh::make_mesh(verticies, indicies);
}

} // namespace vke