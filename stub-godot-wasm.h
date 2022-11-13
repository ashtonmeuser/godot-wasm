#ifndef GODOT_WASM_H
#define GODOT_WASM_H

#include <vector>
#include <Godot.hpp>
#include "wasmer.h"

namespace godot {
class Wasm : public Reference {
    GODOT_CLASS(Wasm, Reference)

private:

public:
    static void _register_methods();
    Wasm();
    ~Wasm();
    void _init();
};
}

#endif
