#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <sys/types.h>
#include "./embind.h"
#include "core/string/ustring.h"
#include "modules/wasm/src/defs.h"
#include "modules/wasm/src/wasm-memory.h"
#include "wasi-shim.h"
#include "wasm.h"

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

    typedef std::tuple<
            const std::vector<wasm_valkind_enum>,
            const std::vector<wasm_valkind_enum>,
            const wasm_func_callback_with_env_t>
            callback_signature;

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
    wasm_trap_t* __register_function(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 8, "Invalid arguments args_get", godot_wasm::wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      auto wasm = (Wasm*)env;
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

      /*
        for i=1,argCount do
            local rawType = buffer.readi32(memory.data, rawArgTypesAddr + i*4)
            local typeInfo = types[rawType]
            args[i] = rawType
        end
    */
      for (auto i = 0; i < arg_count; i++) {
        memory->seek(types + i * 4);
        auto curr_type = memory->get_8();
      }

      auto out = read_null_terminated_string(memory, types);
      if (memory == NULL) return godot_wasm::wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");

      return godot_wasm::wasi_result(results);
    }

    // args = rawType, nameptr, trueValue, falseValue
    wasm_trap_t* __register_bool(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 4, "Invalid arguments args_get", godot_wasm::wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      auto wasm = (Wasm*)env;
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
      return godot_wasm::wasi_result(results);
    }

    wasm_trap_t* __register_bigint(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 5, "Invalid arguments args_get", godot_wasm::wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      auto wasm = (Wasm*)env;
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);
      return godot_wasm::wasi_result(results);
    }
    wasm_trap_t* __register_float(uint rawType, uint name, uint size) {}
    wasm_trap_t* __register_integer(uint primitiveType, uint name, uint size, uint minRange, uint maxRange) {}
    wasm_trap_t* __register_memory_view(uint rawType, uint dataTypeIndex, uint name) {}
    wasm_trap_t* __register_std_string(uint rawType, uint name) {}
    wasm_trap_t* __register_void(uint rawType, uint name) {}
    wasm_trap_t* __abort_js(uint rawType) {}
    wasm_trap_t* __register_emval(uint rawType) {}
    wasm_trap_t* __register_std_wstring(uint rawType, uint charSize, uint name) {}
    wasm_trap_t* __register_std_string(uint rawType, uint charSize, uint name) {}
    wasm_trap_t* __emscripten_resize_heap(uint requestedSize) {}

    std::map<std::string, callback_signature> embind_sig{
      { "env._embind_register_function", { { WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32 }, {}, __register_function } },
      { "env._embind_register_bool", { { WASM_I32, WASM_I32, WASM_I32, WASM_I32 }, {}, __register_bool } },
    };
  } //namespace

  namespace godot_wasm {
    wasm_func_t* get_embind_callback(wasm_store_t* store, Wasm* wasm, const String name) {
      std::string key = std::string(name.utf8().get_data());
      return embind_sig.count(key) ? wasi_callback(store, wasm, embind_sig[key]) : NULL;
    }
  } //namespace godot_wasm
} //namespace godot
