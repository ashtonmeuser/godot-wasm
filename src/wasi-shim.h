#ifndef WASI_SHIM_H
#define WASI_SHIM_H

#include "wasmer.h"
#include "defs.h"

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    typedef std::function<wasm_extern_t* (wasm_store_t*, Wasm*)> wasi_callback;

    wasi_callback get_wasi_import(const String name);
  }
}

#endif
