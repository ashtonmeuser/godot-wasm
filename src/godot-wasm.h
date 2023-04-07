#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <string>
#include <vector>
#include <stdexcept>
#include <Godot.hpp>
#include "wasmer.h"
#include "defs.h"
#include "stream-peer-wasm.h"

namespace godot {
  class Wasm : public Reference {
    GODOT_CLASS(Wasm, Reference)

    private:
      wasm_engine_t* engine;
      wasm_store_t* store;
      wasm_module_t* module;
      wasm_instance_t* instance;
      Dictionary functions;
      Dictionary globals;
      uint16_t memory_index;
      void map_names();

    public:
      static void _register_methods();
      Wasm();
      ~Wasm();
      void _init();
      godot_error compile(PoolByteArray bytecode);
      godot_error instantiate();
      godot_error load(PoolByteArray bytecode);
      Dictionary inspect();
      Variant function(String name, Array args);
      Variant global(String name);
      uint64_t mem_size();
      Ref<StreamPeerWasm> stream;
  };
}

#endif
