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

#else

#include "register_types.h"
#include "src/godot-wasm.h"
#include "src/stream-peer-wasm.h"

using namespace godot;

void initialize_wasm_module(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }

  ClassDB::register_class<Wasm>();
  ClassDB::register_class<StreamPeerWasm>();
}

void uninitialize_wasm_module(ModuleInitializationLevel p_level) {
  if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
    return;
  }
}

extern "C" {
  GDExtensionBool GDE_EXPORT wasm_library_init(const GDExtensionInterface *p_interface, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_interface, p_library, r_initialization);

    init_obj.register_initializer(initialize_wasm_module);
    init_obj.register_terminator(uninitialize_wasm_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
  }
}

#endif
