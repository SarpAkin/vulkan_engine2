#pragma once

#include <json.hpp>

#include "../util.hpp"
#include "serializer.hpp"

namespace vke {

class JsonSerializer final : public Serializer {
public:
    JsonSerializer() {
        m_stack.push_back(JsonStack{});
    }

    void start_object() override {
        m_stack.push_back(JsonStack{});
    }
    void end_object() override {
        auto top = std::move(m_stack.back().json);
        m_stack.pop_back();
        push_primative(std::move(top));
    }

    // arrays are handled the same way as objects
    void start_array() override { JsonSerializer::start_object(); }
    void end_array() override { JsonSerializer::end_object(); }

    nlohmann::json dump() {
        assert(m_stack.size() == 1);
        return std::move(m_stack[0].json);
    }

protected:
    void prepare_insert_field(const char* field_name) override { m_stack.back().field_insert_name = field_name; }

    void _push(u32 item) override { push_primative(item); }
    void _push(i32 item) override { push_primative(item); }
    void _push(f32 item) override { push_primative(item); }
    void _push(std::string_view item) override { push_primative(item); }
    void _push(std::span<u8> bytes) override { TODO(); }
    void _push(std::nullptr_t) override { push_primative(nullptr); };

private:
    template <typename T>
    void push_primative(T item) {
        auto& stack_top = m_stack.back();

        if (stack_top.field_insert_name) {
            stack_top.json.emplace(stack_top.field_insert_name, item);
            stack_top.field_insert_name = nullptr;
        } else {
            stack_top.json.push_back(item);
        }
    }

    struct JsonStack {
        nlohmann::json json;
        const char* field_insert_name;
    };

    std::vector<JsonStack> m_stack;
};

class JsonDeserializer : public Deserializer {
public:
    JsonDeserializer(nlohmann::json json) {
        m_stack.push_back(JsonStack{
            .json = std::move(json),
        });
    }

    JsonDeserializer(const std::string& s) {
        m_stack.push_back(JsonStack{
            .json = std::move(nlohmann::json::parse(s)),
        });
    }

    usize start_object() override {
        nlohmann::json json;
        pull_primative(json);
        usize size = json.size();

        m_stack.push_back(JsonStack{
            .json = std::move(json),
        });

        return size;
    };
    void end_object() override { m_stack.pop_back(); }

    usize start_array() override { return JsonDeserializer::start_object(); }
    void end_array() override { JsonDeserializer::end_object(); }

protected:
    void prepare_pull_field(const char* field_name) override { m_stack.back().field_name = field_name; }
    void prepare_root_pull() override {
        nlohmann::json json;
        json.push_back(std::move(m_stack[0].json));
        m_stack[0].json = std::move(json);
    };

    void _pull(u32& item) override { pull_primative(item); }
    void _pull(i32& item) override { pull_primative(item); }
    void _pull(f32& item) override { pull_primative(item); }
    void _pull(std::string& item) override { pull_primative(item); }

private:
    template <typename T>
    void pull_primative(T& item) {
        auto& stack_top = m_stack.back();
        if (stack_top.field_name) {
            auto field = stack_top.json[stack_top.field_name];
            if (field.is_null()) {
                throw FieldNotFoundException(stack_top.field_name);
            }
            item = field.get<T>();

            stack_top.field_name = nullptr;
        } else {
            item = stack_top.json[stack_top.index++];
        }
    }

private:
    struct JsonStack {
        nlohmann::json json;
        const char* field_name;
        usize index = 0;
    };
    std::vector<JsonStack> m_stack;
};

} // namespace vke
