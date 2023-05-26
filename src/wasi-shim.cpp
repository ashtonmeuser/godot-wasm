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
      Wasm* wasm = (Wasm*)env;
      byte_t* data = wasm_memory_data(wasm->stream.ptr()->memory);
      int32_t fd = args->data[0].of.i32;
      int32_t offset_iov = args->data[1].of.i32;
      int32_t count_iov = args->data[2].of.i32;
      int32_t offset_written = args->data[3].of.i32;
      uint32_t written = 0;
      for (auto i = 0; i < count_iov; i++) {
        wasi_io_vector iov = get_io_vector(wasm->stream.ptr()->memory, offset_iov, i);
        std::string message = std::string(data + iov.offset, data + iov.offset + iov.length);
        if (iov.length == 1 && message == "\u000A") continue; // Skip line feed
        fd == 1 ? PRINT(message.c_str()) : PRINT_ERROR(message.c_str());
        written += iov.length;
      }
      memcpy(data + offset_written, &written, sizeof(int32_t));
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = 0;
      return NULL;
    }

    // WASI proc_exit: [I32] -> []
    wasm_trap_t* wasi_proc_exit(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 1 || results->size != 0, "Invalid call WASI proc_exit", NULL);
      Wasm* wasm = (Wasm*)env;
      wasm->exit(args->data[0].of.i32);
      return NULL;
    }

    // WASI environ_sizes_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_environ_sizes_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid call WASI environ_sizes_get", NULL);
      Wasm* wasm = (Wasm*)env;
      byte_t* data = wasm_memory_data(wasm->stream.ptr()->memory);
      int32_t offset_count = args->data[0].of.i32;
      int32_t offset_length = args->data[1].of.i32;
      int32_t zero = 0;
      memcpy(data + offset_count, &zero, sizeof(int32_t));
      memcpy(data + offset_length, &zero, sizeof(int32_t));
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = 0;
      return NULL;
    }

    // WASI environ_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_environ_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid call WASI environ_get", NULL);
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = 0;
      return NULL;
    }

    // WASI random_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_random_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid call WASI random_get", NULL);
      Wasm* wasm = (Wasm*)env;
      byte_t* data = wasm_memory_data(wasm->stream.ptr()->memory);
      int32_t offset = args->data[0].of.i32;
      int32_t length = args->data[1].of.i32;
      PackedByteArray bytes = RANDOM_BYTES(length);
      memcpy(data + offset, BYTE_ARRAY_POINTER(bytes), length);
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = 0;
      return NULL;
    }

    // WASI clock_time_get: [I32, I64, I32] -> [I32]
    wasm_trap_t* wasi_clock_time_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 3 || results->size != 1, "Invalid call WASI clock_time_get", NULL);
      Wasm* wasm = (Wasm*)env;
      byte_t* data = wasm_memory_data(wasm->stream.ptr()->memory);
      int32_t offset = args->data[2].of.i32;
      int64_t t = UNIX_TIME_NS;
      memcpy(data + offset, &t, sizeof(t));
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = 0;
      return NULL;
    }

    godot_wasm::wasi_callback wasi_factory_factory(const std::vector<wasm_valkind_enum> p_kinds, const std::vector<wasm_valkind_enum> r_kinds, wasm_func_callback_with_env_t c) {
      auto p_types = new std::vector<wasm_valtype_t*>;
      auto r_types = new std::vector<wasm_valtype_t*>;
      for (auto &it: p_kinds) p_types->push_back(wasm_valtype_new(it));
      for (auto &it: r_kinds) r_types->push_back(wasm_valtype_new(it));
      wasm_valtype_vec_t params = { p_types->size(), p_types->data() };
      wasm_valtype_vec_t results = { r_types->size(), r_types->data() };
      wasm_functype_t* t = wasm_functype_new(&params, &results);
      return [t, c](wasm_store_t* s, Wasm* w) { return wasm_func_as_extern(wasm_func_new_with_env(s, t, c, w, NULL)); };
    }

    std::map<std::string, godot_wasm::wasi_callback> factories {
      { "wasi_snapshot_preview1.fd_write", wasi_factory_factory({WASM_I32, WASM_I32, WASM_I32, WASM_I32}, {WASM_I32}, wasi_fd_write) },
      { "wasi_snapshot_preview1.proc_exit", wasi_factory_factory({WASM_I32}, {}, wasi_proc_exit) },
      { "wasi_snapshot_preview1.environ_sizes_get", wasi_factory_factory({WASM_I32, WASM_I32}, {WASM_I32}, wasi_environ_sizes_get) },
      { "wasi_snapshot_preview1.environ_get", wasi_factory_factory({WASM_I32, WASM_I32}, {WASM_I32}, wasi_environ_get) },
      { "wasi_snapshot_preview1.random_get", wasi_factory_factory({WASM_I32, WASM_I32}, {WASM_I32}, wasi_random_get) },
      { "wasi_snapshot_preview1.clock_time_get", wasi_factory_factory({WASM_I32, WASM_I64, WASM_I32}, {WASM_I32}, wasi_clock_time_get) },
    };
  }

  namespace godot_wasm {
    wasi_callback get_wasi_import(const String name) {
      std::string s = std::string(name.utf8().get_data());
      return factories.count(s) ? factories[s] : NULL;
    }
  }
}
