#include "utils.h"

namespace godot {
  namespace godot_wasm {
    Variant decode_variant(wasm_val_t value) {
      switch (value.kind) {
        case WASM_I32: return Variant(value.of.i32);
        case WASM_I64: return Variant(value.of.i64);
        case WASM_F32: return Variant(value.of.f32);
        case WASM_F64: return Variant(value.of.f64);
        default: FAIL("Unsupported Wasm type", NULL_VARIANT);
      }
    }

    wasm_val_t encode_variant(Variant variant, wasm_valkind_t kind) {
      wasm_val_t value;
      value.kind = kind;
      switch (variant.get_type()) {
        case Variant::INT:
          switch (kind) {
            case WASM_I32:
              value.of.i32 = (int32_t)variant;
              return value;
            case WASM_I64:
              value.of.i64 = (int64_t)variant;
              return value;
            default:
              return godot_wasm::error_value("Invalid target type for integer variant");
          }
        case Variant::FLOAT:
          switch (kind) {
            case WASM_F32:
              value.of.f32 = (float32_t)variant;
              return value;
            case WASM_F64:
              value.of.f64 = (float64_t)variant;
              return value;
            default:
              return godot_wasm::error_value("Invalid target type for float variant");
          }
        default:
          return godot_wasm::error_value("Unsupported Godot variant type");
      }
    }
  } //namespace godot_wasm
} //namespace godot
