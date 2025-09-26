#ifndef WASI_SHIM_H
#define WASI_SHIM_H

#include "defs.h"
#include <wasm.h>
#include <functional>

// See https://github.com/WebAssembly/wasi-libc/blob/main/libc-bottom-half/headers/public/wasi/api.h
#define __WASI_CLOCKID_REALTIME (UINT32_C(0)) // The clock measuring real time
#define __WASI_CLOCKID_MONOTONIC (UINT32_C(1)) // The store-wide monotonic clock
#define __WASI_ERRNO_SUCCESS (UINT16_C(0)) // No error occurred
#define __WASI_ERRNO_ACCES (UINT16_C(2)) // [sic] Permission denied
#define __WASI_ERRNO_INVAL (UINT16_C(28)) // Invalid argument
#define __WASI_ERRNO_IO (UINT16_C(29)) // I/O error

namespace godot {
class Wasm; // Forward declare to avoid circular dependency

namespace godot_wasm {
typedef std::tuple<const std::vector<wasm_valkind_enum>, const std::vector<wasm_valkind_enum>, const wasm_func_callback_with_env_t> callback_signature;

wasm_func_t *wasi_callback(wasm_store_t *store, Wasm *wasm, callback_signature signature);
wasm_func_t *get_wasi_callback(wasm_store_t *store, Wasm *wasm, const String name);
wasm_trap_t *wasi_result(wasm_val_vec_t *results, int32_t value = __WASI_ERRNO_SUCCESS, const char *message = nullptr);
} //namespace godot_wasm
} //namespace godot

#endif
