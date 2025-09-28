#include <cstdint>
#include <string>
#include "./embind.h"
#include "core/string/ustring.h"
#include "modules/wasm/src/defs.h"
#include "modules/wasm/src/wasm-memory.h"
#include "wasi-shim.h"
#include "wasm.h"

namespace godot {
  namespace {
    typedef std::tuple<const std::vector<wasm_valkind_enum>, const std::vector<wasm_valkind_enum>, const wasm_func_callback_with_env_t> callback_signature;

    String read_null_terminated_string(WasmMemory* memory, int32_t ptr) {
      auto i = ptr;
      while (true) {
        memory->seek(i);
        auto curr_char = (char)memory->get_8();

        if (!curr_char) break;
        i++;
      }
      memory->seek(ptr);
      return memory->get_string(i - ptr);
    }
    // nameptr, argCount, rawArgTypesAddr, signature, rawInvoker, fn, isAsync, isNonnullReturn
    wasm_trap_t* __register_function(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 8, "Invalid arguments args_get", godot_wasm::wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      auto wasm = (Wasm*)env;
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[0].of.i32);
      auto arg_count = args->data[1].of.i32;
      if (memory == NULL) {
        return godot_wasm::wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      }

      return godot_wasm::wasi_result(results);
    }

    // args = rawType, nameptr, trueValue, falseValue
    wasm_trap_t* __register_bool(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 4, "Invalid arguments args_get", godot_wasm::wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      auto wasm = (Wasm*)env;
      auto memory = wasm->get_memory().ptr();
      auto name = read_null_terminated_string(memory, args->data[1].of.i32);
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
