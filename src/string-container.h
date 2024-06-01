#ifndef GODOT_WASM_STRING_CONTAINER_H
#define GODOT_WASM_STRING_CONTAINER_H

/*
Gets an element of a container of Godot strings
E.g. List, PoolStringArray, PackedStringArray
Handles cases where type differs across target i.e. GDExtension, GDNative, Godot module
Required because List does not support subscript operator, GDExtension PackedStringArray does not support get()
*/

#include <type_traits>
#include "defs.h"

namespace {
    template <typename, typename = void> struct has_subscript_operator: std::false_type {};
    template <typename T> struct has_subscript_operator<T, std::void_t<decltype(std::declval<T&>()[std::declval<size_t>()])>>: std::true_type {};
}

namespace godot{
    template <typename T> String string_container_get(T container, const int i) {
        if constexpr (has_subscript_operator<T>::value) return container[i];
        else return container.get(i);
    }
}

#endif