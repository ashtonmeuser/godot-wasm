#ifdef GODOT_MODULE

#include "register_types.h"
#include "core/class_db.h"
#include "src/wasm.h"
#include "src/wasm-memory.h"

void register_wasm_types() {
    ClassDB::register_class<godot::Wasm>();
    ClassDB::register_class<godot::WasmMemory>();
}

void unregister_wasm_types() { }

#endif
