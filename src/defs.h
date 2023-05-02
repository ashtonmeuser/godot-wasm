#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

#ifdef GODOT_MODULE // Godot includes when building module
  #include "core/io/stream_peer.h"
#else // Godot addon includes
  #include "godot_cpp/classes/ref_counted.hpp"
  #include "godot_cpp/classes/stream_peer_extension.hpp"
#endif

#ifdef GODOT_MODULE
  #define godot_error Error
  #define PRINT_ERROR(message) print_error("Godot Wasm: " + String(message))
  #define REGISTRATION_METHOD _bind_methods
#else
  #define PRINT_ERROR(message) _err_print_error(__FUNCTION__, __FILE__, __LINE__, "Godot Wasm: " + String(message))
  #define godot_error Error
  #define REGISTRATION_METHOD _bind_methods
#endif
#define REAL FLOAT
#define Reference RefCounted
#define PoolByteArray PackedByteArray
#define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
#define FAIL_IF(cond, message, ret) if (unlikely(cond)) FAIL(message, ret)
#define NULL_VARIANT Variant()
#define PAGE_SIZE 65536

#endif
