#ifndef GODOT_WASM_DEFS_H
#define GODOT_WASM_DEFS_H

namespace {
  #ifdef __GNUC__
    #define UNLIKELY(cond) __builtin_expect(!!(cond), 0)
  #else
    #define UNLIKELY(cond) cond
  #endif
  #define GDERROR(error) GODOT_##error
  #define PRINT_ERROR(message) godot::Godot::print_error("Godot Wasm: " + godot::String(message), __func__, __FILE__, __LINE__);
  #define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
  #define FAIL_IF(cond, message, ret) if (UNLIKELY(cond)) FAIL(message, ret)
  #define NULL_VARIANT godot::Variant()
  #define PAGE_SIZE 65536
}

#endif
