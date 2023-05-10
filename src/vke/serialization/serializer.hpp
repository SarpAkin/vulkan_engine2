#pragma once

#include <cassert>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

#include "../common.hpp"
#include "../util.hpp"

namespace vke {
class Serializer;
}

class FieldNotFoundException : public std::runtime_error {
public:
    explicit FieldNotFoundException(const std::string& fieldName)
        : std::runtime_error("Field not found: " + fieldName), fieldName_(fieldName) {}

    const std::string& getFieldName() const {
        return fieldName_;
    }

private:
    std::string fieldName_;
};

void serialize_external(vke::Serializer*, u32);

namespace vke {
class Serializer {
public:
    template <typename T>
    void push(const T& item) { _push(item); }

    virtual void start_object() = 0;
    virtual void end_object()   = 0;
    template <typename T>
    void insert(const char* field_name, const T& item) {
        prepare_insert_field(field_name);
        push(item);
    }

    virtual void start_array() = 0;
    virtual void end_array()   = 0;

protected:
    virtual void prepare_insert_field(const char* field_name) {}

    virtual void _push(u32 item)                    = 0;
    virtual void _push(i32 item)                    = 0;
    virtual void _push(f32 item)                    = 0;
    virtual void _push(const std::string_view item) = 0;
    virtual void _push(std::span<u8> bytes)         = 0;

    void _push(const std::string& item) { _push(std::string_view(item)); }

    template <typename T>
    void _push(const T& item) {
        if constexpr (requires(T x) {x.begin();x.end(); }) {
            _push_container(item);
        } else if constexpr (requires(Serializer * s, const T& x) { x.serialize(s); }) {
            item.serialize(this);
        } else if constexpr (requires(Serializer * s, const T& x) { ::serialize_external(s, x); }) {
            ::serialize_external(this, item);
        } else {
            assert(!"serialization failed");
        }
    }

    template <typename T>
    void _push_container(const T& item) {
        start_array();
        for (const auto& e : item) {
            push(e);
        }

        end_array();
    }

private:
};

class Deserializer {
public:
    template <typename T>
    void pull(T& item) { _pull(item); }

    template <typename T>
    void get_field(const char* field_name, T& item) {
        prepare_pull_field(field_name);
        pull(item);
    }


    template<typename T>
    void pull_root(T& object){
        prepare_root_pull();
        pull(object);
    }


    // retrun the amount of fields in the object
    virtual usize start_object() = 0;
    virtual void end_object()    = 0;

    // return the lenght of array
    virtual usize start_array() = 0;
    virtual void end_array()    = 0;

protected:
    virtual void prepare_pull_field(const char* field_name){};
    virtual void prepare_root_pull(){};


    virtual void _pull(u32& item)         = 0;
    virtual void _pull(i32& item)         = 0;
    virtual void _pull(f32& item)         = 0;
    virtual void _pull(std::string& item) = 0;

    template <typename T>
    void _pull(std::vector<T>& container) {
        usize size = start_array();
        container.resize(size);

        for (auto& e : container) {
            pull(e);
        }
        end_array();
    }

    template<typename T>
    void _pull(std::optional<T>& item){
        try {
            T tmp;
            pull(tmp);
            item = std::move(tmp);
        } catch (FieldNotFoundException& e) {
            item = std::nullopt;
        }
    }

    template <typename T>
    void _pull(T& item) {
        if constexpr (requires(T x) {x.begin();x.end(); }) {
            // _pull_container(item);
            TODO();
        } else if constexpr (requires(Deserializer * d, T& x) { x.deserialize(d); }) {
            item.deserialize(this);
            // } else if constexpr (requires(Serializer * s, const T& x) { ::serialize_external(s, x); }) {
            //     ::serialize_external(this, item);
        } else {
            assert(!"serialization failed");
        }
    }



private:
};

} // namespace vke

#include "foreachmacro.hpp"

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define AUTO_SERIALIZATON(T, fields...)          \
    void serialize(vke::Serializer* ser) const { \
        ser->start_object();                     \
        FOREACH(SERIALIZE_FIELD, fields)         \
        ser->end_object();                       \
    }                                            \
    void deserialize(vke::Deserializer* deser) { \
        deser->start_object();                   \
        FOREACH(DESERIALIZE_FIELD, fields)       \
        deser->end_object();                     \
    }

#define SERIALIZE_FIELD(field) ser->insert(STRINGIFY(field), field);
#define DESERIALIZE_FIELD(field) deser->get_field(STRINGIFY(field), field);
