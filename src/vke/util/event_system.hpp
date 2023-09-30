#pragma once

#include <unordered_map>

namespace vke {

template <auto member_func>
auto method_to_fptr() {
    return []<class TMRet, class TClass, class... TMArgs>(TMRet (TClass::*func_)(TMArgs...)) {
        return +[](TClass* obj, TMArgs... args) { return (obj->*member_func)(args...); };
    }(member_func);
}

template <auto member_func>
auto method_to_fptr_void_handle() {
    return []<class TMRet, class TClass, class... TMArgs>(TMRet (TClass::*func_)(TMArgs...)) {
        return +[](void* obj, TMArgs... args) { return (reinterpret_cast<TClass*>(obj)->*member_func)(args...); };
    }(member_func);
}

template <typename... TArgs>
class EventManager {
    using TCallback = void(*)(void*, TArgs...);

public:
    template <auto method>
    void register_listener(void* handle) {
        m_callbacks[handle] = method_to_fptr_void_handle<method>();
    }

    void remove_listener(void* handle) {
        m_callbacks.erase(handle);
    }

    void event(TArgs... args) {
        for (auto& [handle, fptr] : m_callbacks) {
            fptr(handle, args...);
        }
    }

private:
    std::unordered_map<void*, TCallback> m_callbacks;
};

class Foo {
public:
    void bar(int);
    void bar2();
};

#define EL_REGISTER_METHOD(manager, method) ().register_listener<&decltype(this)::method>(this);
#define EL_REMOVE_METHOD(manager, method) ().remove_listener<&decltype(this)::method>(this);

} // namespace vke