#include <string>
#include <map>
#include "wasi-shim.h"
#include "godot-wasm.h"

namespace godot {
  namespace {
    struct wasi_io_vector {
      int32_t offset;
      int32_t length;
    };

    wasi_io_vector get_io_vector(wasm_memory_t* memory, int32_t offset, int32_t index = 0) {
      wasi_io_vector iov;
      byte_t* data = wasm_memory_data(memory) + offset + index * sizeof(wasi_io_vector);
      memcpy(&iov, data, sizeof(wasi_io_vector));
      return iov;
    }

    // WASI fd_write: [I32, I32, I32, I32] -> [I32]
    wasm_trap_t* wasi_fd_write(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 4 || results->size != 1, "Invalid call WASI fd_write", NULL);
      Wasm* gw = (Wasm*)env;
      wasm_memory_t* memory = gw->stream.ptr()->memory;
      int32_t fd = args->data[0].of.i32;
      int32_t offset = args->data[1].of.i32;
      int32_t length = args->data[2].of.i32;
      uint32_t written = 0;
      for (uint16_t i = 0; i < length; i++) {
        wasi_io_vector iov = get_io_vector(memory, offset, i);
        byte_t* data = wasm_memory_data(memory) + iov.offset;
        std::string message = std::string(data, data + iov.length);
        if (iov.length == 1 && message == "\u000A") continue; // Skip line feed
        fd == 1 ? PRINT(message.c_str()) : PRINT_ERROR(message.c_str());
        written += iov.length;
      }
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = written;
      return NULL;
    }

    // WASI proc_exit: [I32] -> []
    wasm_trap_t* wasi_proc_exit(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 1 || results->size != 0, "Invalid call WASI proc_exit", NULL);
      Wasm* gw = (Wasm*)env;
      gw->exit(args->data[0].of.i32);
      return NULL;
    }

    wasm_extern_t* factory_wasi_fd_write(wasm_store_t* store, Wasm* gw) {
      wasm_valtype_t* p[] = { wasm_valtype_new_i32(), wasm_valtype_new_i32(), wasm_valtype_new_i32(), wasm_valtype_new_i32() };
      wasm_valtype_t* r[] = { wasm_valtype_new_i32() };
      wasm_valtype_vec_t params, results;
      wasm_valtype_vec_new(&params, 4, p);
      wasm_valtype_vec_new(&results, 1, r);
      wasm_functype_t* type = wasm_functype_new(&params, &results);
      return wasm_func_as_extern(wasm_func_new_with_env(store, type, wasi_fd_write, gw, NULL));
    };

    wasm_extern_t* factory_wasi_proc_exit(wasm_store_t* store, Wasm* gw) {
      wasm_valtype_t* p[] = { wasm_valtype_new_i32() };
      wasm_valtype_vec_t params, results;
      wasm_valtype_vec_new(&params, 1, p);
      wasm_valtype_vec_new_empty(&results);
      wasm_functype_t* type = wasm_functype_new(&params, &results);
      return wasm_func_as_extern(wasm_func_new_with_env(store, type, wasi_proc_exit, gw, NULL));
    };

    std::map<std::string, godot_wasm::wasi_callback> factories {
      { "wasi_snapshot_preview1.fd_write", factory_wasi_fd_write },
      { "wasi_snapshot_preview1.proc_exit", factory_wasi_proc_exit },
    };
  }

  namespace godot_wasm {
    wasi_callback get_wasi_import(const String name) {
      std::string s = std::string(name.utf8().get_data());
      return factories.count(s) ? factories[s] : NULL;
    }
  }
}
