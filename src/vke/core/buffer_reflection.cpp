#include "buffer_reflection.hpp"

#include "spv_reflect_util.hpp"

namespace vke {

ReflectionMappedBuffer BufferReflection::bind(vke::IBufferSpan* buffer) {
    return ReflectionMappedBuffer(buffer, this);
}

BufferReflection::~BufferReflection() = default;

std::optional<BufferReflection::Field> BufferReflection::get_field(const std::string& field) const {
    if (auto it = m_fields.find(field); it != m_fields.end()) {
        return it->second;
    } else {
        return std::nullopt;
    }
}

BufferReflection::BufferReflection(SpvReflectBlockVariable* block, VkShaderStageFlagBits stage) {
    m_stages      = stage;
    m_buffer_size = block->size;

    for (int i = 0; i < block->member_count; i++) {
        auto& member = block->members[i];
        using Type   = BufferReflection::Field::Type;
        Type type    = Type::NONE;

        switch (member.type_description->type_flags) {
        case SPV_REFLECT_TYPE_FLAG_BOOL:
            type = Type::BOOL;
            break;
        case SPV_REFLECT_TYPE_FLAG_INT:
            if (member.type_description->id == 25) // 25 is bool
            {
                type = Type::BOOL;
                break;
            }
            type = Type::INT;
            break;
        case SPV_REFLECT_TYPE_FLAG_FLOAT:
            type = Type::FLOAT;
            break;
        case (SPV_REFLECT_TYPE_FLAG_VECTOR | SPV_REFLECT_TYPE_FLAG_FLOAT):
            type = Type(Type::VEC_BASE + member.type_description->traits.numeric.vector.component_count);
            break;
        case (SPV_REFLECT_TYPE_FLAG_MATRIX | SPV_REFLECT_TYPE_FLAG_FLOAT):
            type = Field::MAT4;
            break;
        }

        m_fields.emplace(member.name,
            Field{
                .type   = type,
                .offset = member.absolute_offset,
            } //
        );
    }
}

} // namespace vke