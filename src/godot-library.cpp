#ifndef GODOT_MODULE

#include "wasm.h"
#include "wasm-memory.h"

extern "C" void GDN_EXPORT wasm_gdnative_init(godot_gdnative_init_options *o) {
  godot::Godot::gdnative_init(o);
}

extern "C" void GDN_EXPORT wasm_gdnative_terminate(godot_gdnative_terminate_options *o) {
  godot::Godot::gdnative_terminate(o);
}

extern "C" void GDN_EXPORT wasm_nativescript_init(void *handle) {
  godot::Godot::nativescript_init(handle);

  godot::register_class<godot::Wasm>();
  godot::register_class<godot::WasmMemory>();
}

#endif
