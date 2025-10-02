#include <cstdint>
#include <functional>
#include <map>
#include <sys/types.h>
#include "core/string/ustring.h"
#include "../wasm-memory.h"
#include "../wasm.h"
#include "extension.h"

namespace godot {
  namespace {
    struct CustomType {
      bool is_void;
      String name;
      int arg_parse_advance;
      std::function<uint32_t(bool)> from_wire_type;
      std::function<uint32_t(bool)> to_wire_type;
    };
    std::map<uint32_t, CustomType> types;

    void register_type(uint32_t raw_type, CustomType custom_type) {
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
      auto types = args->data[2].of.i32;
      auto signature = args->data[3].of.i32;
      auto invoker = args->data[4].of.i32;
      auto function = args->data[5].of.i32;
      auto is_async = !!args->data[6].of.i32;
      auto is_non_null_return = !!args->data[7].of.i32;

      if (!is_non_null_return) arg_count--;

      for (auto i = 0; i < arg_count; i++) {
        memory->seek(types + i * 4);
        auto curr_type = memory->get_8();
      }

      auto out = read_null_terminated_string(memory, types);
      return NULL;
    }

    // args = rawType, nameptr, trueValue, falseValue
    wasm_trap_t* register_bool(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 4, "Invalid arguments args_get", NULL);
      auto memory = wasm->get_memory().ptr();
      auto raw_type = args->data[1].of.i32;
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);

      auto true_value = args->data[2].of.i32;
      auto false_value = args->data[3].of.i32;

      CustomType curr_type;
      curr_type.name = name;
      curr_type.is_void = false;
      curr_type.arg_parse_advance = 8;
      curr_type.from_wire_type = [](bool wt) { return wt; };
      curr_type.to_wire_type = [true_value, false_value](bool wt) {
        if (wt) return true_value;
        return false_value;
      };

      register_type(raw_type, curr_type);
      return NULL;
    }

    wasm_trap_t* register_bigint(Wasm* wasm, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 5, "Invalid arguments args_get", NULL);
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);
      return NULL;
    }

    wasm_trap_t* register_float(uint rawType, uint name, uint size) {}
    wasm_trap_t* register_integer(uint primitiveType, uint name, uint size, uint minRange, uint maxRange) {}
    wasm_trap_t* register_memory_view(uint rawType, uint dataTypeIndex, uint name) {}
    wasm_trap_t* register_std_string(uint rawType, uint name) {}
    wasm_trap_t* register_void(uint rawType, uint name) {}
    wasm_trap_t* abort_js(uint rawType) {}
    wasm_trap_t* register_emval(uint rawType) {}
    wasm_trap_t* register_std_wstring(uint rawType, uint charSize, uint name) {}
    wasm_trap_t* register_std_string(uint rawType, uint charSize, uint name) {}
    wasm_trap_t* emscripten_resize_heap(uint requestedSize) {}
  } //namespace

  namespace godot_wasm {
    class Embind: public Extension {
    public:
      Embind(Wasm* wasm): Extension(wasm) {
        register_callback("env._embind_register_function",
                          { WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32 },
                          {},
                          register_function);
        register_callback("env._embind_register_bool",
                          { WASM_I32, WASM_I32, WASM_I32, WASM_I32 },
                          {},
                          register_bool);
      }
    };
  } //namespace godot_wasm
} //namespace godot
