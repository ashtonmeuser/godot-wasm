#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <map>
#include "wasm.h"
#include "defs.h"
#include "wasm-memory.h"

namespace godot {
  namespace godot_wasm {
    struct ContextExtern;
    struct ContextFuncImport;
    struct ContextFuncExport;
    struct ContextMemory;
  }

  class Wasm : public RefCounted {
    GDCLASS(Wasm, RefCounted);

    private:
      wasm_module_t* module;
      wasm_instance_t* instance;
      godot_wasm::ContextMemory* memory_context;
      Dictionary permissions;
      Ref<WasmMemory> memory;
      std::map<String, godot_wasm::ContextFuncImport> import_funcs;
      std::map<String, godot_wasm::ContextExtern> export_globals;
      std::map<String, godot_wasm::ContextFuncExport> export_funcs;
      void reset_instance();
      GODOT_WASM_ERROR map_names();
      wasm_func_t* create_callback(godot_wasm::ContextFuncImport* context);

    public:
      static void GODOT_WASM_REGISTRATION_METHOD();
      Wasm();
      ~Wasm();
      void _init();
      void exit(int32_t code);
      GODOT_WASM_ERROR compile(PackedByteArray bytecode);
      GODOT_WASM_ERROR instantiate(const Dictionary import_map);
      GODOT_WASM_ERROR load(PackedByteArray bytecode, const Dictionary import_map);
      Dictionary inspect() const;
      Variant function(String name, Array args) const;
      Variant global(String name) const;
      Ref<WasmMemory> get_memory() const;
      void set_permissions(const Dictionary &update);
      Dictionary get_permissions() const;
      bool has_permission(String permission) const;
  };
}

#endif
