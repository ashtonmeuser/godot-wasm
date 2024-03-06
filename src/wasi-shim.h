#ifndef GODOT_WASM_WASI_SHIM_H
#define GODOT_WASM_WASI_SHIM_H

#include <functional>
#include "wasm.h"
#include "defs.h"

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    wasm_func_t* get_wasi_callback(wasm_store_t* store, Wasm* wasm, const String name);
  }
}

#endif
