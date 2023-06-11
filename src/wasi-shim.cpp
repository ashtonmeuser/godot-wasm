#include <string>
#include <vector>
#include <map>
#include "wasi-shim.h"
#include "godot-wasm.h"

// See https://github.com/WebAssembly/wasi-libc/blob/main/libc-bottom-half/headers/public/wasi/api.h
#define __WASI_CLOCKID_REALTIME (UINT32_C(0)) // The clock measuring real time
#define __WASI_CLOCKID_MONOTONIC (UINT32_C(1)) // The store-wide monotonic clock
#define __WASI_ERRNO_SUCCESS (UINT16_C(0)) // No error occurred
#define __WASI_ERRNO_ACCES (UINT16_C(2)) // [sic] Permission denied
#define __WASI_ERRNO_INVAL (UINT16_C(28)) // Invalid argument
#define __WASI_ERRNO_IO (UINT16_C(29)) // I/O error

namespace godot {
  namespace {
    typedef std::tuple<const std::vector<wasm_valkind_enum>, const std::vector<wasm_valkind_enum>, const wasm_func_callback_with_env_t> callback_signature;

    struct wasi_io_vector {
      int32_t offset;
      int32_t length;
    };

    struct wasi_encoded_strings {
      int32_t count;
      int32_t length;
      std::vector<std::string> args;
    };

    wasi_io_vector get_io_vector(wasm_memory_t* memory, int32_t offset, int32_t index = 0) {
      wasi_io_vector iov;
      byte_t* data = wasm_memory_data(memory) + offset + index * sizeof(wasi_io_vector);
      memcpy(&iov, data, sizeof(wasi_io_vector));
      return iov;
    }

    template <class T> wasi_encoded_strings encode_args(T args) {
      wasi_encoded_strings encoded = { 0, 0, {} };
      String incomplete = "";
      for (auto i = 0; i < args.size(); i++) {
        String s = args[i];
        if (!s.begins_with("--")) { // Invalid; may be value for previous key
          if (incomplete == "") continue; // Ignore garbage
          s = incomplete + "=" + s; // Value for previous key
          incomplete = ""; // Reset incomplete key value pair
        } else { // Valid key or key value pair
          s = s.substr(2, -1); // Just key or key=value
          auto parts = s.split("=");
          if (parts.size() < 2) { // Incomplete; may have subsequent value
            incomplete = s;
            continue;
          }
          s = parts[0] + "=" + parts[1]; // Have both key and value
        }
        std::string bytes = std::string(s.utf8().get_data()) + '\0'; // Null termination
        encoded.count += 1;
        encoded.args.push_back(bytes);
        encoded.length += bytes.length();
      }
      return encoded;
    }

    wasm_trap_t* wasi_result(wasm_val_vec_t* results, int32_t value = __WASI_ERRNO_SUCCESS, const char* message = nullptr) {
      results->data[0].kind = WASM_I32;
      results->data[0].of.i32 = value;
      if (value == __WASI_ERRNO_SUCCESS) return NULL;
      wasm_message_t trap_message;
      wasm_name_new_from_string_nt(&trap_message, message);
      return wasm_trap_new(NULL, &trap_message);
    }

    // WASI fd_write: [I32, I32, I32, I32] -> [I32]
    wasm_trap_t* wasi_fd_write(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 4 || results->size != 1, "Invalid arguments fd_write", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      wasm_memory_t* memory = wasm->get_stream().ptr()->memory;
      if (memory == NULL) return wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      if (!wasm->has_permission("print")) return wasi_result(results, __WASI_ERRNO_ACCES, "Not permitted\0");
      byte_t* data = wasm_memory_data(memory);
      int32_t fd = args->data[0].of.i32;
      int32_t offset_iov = args->data[1].of.i32;
      int32_t count_iov = args->data[2].of.i32;
      int32_t offset_written = args->data[3].of.i32;
      uint32_t written = 0;
      for (auto i = 0; i < count_iov; i++) {
        wasi_io_vector iov = get_io_vector(memory, offset_iov, i);
        std::string message = std::string(data + iov.offset, data + iov.offset + iov.length);
        if (iov.length == 1 && message == "\u000A") continue; // Skip line feed
        fd == 1 ? PRINT(message.c_str()) : PRINT_ERROR(message.c_str());
        written += iov.length;
      }
      memcpy(data + offset_written, &written, sizeof(int32_t));
      return wasi_result(results);
    }

    // WASI proc_exit: [I32] -> []
    wasm_trap_t* wasi_proc_exit(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 1 || results->size != 0, "Invalid arguments proc_exit", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      if (!wasm->has_permission("exit")) return wasi_result(results, __WASI_ERRNO_ACCES, "Not permitted\0");
      wasm->exit(args->data[0].of.i32);
      return NULL;
    }

    // WASI args_sizes_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_args_sizes_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid arguments args_sizes_get", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      wasm_memory_t* memory = wasm->get_stream().ptr()->memory;
      if (memory == NULL) return wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      if (!wasm->has_permission("args")) return wasi_result(results, __WASI_ERRNO_ACCES, "Not permitted\0");
      byte_t* data = wasm_memory_data(memory);
      int32_t offset_count = args->data[0].of.i32;
      int32_t offset_length = args->data[1].of.i32;
      wasi_encoded_strings encoded = encode_args(CMDLINE_ARGS);
      memcpy(data + offset_count, &encoded.count, sizeof(int32_t));
      memcpy(data + offset_length, &encoded.length, sizeof(int32_t));
      return wasi_result(results);
    }

