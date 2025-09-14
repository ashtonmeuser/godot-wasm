#ifndef WASI_SHIM_H
#define WASI_SHIM_H

#include <functional>
#if defined(WASM_RUNTIME_wamr)
#include "wasm_c_api.h"
#else
#include "wasm.h"
#endif
#include "defs.h"

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    wasm_func_t* get_wasi_callback(wasm_store_t* store, Wasm* wasm, const String name);
  }
}

#endif
