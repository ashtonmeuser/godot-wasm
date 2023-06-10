#ifndef WASI_SHIM_H
#define WASI_SHIM_H

#include <functional>
#include "wasmer.h"
#include "defs.h"

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    wasm_extern_t* get_wasi_import(wasm_store_t* store, Wasm* wasm, const String name);
  }
}

#endif
