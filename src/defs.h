#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

/*
Common definitions and imports
Useful for minimizing changes to implementation files between targets e.g. GDExtension, GDNative, Godot module
*/

#ifdef GODOT_MODULE // Godot includes when building module
  #include <core/os/os.h>
  #include <core/crypto/crypto.h>
  #include <core/io/stream_peer.h>
#else // Godot addon includes
  #include <Godot.hpp>
  #include <OS.hpp>
  #include <Crypto.hpp>
  #include <StreamPeerGDNative.hpp>
#endif

#ifdef GODOT_MODULE
  #define PRINT(message) print_line(String(message))
  #define PRINT_ERROR(message) print_error("Godot Wasm: " + String(message))
  #define godot_error Error
  #define INSTANCE_FROM_ID(id) ObjectDB::get_instance(id)
  #define INSTANCE_VALIDATE(id) ObjectDB::instance_validate(id)
  #define REGISTRATION_METHOD _bind_methods
  #define RANDOM_BYTES(n) Crypto::create()->generate_random_bytes(n)
#else
  #define OK GODOT_OK
  #define FAILED GODOT_FAILED
  #define ERR_INVALID_DATA GODOT_ERR_INVALID_DATA
  #define ERR_COMPILATION_FAILED GODOT_ERR_COMPILATION_FAILED
  #define ERR_CANT_CREATE GODOT_ERR_CANT_CREATE
  #define ERR_PARAMETER_RANGE_ERROR GODOT_ERR_PARAMETER_RANGE_ERROR
  #define PRINT(message) Godot::print(String(message))
  #define PRINT_ERROR(message) Godot::print_error("Godot Wasm: " + String(message), __func__, __FILE__, __LINE__)
  #define ObjectID int64_t
  #define INSTANCE_FROM_ID(id) godot::core_1_2_api->godot_instance_from_id(id) == nullptr ? nullptr : godot::detail::get_wrapper<Object>(godot::core_1_2_api->godot_instance_from_id(id))
  #define INSTANCE_VALIDATE(id) godot::core_1_1_api->godot_is_instance_valid((godot_object*)id)
  #define GDCLASS GODOT_CLASS
  #define REGISTRATION_METHOD _register_methods
  #define RANDOM_BYTES(n) Crypto::_new()->generate_random_bytes(n)
#endif
#define FLOAT REAL
#define RefCounted Reference
#define PackedByteArray PoolByteArray
#define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
#define FAIL_IF(cond, message, ret) if (unlikely(cond)) FAIL(message, ret)
#define INSTANTIATE_REF(ref) ref.instance()
#define BYTE_ARRAY_POINTER(array) array.read().ptr()
#define CMDLINE_ARGS PoolStringArray() // User CLI args unsupported in Godot 3
#define TIME_REALTIME OS::get_singleton()->get_system_time_msecs() * 1000000
#define TIME_MONOTONIC OS::get_singleton()->get_ticks_usec() * 1000
#define NULL_VARIANT Variant()
#define PAGE_SIZE 65536
#define GODOT_WASM_EXTERNREF 128 // Different enums in Wasmer & Wasmtime (see https://github.com/wasmerio/wasmer/issues/5082)

#endif
