#pragma once

#include <concepts>
#include <optional>
#include <string>
#include <unordered_map>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../common.hpp"

#include "buffer.hpp"

struct SpvReflectBlockVariable;

namespace vke {

class ReflectionMappedBuffer;

class BufferReflection {
public:
    BufferReflection(SpvReflectBlockVariable* block, VkShaderStageFlagBits stages);
    ~BufferReflection();

    struct Field {
        enum Type : u32 {
            NONE        = 0,
            UINT        = 1,
            INT         = 2,
            FLOAT       = 3,
            BOOL        = 4,
            VEC_BASE    = 10,
            VEC2        = 12,
            VEC3        = 13,
            VEC4        = 14,
            IVEC_BASE   = 20,
            IVEC2       = 22,
            IVEC3       = 23,
            IVEC4       = 24,
            BLOCK       = 100,
            BLOCK_ARRAY = 101,
        };

        Type type;
        u32 offset;
    };

public:
    const auto& get_fields() const { return m_fields; }
    usize get_buffer_size() const { return m_buffer_size; }
    VkShaderStageFlagBits get_stages() const { return m_stages; }

    std::optional<Field> get_field(const std::string& field) const;

    ReflectionMappedBuffer bind(vke::IBufferSpan* buffer);

private:
    VkShaderStageFlagBits m_stages;

private:
    usize m_buffer_size;
    std::unordered_map<std::string, Field> m_fields;
};

class FieldAccesor {
    using FType = BufferReflection::Field::Type;

public:
    FieldAccesor(vke::IBufferSpan* buffer_root, BufferReflection::Field field) : m_span(buffer_root->subspan(field.offset)) {
        m_field_data = field;
    }

    FType get_type() { return m_field_data.type; }

    template <class T>
    T& get_as() { return m_span.mapped_data<T>()[0]; }

    void operator=(const i32& val) {
        assert(m_field_data.type == FType::UINT || m_field_data.type == FType::INT);
        m_span.mapped_data<i32>()[0] = val;
    }

    void operator=(const f32& val) {
        assert(m_field_data.type == FType::FLOAT);
        m_span.mapped_data<f32>()[0] = val;
    }

    void operator=(const bool& val) {
        assert(m_field_data.type == FType::BOOL);
        m_span.mapped_data<u32>()[0] = val ? 1 : 0;
    }

    void operator=(const glm::vec3& val) {
        assert(m_field_data.type == (FType::VEC_BASE + 3));
        m_span.mapped_data<glm::vec3>()[0] = val;
    }

    void operator=(const glm::vec2& val) {
        assert(m_field_data.type == (FType::VEC_BASE + 2));
        m_span.mapped_data<glm::vec2>()[0] = val;
    }

    template <usize N>
    void operator=(const glm::vec<N, i32>& val) {
        assert(m_field_data.type == (FType::IVEC_BASE + N));
        m_span.mapped_data<glm::vec<N, i32>>()[0] = val;
    }

    template <usize N>
    void operator=(const glm::vec<N, f32>& val) {
        assert(m_field_data.type == (FType::VEC_BASE + N));
        m_span.mapped_data<glm::vec<N, f32>>()[0] = val;
    }

private:
    vke::BufferSpan m_span;
    BufferReflection::Field m_field_data;
};

class ReflectionMappedBuffer {
public:
    ReflectionMappedBuffer(vke::IBufferSpan* buffer, BufferReflection* reflection) {
        m_buffer     = buffer;
        m_reflection = reflection;
    }

    FieldAccesor operator[](const std::string& field_name) {
        auto field = m_reflection->get_field(field_name);
        return FieldAccesor(m_buffer, field.value());
    }

    template <class F>
        requires std::invocable<F, const std::string&, FieldAccesor&>
    void for_each_field(F&& func) {
        for (auto& [name, field] : m_reflection->get_fields()) {
            auto accesor = FieldAccesor(m_buffer, field);
            func(name, accesor);
        }
    }

private:
    vke::IBufferSpan* m_buffer;
    BufferReflection* m_reflection;
};

} // namespace vke