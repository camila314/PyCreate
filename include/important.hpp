#pragma once

#include "json_types.hpp"
#include <Geode/Geode.hpp>
#include <Geode/loader/IPC.hpp>

using namespace geode::prelude;

template <typename T>
std::string convert_to_hex(T value) {
    std::stringstream ss;
    ss << std::hex << (long)value;
    return ss.str();
}

template <typename T>
struct _not_void {
    using type = T;
};
template <>
struct _not_void<void> {
    using type = std::nullptr_t;
};

template <>
struct _not_void<TodoReturnPlaceholder> {
    using type = std::nullptr_t;
};

template <typename T>
using not_void = typename _not_void<T>::type;

template <typename T>
struct swap_todo;

template <typename R, typename J, typename ...Args>
struct swap_todo<R(J::*)(Args...)> {
    using type = void(J::*)(Args...);
};

template <typename R, typename ...Args>
struct swap_todo<R(*)(Args...)> {
    using type = void(*)(Args...);
};

template <typename R, typename J, typename ...Args>
struct swap_todo<R(J::*)(Args...) const> {
    using type = void(J::*)(Args...) const;
};

template <typename T>
using swap_todo_t = typename swap_todo<T>::type;


template <typename T>
struct remove_const_reference {
    using type = T;
};

template <typename T> requires (std::is_const_v<std::remove_reference_t<T>>)
struct remove_const_reference<T&> {
    using type = T;
};

template <typename T>
using remove_const_reference_t = typename remove_const_reference<T>::type;


template <typename T>
remove_const_reference_t<T> fromJson(matjson::Value const& val) {
    if constexpr (std::is_same_v<T, char const*>) {
        auto tmp = CCString::createWithFormat("%s", val.as_string().c_str());
        return tmp->getCString();
    }

    // if a non-const reference, turn into a pointer
    if constexpr (std::is_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>) {
        return *val.as<std::remove_reference_t<T>*>();
    } else {
        return val.as<std::remove_const_t<std::remove_reference_t<T>>>();
    }
}

template <bool a, typename T>
matjson::Value toJson(T const& val) {
    if constexpr (std::is_same_v<T, _ccColor3B>) {
        return std::vector {val.r, val.g, val.b};
    } else if constexpr (std::is_same_v<T, CCArray*>) {
        return matjson::Serialize<T>::to_json(val);
    } else if constexpr (std::is_pointer<T>::value) {
        //todo move to serialize
        if constexpr (a && std::is_base_of_v<CCObject, std::remove_pointer_t<T>>) {
            const_cast<std::remove_const_t<std::remove_pointer_t<T>>*>(val)->retain();
        }

        return convert_to_hex(val);
    } else {
        return val;
    }
}

template <typename T>
remove_const_reference_t<T> fromJsonArr(std::vector<matjson::Value>& val) {
    auto first = val[0];
    val.erase(val.begin());
    return fromJson<T>(first);
}

template <typename R, typename U, typename V, typename ...Args>
not_void<R> callMemberFn(U fn, V first, Args... args) {
    if constexpr (std::is_same_v<R, void>) {
        (first->*fn)(args...);
        return nullptr;
    } else if constexpr (std::is_same_v<R, TodoReturnPlaceholder>) {
        auto fn2 = reference_cast<swap_todo_t<U>>(fn);
        (first->*fn2)(args...);
        return nullptr;
    } else {
        return (first->*fn)(args...);
    }
}
