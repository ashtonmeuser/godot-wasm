#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

/*
Common definitions and imports
Useful for minimizing changes to implementation files between targets e.g. GDExtension, GDNative, Godot module
*/

#ifdef GODOT_MODULE // Godot includes when building module
  #include <core/os/os.h>
  #include <core/os/time.h>
  #include <core/crypto/crypto.h>
  #include <core/io/stream_peer.h>
  #include <core/variant/variant_utility.h>
#else // Godot addon includes
  #include <godot_cpp/classes/ref_counted.hpp>
  #include <godot_cpp/classes/os.hpp>
  #include <godot_cpp/classes/time.hpp>
  #include <godot_cpp/classes/crypto.hpp>
  #include <godot_cpp/classes/stream_peer_extension.hpp>
  #include <godot_cpp/variant/utility_functions.hpp>
#endif

#ifdef GODOT_MODULE
  #define PRINT(message) print_line(String(message))
  #define PRINT_ERROR(message) print_error("Godot Wasm: " + String(message))
  #define godot_error Error
  #define REGISTRATION_METHOD _bind_methods
  #define RANDOM_BYTES(n) Crypto::create()->generate_random_bytes(n)
#else
  #define PRINT(message) UtilityFunctions::print(String(message))
  #define PRINT_ERROR(message) _err_print_error(__FUNCTION__, __FILE__, __LINE__, "Godot Wasm: " + String(message))
  #define godot_error Error
  #define VariantUtilityFunctions UtilityFunctions
  #define REGISTRATION_METHOD _bind_methods
  #define RANDOM_BYTES(n) [n]()->PackedByteArray{Ref<Crypto> c;c.instantiate();return c->generate_random_bytes(n);}()
#endif
#define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
#define FAIL_IF(cond, message, ret) if (unlikely(cond)) FAIL(message, ret)
#define INSTANTIATE_REF(ref) ref.instantiate()
#define BYTE_ARRAY_POINTER(array) array.ptr()
#define CMDLINE_ARGS OS::get_singleton()->get_cmdline_user_args()
#define TIME_REALTIME Time::get_singleton()->get_unix_time_from_system() * 1000000000
#define TIME_MONOTONIC Time::get_singleton()->get_ticks_usec() * 1000
#define NULL_VARIANT Variant()
#define PAGE_SIZE 65536

#endif
