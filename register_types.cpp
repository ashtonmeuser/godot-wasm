#ifdef GODOT_MODULE

#include "register_types.h"

#include "core/class_db.h"
#include "src/godot-wasm.h"
#include "src/stream-peer-wasm.h"

void register_wasm_types() {
    ClassDB::register_class<godot::Wasm>();
    ClassDB::register_class<godot::StreamPeerWasm>();
}

void unregister_wasm_types() { }

#endif
