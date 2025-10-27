#ifndef EMBIND_H
#define EMBIND_H
#include <cstdint>
#include <functional>
#include <map>
#include <sys/types.h>
#include "core/error/error_macros.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"
#include "core/variant/variant.h"
#include "extension.h"
#include "../defs.h"
#include "modules/wasm/src/utils.h"
#include "wasm.h"

namespace godot {
  namespace godot_wasm {
    struct CustomType {
      String name;
      int raw_type;
      int arg_parse_advance = 8;
      bool is_void = false;
      bool is_enum = false;
      bool is_em_val = false;

      std::function<Variant(uint32_t)> from_wire_type;
      std::function<wasm_val_t(bool, Variant)> to_wire_type;
    };
    struct EmbindFunction {
      String name;
      int32_t fn;
      std::vector<CustomType> args;
      int32_t ret;
    };
  } //namespace godot_wasm
  namespace {
    std::map<uint32_t, godot_wasm::CustomType> types;
    std::map<uint32_t, Variant> ids;

    void register_type(uint32_t raw_type, godot_wasm::CustomType custom_type) {
      if (unlikely(types.count(raw_type) > 0)) {
        print_error("Godot Wasm: " + String("Type already registered"));
        return;
      }
      types[raw_type] = custom_type;
    }

    String read_null_terminated_string(Wasm* wasm, int32_t ptr) {
      auto curr_pos = ptr;
      char curr_char;
      auto memory = wasm->get_memory().ptr();
      do {
        memory->seek(curr_pos);
        curr_char = memory->get_8();

        curr_pos++;
      } while (curr_char);

      memory->seek(ptr);
      return memory->get_string(curr_pos - ptr);
    }

    // nameptr, argCount, rawArgTypesAddr, signature, rawInvoker, fn, isAsync, isNonnullReturn
    wasm_trap_t* register_function(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 8, "Invalid arguments args_get", NULL);
      auto memory = wasm->get_memory().ptr();

      auto name = read_null_terminated_string(wasm, args->data[0].of.i32);
      auto arg_count = args->data[1].of.i32;
      auto arg_types = args->data[2].of.i32;
      auto signature = read_null_terminated_string(wasm, args->data[3].of.i32);
      auto invoker = args->data[4].of.i32;
      auto function = args->data[5].of.i32;
      auto is_async = !!args->data[6].of.i32;
      auto is_non_null_return = !!args->data[7].of.i32;

      std::vector<godot_wasm::CustomType> func_args;

      for (auto i = 0; i < arg_count; i++) {
        memory->seek(arg_types + i * 4);
        auto curr_type = memory->get_32();
        FAIL_IF(types.count(curr_type) == 0, "Type not found", NULL);
        func_args.push_back(types[curr_type]);
      }

      godot_wasm::EmbindFunction custom_function{
        .name = name,
        .fn = function,
        .args = func_args,
      };

      wasm->register_embind_function(custom_function);

      return NULL;
    }

    // args = rawType, nameptr, trueValue, falseValue
    wasm_trap_t* register_bool(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto raw_type = args->data[0].of.i32;
      auto name = read_null_terminated_string(wasm, args->data[1].of.i32);
      auto true_value = args->data[2].of.i32;
      auto false_value = args->data[3].of.i32;

      godot_wasm::CustomType curr_type{
        .name = name,
        .raw_type = raw_type,
        .to_wire_type = [raw_type, true_value, false_value](bool destructors, Variant wt) {
          wasm_val_t value;
          value.kind = raw_type;
          if (wt) value.of.f32 = true_value;
          else value.of.f32 = false_value;
          return value;
        }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_bigint(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto name = read_null_terminated_string(wasm, args->data[1].of.i32);
      return NULL;
    }

    // rawType, nameptr, size, minr, maxr
    wasm_trap_t* register_float(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto raw_type = args->data[0].of.i32;
      auto name = read_null_terminated_string(wasm, args->data[1].of.i32);

      auto min_value = args->data[2].of.i32;
      auto max_value = args->data[3].of.i32;

      godot_wasm::CustomType curr_type{
        .name = name,
        .raw_type = raw_type,
        .to_wire_type = [](bool destructors, Variant wt) {
          return godot_wasm::encode_variant(wt, WASM_F32);
        }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_integer(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto raw_type = args->data[0].of.i32;
      auto name = read_null_terminated_string(wasm, args->data[1].of.i32);

      auto min_value = args->data[2].of.i32;
      auto max_value = args->data[3].of.i32;

      godot_wasm::CustomType curr_type{
        .name = name,
        .raw_type = raw_type,
        .to_wire_type = [raw_type](bool destructors, Variant wt) {
          wasm_val_t value;
          value.kind = raw_type;
          return value; }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_emval(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto raw_type = args->data[0].of.i32;
      godot_wasm::CustomType curr_type{
        .name = "EmVal",
        .raw_type = raw_type,
        .to_wire_type = [raw_type](bool destructors, Variant wt) {
          wasm_val_t value;
          value.kind = raw_type;
          return value;
        }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_void(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto name = read_null_terminated_string(wasm, args->data[1].of.i32);
      auto raw_type = args->data[0].of.i32;

      godot_wasm::CustomType curr_type{
        .name = name,
        .raw_type = raw_type,
        .arg_parse_advance = 0,
        .is_void = true,
        .from_wire_type = [](Variant wt) { return NULL; },
        .to_wire_type = [raw_type](bool destructors, Variant wt) {
          wasm_val_t value;
          value.kind = raw_type;
          return value; }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_memory_view(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto name = read_null_terminated_string(wasm, args->data[1].of.i32);
      return NULL;
    }

    wasm_trap_t* register_std_string(uint rawType, uint name) {}
    wasm_trap_t* abort_js(uint rawType) {}
    wasm_trap_t* register_std_wstring(uint rawType, uint charSize, uint name) {}
    wasm_trap_t* register_std_string(uint rawType, uint charSize, uint name) {}
    wasm_trap_t* emscripten_resize_heap(uint requestedSize) {}

    const std::vector<wasm_valkind_enum> generate_arg_vector(const int amount, const wasm_valkind_enum to_repeat) {
      std::vector<wasm_valkind_enum> vec;
      for (auto i = 0; i < amount; i++) vec.push_back(to_repeat);
      return vec;
    }
  } //namespace

  namespace godot_wasm {
    class Embind: public Extension {
    public:
      Embind(Wasm* wasm):
              Extension(wasm) {
        register_callback("env._embind_register_function",
                          generate_arg_vector(8, WASM_I32),
                          {},
                          register_function);
        register_callback("env._embind_register_bool",
                          generate_arg_vector(4, WASM_I32),
                          {},
                          register_bool);
        register_callback("env._embind_register_integer",
                          generate_arg_vector(5, WASM_I32),
                          {},
                          register_integer);
        register_callback("env._embind_register_float",
                          generate_arg_vector(3, WASM_I32),
                          {},
                          register_float);
        register_callback("env._embind_register_emval",
                          generate_arg_vector(1, WASM_I32),
                          {},
                          register_emval);
        register_callback("env._embind_register_void",
                          generate_arg_vector(2, WASM_I32),
                          {},
                          register_void);
      }
    };
  } //namespace godot_wasm
} //namespace godot
#endif
