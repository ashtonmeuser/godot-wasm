#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <string>
#include <vector>
#include <Godot.hpp>
#include "wasmer.h"

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
      wasm_memory_t* memory;
      void map_names();

    public:
      static void _register_methods();
      Wasm();
      ~Wasm();
      void _init();
      // godot_error compile(PoolByteArray bytecode);
      // godot_error instantiate(PoolByteArray bytecode);
      godot_error load(PoolByteArray bytecode);
      Dictionary inspect();
      Variant function(String name, Array args);
      Variant global(String name);
      Variant mem_read(uint8_t type, uint64_t offset, uint32_t length);
      uint64_t mem_write(Variant value, uint64_t offset);
  };
}

#endif
