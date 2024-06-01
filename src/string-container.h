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
  template <typename T> struct has_subscript_operator<T, std::__void_t<decltype(std::declval<T&>()[std::declval<size_t>()])>>: std::true_type {};
  template <typename, typename = void> struct has_get_method: std::false_type {};
  template <typename T> struct has_get_method<T, std::__void_t<decltype(std::declval<T&>().get(std::declval<size_t>()))>>: std::true_type {};
}

namespace godot {
  template <typename T> typename std::enable_if<has_subscript_operator<T>::value, String>::type string_container_get(T container, const uint32_t i) {
    return container[i];
  }

  template <typename T> typename std::enable_if<has_get_method<T>::value && !has_subscript_operator<T>::value, String>::type string_container_get(T container, const uint32_t i) {
    return container.get(i);
  }
}

#endif