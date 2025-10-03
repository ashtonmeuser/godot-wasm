#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <sys/types.h>
#include "core/string/ustring.h"
#include "../wasm-memory.h"
#include "../wasm.h"
#include "core/typedefs.h"
#include "core/variant/variant.h"
#include "extension.h"
#include "modules/wasm/src/defs.h"

namespace godot {
  namespace {
    struct CustomType {
      String name;
      int arg_parse_advance = 8;
      bool is_void = false;
      bool is_enum = false;
      bool is_em_val = false;

      std::function<uint32_t(uint32_t)> from_wire_type = [](uint32_t wt) { return wt; };
      std::function<uint32_t(bool, Variant)> to_wire_type;
    };

    struct CustomFunction {
      int32_t raw_invoker;
      int32_t fn;
	}

    std::map<uint32_t, CustomType> types;
    std::map<uint32_t, Variant> ids;

    void register_type(uint32_t raw_type, CustomType custom_type) {
      if (unlikely(types.count(raw_type) > 0)) {
        print_error("Godot Wasm: " + String("Type already registered"));
        return;
      }
      types[raw_type] = custom_type;
    }

    String read_null_terminated_string(WasmMemory* memory, int32_t ptr) {
      auto curr_pos = ptr;
      char curr_char;
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

      auto name = read_null_terminated_string(memory, args->data[0].of.i32);
      auto arg_count = args->data[1].of.i32;
      auto arg_types = args->data[2].of.i32;
      auto signature = read_null_terminated_string(memory, args->data[3].of.i32);
      auto invoker = args->data[4].of.i32;
      auto function = args->data[5].of.i32;
      auto is_async = !!args->data[6].of.i32;
      auto is_non_null_return = !!args->data[7].of.i32;

      if (!is_non_null_return) arg_count--;

      std::vector<CustomType> func_args;

      for (auto i = 0; i < arg_count; i++) {
        memory->seek(arg_types + i * 4);
        auto curr_type = memory->get_32();
        FAIL_IF(types.count(curr_type) == 0, "Type not found", NULL);
        func_args.push_back(types[curr_type]);
      }

      return NULL;
    }

    // args = rawType, nameptr, trueValue, falseValue
    wasm_trap_t* register_bool(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto memory = wasm->get_memory().ptr();

      auto raw_type = args->data[0].of.i32;
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);
      auto true_value = args->data[2].of.i32;
      auto false_value = args->data[3].of.i32;

      CustomType curr_type{
        .name = name,
        .to_wire_type = [true_value, false_value](bool destructors, Variant wt) {
          if (wt) return true_value;
          return false_value; }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_bigint(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);
      return NULL;
    }

    // rawType, nameptr, size, minr, maxr
    wasm_trap_t* register_float(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto memory = wasm->get_memory().ptr();
      auto raw_type = args->data[0].of.i32;
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);

      auto min_value = args->data[2].of.i32;
      auto max_value = args->data[3].of.i32;

      CustomType curr_type{
        .name = name,
        .to_wire_type = [](bool destructors, Variant wt) { return wt; }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_integer(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto memory = wasm->get_memory().ptr();
      auto raw_type = args->data[0].of.i32;
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);

      auto min_value = args->data[2].of.i32;
      auto max_value = args->data[3].of.i32;

      CustomType curr_type{
        .name = name,
        .to_wire_type = [](bool destructors, Variant wt) { return wt; }
      };

      register_type(raw_type, curr_type);
      return NULL;
    }
    wasm_trap_t* register_emval(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      CustomType curr_type{
        .name = "EmVal",
        .to_wire_type = [](bool destructors, Variant wt) {
          auto idx = ids.size();
          ids[idx] = wt;
          return idx;
        }
      };

      register_type(args->data[0].of.i32, curr_type);
      return NULL;
    }

    wasm_trap_t* register_void(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);

      CustomType curr_type{
        .name = name,
        .arg_parse_advance = 0,
        .is_void = true,
        .from_wire_type = [](Variant wt) { return NULL; },
        .to_wire_type = [](bool destructors, Variant wt) { return NULL; }
      };

      register_type(args->data[0].of.i32, curr_type);
      return NULL;
    }

    wasm_trap_t* register_memory_view(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);
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
