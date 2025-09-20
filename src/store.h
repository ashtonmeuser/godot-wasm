#ifndef GODOT_WASM_STORE_H
#define GODOT_WASM_STORE_H

/*
Singleton Wasm C API store
The same store is used between all compiled Wasm modules
*/

#include "wasm.h"

#define STORE ::godot_wasm::Store::instance().store

namespace godot_wasm {
  struct Store {
    private:
      Store() {
        engine = wasm_engine_new();
        store = wasm_store_new(engine);
      }

      ~Store() {
        wasm_store_delete(store);
        wasm_engine_delete(engine);
      }

    public:
      wasm_engine_t* engine;
      wasm_store_t* store;

      static Store& instance() { // Public accessor
        static Store s;
        return s;
      }

      Store(const Store &) = delete; // Prevent copy constructor
      Store & operator = (const Store &) = delete; // Prevent assignment
  };
}

#endif
