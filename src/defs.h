#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

#ifdef GODOT_MODULE // Godot includes when building module
  #include "core/io/stream_peer.h"
#else // Godot addon includes
  #include <Godot.hpp>
  #include <StreamPeerGDNative.hpp>
#endif

namespace {
  #ifdef __GNUC__
    #define UNLIKELY(cond) __builtin_expect(!!(cond), 0)
  #else
    #define UNLIKELY(cond) cond
  #endif
  #ifdef GODOT_MODULE
    #define NS
    #define godot_error Error
    #define PRINT_ERROR(message) print_error("Godot Wasm: " + String(message))
    #define REGISTRATION_METHOD _bind_methods
  #else
    #define NS godot
    #define OK GODOT_OK
    #define ERR_INVALID_DATA GODOT_ERR_INVALID_DATA
    #define ERR_COMPILATION_FAILED GODOT_ERR_COMPILATION_FAILED
    #define ERR_CANT_CREATE GODOT_ERR_CANT_CREATE
    #define PRINT_ERROR(message) godot::Godot::print_error("Godot Wasm: " + godot::String(message), __func__, __FILE__, __LINE__)
    #define GDCLASS GODOT_CLASS
    #define REGISTRATION_METHOD _register_methods
  #endif
  #define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
  #define FAIL_IF(cond, message, ret) if (UNLIKELY(cond)) FAIL(message, ret)
  #define NULL_VARIANT NS::Variant()
  #define PAGE_SIZE 65536
}

#endif
