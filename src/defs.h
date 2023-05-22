#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

#ifdef GODOT_MODULE // Godot includes when building module
  #include "core/io/stream_peer.h"
#else // Godot addon includes
  #include <Godot.hpp>
  #include <StreamPeerGDNative.hpp>
#endif

#ifdef GODOT_MODULE
  #define godot_error Error
  #define PRINT_ERROR(message) print_error("Godot Wasm: " + String(message))
  #define REGISTRATION_METHOD _bind_methods
#else
  #define OK GODOT_OK
  #define ERR_INVALID_DATA GODOT_ERR_INVALID_DATA
  #define ERR_COMPILATION_FAILED GODOT_ERR_COMPILATION_FAILED
  #define ERR_CANT_CREATE GODOT_ERR_CANT_CREATE
  #define ERR_PARAMETER_RANGE_ERROR GODOT_ERR_PARAMETER_RANGE_ERROR
  #define PRINT_ERROR(message) Godot::print_error("Godot Wasm: " + String(message), __func__, __FILE__, __LINE__)
  #define GDCLASS GODOT_CLASS
  #define REGISTRATION_METHOD _register_methods
#endif
#define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
#define FAIL_IF(cond, message, ret) if (unlikely(cond)) FAIL(message, ret)
#define NULL_VARIANT Variant()
#define PAGE_SIZE 65536

#endif
