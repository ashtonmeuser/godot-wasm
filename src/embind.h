#ifndef EMBIND_H
#define EMBIND_H
#include "./wasm.h"
#include "core/object/ref_counted.h"
#include "modules/wasm/src/wasi-shim.h"
#include <wasm.h>

namespace godot {
class Wasm; // Forward declare to avoid circular dependency

namespace godot_wasm {
wasm_func_t *get_embind_callback(wasm_store_t *store, Wasm *wasm, const String name);
}
} //namespace godot
#endif
