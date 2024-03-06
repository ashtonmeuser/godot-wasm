#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

#ifdef GODOT_MODULE // Godot includes when building module
  #include "core/os/os.h"
  #include "core/os/time.h"
  #include "core/crypto/crypto.h"
  #include "core/io/stream_peer.h"
#else // Godot addon includes
  #include "godot_cpp/classes/ref_counted.hpp"
  #include "godot_cpp/classes/os.hpp"
  #include "godot_cpp/classes/time.hpp"
  #include "godot_cpp/classes/crypto.hpp"
  #include "godot_cpp/classes/stream_peer_extension.hpp"
  #include "godot_cpp/variant/utility_functions.hpp"
#endif

#ifdef GODOT_MODULE
  #define GODOT_WASM_ERROR Error
  #define GODOT_WASM_PRINT(message) print_line(String(message))
  #define GODOT_WASM_PRINT_ERROR(message) print_error("Godot Wasm: " + String(message))
  #define GODOT_WASM_REGISTRATION_METHOD _bind_methods
  #define GODOT_WASM_RANDOM_BYTES(n) Crypto::create()->generate_random_bytes(n)
#else
  #define GODOT_WASM_PRINT(message) UtilityFunctions::print(String(message))
  #define GODOT_WASM_PRINT_ERROR(message) _err_print_error(__FUNCTION__, __FILE__, __LINE__, "Godot Wasm: " + String(message))
  #define GODOT_WASM_ERROR Error
  #define GODOT_WASM_REGISTRATION_METHOD _bind_methods
  #define GODOT_WASM_RANDOM_BYTES(n) [n]()->PackedByteArray{Ref<Crypto> c;c.instantiate();return c->generate_random_bytes(n);}()
#endif
#define GODOT_WASM_FAIL(message, ret) do { GODOT_WASM_PRINT_ERROR(message); return ret; } while (0)
#define GODOT_WASM_FAIL_IF(cond, message, ret) if (unlikely(cond)) GODOT_WASM_FAIL(message, ret)
#define GODOT_WASM_INSTANTIATE_REF(ref) ref.instantiate()
#define GODOT_WASM_BYTE_ARRAY_POINTER(array) array.ptr()
#define GODOT_WASM_CMDLINE_ARGS OS::get_singleton()->get_cmdline_user_args()
#define GODOT_WASM_TIME_REALTIME Time::get_singleton()->get_unix_time_from_system() * 1000000000
#define GODOT_WASM_TIME_MONOTONIC Time::get_singleton()->get_ticks_usec() * 1000
#define GODOT_WASM_NULL_VARIANT Variant()
#define GODOT_WASM_PAGE_SIZE 65536

#endif
