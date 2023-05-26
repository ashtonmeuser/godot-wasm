#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <map>
#include "wasmer.h"
#include "defs.h"
#include "stream-peer-wasm.h"

namespace godot {
  namespace godot_wasm {
    struct context_extern;
    struct context_callback;
  }

  class Wasm : public RefCounted {
    GDCLASS(Wasm, RefCounted);

    private:
      wasm_engine_t* engine;
      wasm_store_t* store;
      wasm_module_t* module;
      wasm_instance_t* instance;
      uint16_t memory_index;
      Dictionary permissions;
      Ref<StreamPeerWasm> stream;
      std::map<String, godot_wasm::context_callback> import_funcs;
      std::map<String, godot_wasm::context_extern> export_globals;
      std::map<String, godot_wasm::context_extern> export_funcs;
      void reset();
      godot_error map_names();
      wasm_func_t* create_callback(godot_wasm::context_callback* context);

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
      uint64_t mem_size() const;
      Ref<StreamPeerWasm> get_stream() const;
      void set_permissions(const Dictionary &update);
      Dictionary get_permissions() const;
      bool has_permission(String permission) const;
  };
}

#endif
