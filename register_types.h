#ifndef GODOT_WASM_REGISTER_TYPES_H
#define GODOT_WASM_REGISTER_TYPES_H

// This define is only needed because the GODOT_MODULE define isn't detected here
// May be related to https://github.com/godotengine/godot/issues/75914
#ifdef GDEXTENSION
  #define NS godot
  #include <godot_cpp/core/class_db.hpp>
#else
  #define NS
  #include "modules/register_module_types.h"
  #include "core/object/class_db.h"
#endif

void initialize_wasm_module(NS::ModuleInitializationLevel p_level);
void uninitialize_wasm_module(NS::ModuleInitializationLevel p_level);

#endif
