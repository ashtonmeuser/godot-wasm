#ifndef WASI_EXTENSION_H
#define WASI_EXTENSION_H

#include <functional>
#include <wasm.h>
#include "../defs.h"

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    typedef std::tuple<const std::vector<wasm_valkind_enum>, const std::vector<wasm_valkind_enum>, const wasm_func_callback_with_env_t> callback_signature;

    wasm_functype_t* create_functype(std::vector<wasm_valkind_enum> p_params, std::vector<wasm_valkind_enum> p_results);
    wasm_func_t* wasi_callback(wasm_store_t* store, Wasm* wasm, callback_signature signature);
    wasm_func_t* get_wasi_callback(wasm_store_t* store, Wasm* wasm, const String name);
    wasm_trap_t* wasi_result(wasm_val_vec_t* results, int32_t value = __WASI_ERRNO_SUCCESS, const char* message = nullptr);
  } //namespace godot_wasm
} //namespace godot

#endif
