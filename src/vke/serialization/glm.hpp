#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/matrix.hpp>

#include "serializer.hpp"

namespace vke {
template <typename T, size_t N>
class SerializationHelper<glm::vec<N, T>> {
public:
    void serialize(Serializer* s, glm::vec<N, T> vec) {
        s->start_array();
        for (int i = 0; i < N; ++i) {
            s->push(vec[i]);
        }
        s->end_array();
    }
    void deserialize(Deserializer* d, glm::vec<N, T>& vec) {
        d->start_array();
        for (int i = 0; i < N; ++i) {
            d->pull(vec[i]);
        }
        d->end_array();
    }
};



} // namespace vke