#pragma once
#include <DxUi/Failure.h>
#include <type_traits>

namespace RedTilde
{
// Public InvokeNative accepts free native function pointers. This adapter adds only a synchronous COM
// member-call thunk: no application callback/model work belongs in it. Arguments are prepared before entry;
// reentrant SDK callbacks regain their own scopes and an escaping native C++ exception stays fatal.
template <class Interface, class Method, class... Args>
auto InvokeNativeMethod(Interface *instance, Method method, Args... args) noexcept -> decltype((instance->*method)(args...))
{
    static_assert(std::is_member_function_pointer_v<Method>);
    return DxUi::Failure::InvokeNative(
        +[](Interface *object, Method member, Args... values) noexcept -> decltype((object->*member)(values...))
        { return (object->*member)(values...); }, instance, method, args...);
}
} // namespace RedTilde
