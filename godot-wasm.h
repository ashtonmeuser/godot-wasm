#ifndef GODOT_WASM_H
#define GODOT_WASM_H

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
    Dictionary names;
    static Dictionary map_names(wasm_module_t* module);
    static Variant extract_variant(wasm_val_t value);

public:
    static void _register_methods();
    Wasm();
    ~Wasm();
    void _init();
    godot_error load(PoolByteArray bytecode);
    Variant function(String name, Array args);
    Variant global(String name);
};
}

#endif
