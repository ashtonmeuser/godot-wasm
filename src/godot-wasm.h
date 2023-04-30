#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include "wasmer.h"
#include "defs.h"
#include "stream-peer-wasm.h"

namespace {
  struct context_extern;
  struct context_callback;
}

namespace godot {
  class Wasm : public Reference {
    GDCLASS(Wasm, Reference);

    private:
      wasm_engine_t* engine;
      wasm_store_t* store;
      wasm_module_t* module;
      wasm_instance_t* instance;
      uint16_t memory_index;
      std::map<String, context_callback> import_funcs;
      std::map<String, context_extern> export_globals;
      std::map<String, context_extern> export_funcs;
      godot_error map_names();
      wasm_func_t* create_callback(context_callback* context);

    public:
      static void REGISTRATION_METHOD();
      Wasm();
      ~Wasm();
      void _init();
      godot_error compile(PoolByteArray bytecode);
      godot_error instantiate(const Dictionary import_map);
      godot_error load(PoolByteArray bytecode, const Dictionary import_map);
      Dictionary inspect();
      Variant function(String name, Array args);
      Variant global(String name);
      uint64_t mem_size();
      Ref<StreamPeerWasm> stream;
      Ref<StreamPeerWasm> get_stream() const;
  };
}

#endif