    // WASI args_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_args_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid arguments args_get", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      wasm_memory_t* memory = wasm->get_stream().ptr()->memory;
      if (memory == NULL) return wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      if (!wasm->has_permission("args")) return wasi_result(results, __WASI_ERRNO_ACCES, "Not permitted\0");
      byte_t* data = wasm_memory_data(memory);
      int32_t offset_environ = args->data[0].of.i32;
      int32_t offset_buffer = args->data[1].of.i32;
      wasi_encoded_strings encoded = encode_args(CMDLINE_ARGS);
      for (auto i = 0; i < encoded.count; i++) {
        std::string s = encoded.args[i];
        memcpy(data + offset_environ, &offset_buffer, sizeof(int32_t));
        memcpy(data + offset_buffer, s.c_str(), s.length());
        offset_environ += sizeof(int32_t);
        offset_buffer += s.length();
      }
      return wasi_result(results);
    }

    // WASI environ_sizes_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_environ_sizes_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid arguments environ_sizes_get", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      wasm_memory_t* memory = wasm->get_stream().ptr()->memory;
      if (memory == NULL) return wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      byte_t* data = wasm_memory_data(memory);
      int32_t offset_count = args->data[0].of.i32;
      int32_t offset_length = args->data[1].of.i32;
      int32_t zero = 0;
      memcpy(data + offset_count, &zero, sizeof(int32_t));
      memcpy(data + offset_length, &zero, sizeof(int32_t));
      return wasi_result(results);
    }

    // WASI environ_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_environ_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid arguments environ_get", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      return wasi_result(results);
    }

    // WASI random_get: [I32, I32] -> [I32]
    wasm_trap_t* wasi_random_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 2 || results->size != 1, "Invalid arguments random_get", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      wasm_memory_t* memory = wasm->get_stream().ptr()->memory;
      if (memory == NULL) return wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      if (!wasm->has_permission("random")) return wasi_result(results, __WASI_ERRNO_ACCES, "Not permitted\0");
      byte_t* data = wasm_memory_data(memory);
      int32_t offset = args->data[0].of.i32;
      int32_t length = args->data[1].of.i32;
      PackedByteArray bytes = RANDOM_BYTES(length);
      memcpy(data + offset, BYTE_ARRAY_POINTER(bytes), length);
      return wasi_result(results);
    }

    // WASI clock_time_get: [I32, I64, I32] -> [I32]
    wasm_trap_t* wasi_clock_time_get(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      FAIL_IF(args->size != 3 || results->size != 1, "Invalid arguments clock_time_get", wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
      Wasm* wasm = (Wasm*)env;
      wasm_memory_t* memory = wasm->get_stream().ptr()->memory;
      if (memory == NULL) return wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
      if (!wasm->has_permission("time")) return wasi_result(results, __WASI_ERRNO_ACCES, "Not permitted\0");
      byte_t* data = wasm_memory_data(memory);
      int32_t clock_id = args->data[0].of.i32;
      int32_t offset = args->data[2].of.i32;
      int64_t t = clock_id == __WASI_CLOCKID_REALTIME ? TIME_REALTIME : TIME_MONOTONIC;
      memcpy(data + offset, &t, sizeof(t));
      return wasi_result(results);
    }

    wasm_extern_t* wasi_callback(wasm_store_t* store, Wasm* wasm, callback_signature signature) {
      auto p_types = new std::vector<wasm_valtype_t*>;
      auto r_types = new std::vector<wasm_valtype_t*>;
      for (auto &it: std::get<0>(signature)) p_types->push_back(wasm_valtype_new(it));
      for (auto &it: std::get<1>(signature)) r_types->push_back(wasm_valtype_new(it));
      wasm_valtype_vec_t params = { p_types->size(), p_types->data() };
      wasm_valtype_vec_t results = { r_types->size(), r_types->data() };
      wasm_functype_t* functype = wasm_functype_new(&params, &results);
      auto callback = wasm_func_as_extern(wasm_func_new_with_env(store, functype, std::get<2>(signature), wasm, NULL));
      wasm_functype_delete(functype);
      return callback;
    }

    std::map<std::string, callback_signature> signatures {
      { "wasi_snapshot_preview1.fd_write", { {WASM_I32, WASM_I32, WASM_I32, WASM_I32}, {WASM_I32}, wasi_fd_write } },
      { "wasi_snapshot_preview1.proc_exit", { {WASM_I32}, {}, wasi_proc_exit } },
      { "wasi_snapshot_preview1.args_sizes_get", { {WASM_I32, WASM_I32}, {WASM_I32}, wasi_args_sizes_get } },
      { "wasi_snapshot_preview1.args_get", { {WASM_I32, WASM_I32}, {WASM_I32}, wasi_args_get } },
      { "wasi_snapshot_preview1.environ_sizes_get", { {WASM_I32, WASM_I32}, {WASM_I32}, wasi_environ_sizes_get } },
      { "wasi_snapshot_preview1.environ_get", { {WASM_I32, WASM_I32}, {WASM_I32}, wasi_environ_get } },
      { "wasi_snapshot_preview1.random_get", { {WASM_I32, WASM_I32}, {WASM_I32}, wasi_random_get } },
      { "wasi_snapshot_preview1.clock_time_get", { {WASM_I32, WASM_I64, WASM_I32}, {WASM_I32}, wasi_clock_time_get } },
    };
  }

  namespace godot_wasm {
    wasm_extern_t* get_wasi_import(wasm_store_t* store, Wasm* wasm, const String name) {
      std::string key = std::string(name.utf8().get_data());
      return signatures.count(key) ? wasi_callback(store, wasm, signatures[key]) : NULL;
    }
  }
}
