#pragma once

#include <cassert>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../common.hpp"
#include "../util.hpp"

namespace vke {
class Serializer;
class Deserializer;

template <typename T>
concept Serializable =
    requires(T& a, Serializer* s, Deserializer* d) {
        { a.serialize(s) };
        { a.deserialize(d) };
    };

template <typename T>
concept BinarySerializable =
    requires(T& a, Serializer* s, Deserializer* d) {
        { a.serialize_bin(s) };
        { a.deserialize_bin(d) };
    };

template <typename T>
class SerializationHelper {
};

// template <>
// class SerializationHelper<int> {
//     static void serialize(Serializer* s, int a);
//     static void deserialize(Deserializer* d, int& a);
// };

template <typename T>
concept ExternalySerializeable =
    requires(T a, Serializer* s, Deserializer* d) {
        { SerializationHelper<T>::serialize(s, a) };
        { SerializationHelper<T>::deserialize(d, a) };
    };

class FieldNotFoundException : public std::runtime_error {
public:
    explicit FieldNotFoundException(const std::string& field_name)
        : std::runtime_error("Field not found: " + field_name),
          fieldName_(field_name) {}

    const std::string& get_field_name() const { return fieldName_; }

private:
    std::string fieldName_;
};

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
    virtual void _push(f64 item)                    = 0;
    virtual void _push(const std::string_view item) = 0;
    virtual void _push(std::span<u8> bytes)         = 0;
    virtual void _push(std::nullptr_t);

    void _push(const std::string& item) { _push(std::string_view(item)); }

    template <Serializable T>
    void _push(const T& item) { item.serialize(this); }
    template <ExternalySerializeable T>
    void _push(const T& item) { SerializationHelper<T>::serialize(this, item); }

    template <typename T>
    void _push(const std::optional<T>& item) {
        if (item.has_value()) {
            _push(item.value());
        } else {
            _push(nullptr);
        }
    }

    template <typename T>
    void _push(const std::vector<T>& item) {
        start_array();
        for (const auto& e : item) {
            push(e);
        }
        end_array();
    }

    template <typename T1, typename T2>
    void _push(const std::pair<T1, T2>& item) {
        start_array();
        push(item.first);
        push(item.second);
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

    template <typename T>
    void pull_root(T& object) {
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
    virtual void _pull(f64& item)         = 0;
    virtual void _pull(std::string& item) = 0;

    template <Serializable T>
    void _pull(T& item) { item.deserialize(this); }

    template <ExternalySerializeable T>
    void _push(T& item) { SerializationHelper<T>::serialize(this, item); }

    template <typename T>
    void _pull(std::vector<T>& container) {
        usize size = start_array();
        container.resize(size);

        for (auto& e : container) {
            pull(e);
        }
        end_array();
    }

    template <typename T1, typename T2>
    void _pull(std::pair<T1, T2>& pair) {
        usize size = start_array();
        if (size < 2) throw FieldNotFoundException("<pair is empty>"); // TODO better error
        pull(pair.first);
        pull(pair.second);
        end_array();
    }

    template <typename T>
    void _pull(std::optional<T>& item) {
        try {
            T tmp;
            pull(tmp);
            item = std::move(tmp);
        } catch (FieldNotFoundException& e) {
            item = std::nullopt;
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
