#ifndef UTILS_H
#define UTILS_H

#include "wasm.h"
namespace godot {
  namespace godot_wasm {
    inline wasm_val_t error_value(const char* message) {
      PRINT_ERROR(message);
      wasm_val_t value;
      value.kind = WASM_EXTERNREF;
      value.of.ref = NULL;
      return value;
    }

    Variant decode_variant(wasm_val_t value);
    wasm_val_t encode_variant(Variant variant, wasm_valkind_t kind);
  } //namespace godot_wasm
} //namespace godot
#endif
