#ifndef GODOT_WASM_STORE_H
#define GODOT_WASM_STORE_H

/*
Singleton Wasm C API store
The same store is used between all compiled Wasm modules
*/

#include <cstddef>
#include <wasm.h>
#include <wasmtime/store.h>

#define STORE ::godot_wasm::Store::instance().store
#define STORE_CONTEXT wasmtime_store_context(::godot_wasm::Store::instance().wasmtime_store)

namespace godot_wasm {
  struct Store {
    private:
      Store() {
        engine = wasm_engine_new();
        store = wasm_store_new(engine);
        wasmtime_store = wasmtime_store_new(engine, NULL, NULL);
      }

      ~Store() {
        wasm_store_delete(store);
        wasmtime_store_delete(wasmtime_store);
        wasm_engine_delete(engine);
      }

    public:
      wasm_engine_t* engine;
      wasm_store_t* store;
      wasmtime_store_t* wasmtime_store;

      static Store& instance() { // Public accessor
        static Store s;
        return s;
      }

      Store(const Store &) = delete; // Prevent copy constructor
      Store & operator = (const Store &) = delete; // Prevent assignment
  };
}

#endif
