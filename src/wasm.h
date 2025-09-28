#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <map>
#include <wasm.h>
#include "defs.h"
#include "wasm-memory.h"

namespace godot {
  namespace godot_wasm {
    struct ContextExtern;
    struct ContextFuncImport;
    struct ContextFuncExport;
    struct ContextMemory;
  } //namespace godot_wasm

  class Wasm: public RefCounted {
    GDCLASS(Wasm, RefCounted);

    private:
      wasm_module_t* module;
      wasm_instance_t* instance;
      godot_wasm::ContextMemory* memory_context;
      PackedStringArray extensions;
      Ref<WasmMemory> memory;
      std::map<String, godot_wasm::ContextFuncImport> import_funcs;
      std::map<String, godot_wasm::ContextExtern> export_globals;
      std::map<String, godot_wasm::ContextFuncExport> export_funcs;
      void reset_instance();
      godot_error map_names();
      wasm_func_t* create_callback(godot_wasm::ContextFuncImport* context);

  public:
    static void REGISTRATION_METHOD();
    Wasm();
    ~Wasm();
    void _init();

      void exit(int32_t code);
      godot_error compile(PackedByteArray bytecode);
      godot_error instantiate(const Dictionary import_map);
      godot_error load(PackedByteArray bytecode, const Dictionary import_map);
      Dictionary inspect() const;
      Variant function(String name, Array args) const;
      Variant global(String name) const;
      Ref<WasmMemory> get_memory() const;
      void set_extensions(const PackedStringArray &extension_names);
      PackedStringArray get_extensions() const;
  };
} //namespace godot

#endif
