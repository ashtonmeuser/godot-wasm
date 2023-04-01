#include "godot-wasm.h"

namespace {
  #define PRINT_ERROR(message) godot::Godot::print_error("Godot Wasm: " + godot::String(message), __func__, __FILE__, __LINE__);
  #define FAIL(message, ret) do { PRINT_ERROR(message); return ret; } while (0)
  #define FAIL_IF(cond, message, ret) do { if (__builtin_expect(!!(cond), 0)) FAIL(message, ret); } while (0)
  #define NULL_VARIANT godot::Variant()

  godot::Variant extract_variant(wasm_val_t value) {
    switch (value.kind) {
      case WASM_I32: return godot::Variant(value.of.i32);
      case WASM_I64: return godot::Variant(value.of.i64);
      case WASM_F32: return godot::Variant(value.of.f32);
      case WASM_F64: return godot::Variant(value.of.f64);
      case WASM_ANYREF: if (value.of.ref == NULL) return NULL_VARIANT;
      default: FAIL("Unsupported Wasm type", NULL_VARIANT);
    }
  }
}

namespace godot {
  void Wasm::_register_methods() {
    register_method("load", &Wasm::load);
    register_method("inspect", &Wasm::inspect);
    register_method("global", &Wasm::global);
    register_method("function", &Wasm::function);
    register_method("mem_read", &Wasm::mem_read);
    register_method("mem_write", &Wasm::mem_write);
  }

  Wasm::Wasm() {
    engine = wasm_engine_new();
    store = wasm_store_new(engine);
    module = NULL;
    instance = NULL;
    functions = Dictionary();
    globals = Dictionary();
    memory = NULL;
  }

  Wasm::~Wasm() {
    if (instance) wasm_instance_delete(instance);
    if (module) wasm_module_delete(module);
    if (store != nullptr) wasm_store_delete(store);
    if (engine != nullptr) wasm_engine_delete(engine);
  }

  void Wasm::_init() { }

  godot_error Wasm::load(PoolByteArray bytecode) {
    // Load binary
    wasm_byte_vec_t wasm_bytes;
    wasm_byte_vec_new_uninitialized(&wasm_bytes, bytecode.size());
    for (int i = 0; i < bytecode.size(); i++) wasm_bytes.data[i] = bytecode[i];

    // Validate binary
    FAIL_IF(!wasm_module_validate(store, &wasm_bytes), "Invalid binary", GODOT_ERR_INVALID_DATA);

    // Compile
    module = wasm_module_new(store, &wasm_bytes);
    wasm_byte_vec_delete(&wasm_bytes);
    FAIL_IF(module == NULL, "Compilation failed", GODOT_ERR_COMPILATION_FAILED);

    // Instantiate
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    instance = wasm_instance_new(store, module, &imports, NULL);
    FAIL_IF(instance == NULL, "Instantiation failed", GODOT_ERR_CANT_CREATE);

    // Map names to export indices
    map_names();

    return GODOT_OK;
  }

  Dictionary Wasm::inspect() {
    Dictionary dict;
    FAIL_IF(memory == NULL, "Inspection failed", dict);
    dict["functions"] = functions.keys();
    dict["globals"] = globals.keys();
    dict["memory"] = Variant((int)wasm_memory_data_size(memory));
    return dict;
  }

  Variant Wasm::global(String name) {
    // Validate instance and global name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!globals.has(name), "Unknown global name", NULL_VARIANT);

    // Retrieve exports
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    const wasm_global_t* global = wasm_extern_as_global(exports.data[(int)globals[name]]);
    FAIL_IF(global == NULL, "Failed to retrieve global export", NULL_VARIANT);

    // Extract result
    wasm_val_t result;
    wasm_global_get(global, &result);
    return extract_variant(result);
  }

  Variant Wasm::function(String name, Array args) {
    // Validate instance and function name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!functions.has(name), "Unknown function name", NULL_VARIANT);

    // Retrieve exports
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    const wasm_func_t* func = wasm_extern_as_func(exports.data[(int)functions[name]]);
    FAIL_IF(func == NULL, "Failed to retrieve function export", NULL_VARIANT);

    // Construct args
    std::vector<wasm_val_t> vect;
    for (int i = 0; i < args.size(); i++) {
      Variant val = args[i];
      switch (val.get_type()) {
        case Variant::INT:
          vect.push_back(WASM_I64_VAL((int64_t)val));
          break;
        case Variant::REAL:
          vect.push_back(WASM_F64_VAL((float64_t)val));
          break;
        default: FAIL("Invalid argument type", NULL_VARIANT);
      }
    }

    // Call function
    wasm_val_t results_val[1] = { WASM_INIT_VAL };
    wasm_val_vec_t f_args = { vect.size(), vect.data() };
    wasm_val_vec_t f_results = WASM_ARRAY_VEC(results_val);
    FAIL_IF(wasm_func_call(func, &f_args, &f_results), "Failed calling function", NULL_VARIANT);

    // Extract result
    wasm_val_t result = results_val[0];
    return extract_variant(result);
  }

  Variant Wasm::mem_read(uint8_t type, uint64_t offset, uint32_t length) {
    // Validate memory
    FAIL_IF(memory == NULL, "No memory", NULL_VARIANT);

    byte_t* data = wasm_memory_data(memory) + offset;
    if (type == Variant::Type::INT) {
      int64_t v;
      memcpy(&v, data, sizeof v);
      return Variant(v);
    } else if (type == Variant::Type::REAL) {
      float64_t v;
      memcpy(&v, data, sizeof v);
      return Variant(v);
    } else if (type == Variant::Type::BOOL) {
      int64_t v = mem_read(Variant::Type::INT, offset, length);
      return Variant(v ? true : false);
    } else if (type == Variant::Type::STRING) {
      std::string v(data, data + length);
      return String(v.c_str());
    } else if (type == Variant::Type::VECTOR2) {
      real_t x = mem_read(Variant::Type::REAL, offset, length);
      real_t y = mem_read(Variant::Type::REAL, offset + sizeof(float64_t), length);
      return Vector2(x, y);
    } else if (type == Variant::Type::VECTOR3) {
      real_t x = mem_read(Variant::Type::REAL, offset, length);
      real_t y = mem_read(Variant::Type::REAL, offset + sizeof(float64_t), length);
      real_t z = mem_read(Variant::Type::REAL, offset + sizeof(float64_t) * 2, length);
      return Vector3(x, y, z);
    } else if (type == Variant::Type::POOL_BYTE_ARRAY) {
      PoolByteArray v;
      v.resize(length);
      std::memcpy(v.write().ptr(), data, length);
      return v;
    } else if (type == Variant::Type::POOL_INT_ARRAY) {
      PoolIntArray v;
      for (uint32_t i = 0; i < length; i++) v.append((const int)mem_read(Variant::Type::INT, offset + i * sizeof(int64_t), length));
      return v;
    } else if (type == Variant::Type::POOL_REAL_ARRAY) {
      PoolRealArray v;
      for (uint32_t i = 0; i < length; i++) v.append((real_t)mem_read(Variant::Type::REAL, offset + i * sizeof(float64_t), length));
      return v;
    }
    return NULL_VARIANT;
  }

  uint64_t Wasm::mem_write(Variant value, uint64_t offset) {
    // Validate memory
    FAIL_IF(memory == NULL, "No memory", NULL_VARIANT);

    byte_t* data = wasm_memory_data(memory) + offset;
    const Variant::Type type = value.get_type();
    if (type == Variant::Type::INT) {
      int64_t v = value;
      byte_t* bytes = reinterpret_cast<byte_t*>(&v);
      size_t s = sizeof v;
      std::memcpy(data, bytes, s);
      return offset + s;
    } else if (type == Variant::Type::REAL) {
      float64_t v = value;
      byte_t* bytes = reinterpret_cast<byte_t*>(&v);
      size_t s = sizeof v;
      std::memcpy(data, bytes, s);
      return offset + s;
    } else if (type == Variant::Type::BOOL) {
      bool v = value;
      data[offset] = value ? 0x1 : 0x0;
      return offset + 1;
    } else if (type == Variant::Type::STRING) {
      String v = value;
      CharString c = v.utf8();
      const byte_t* bytes = c.get_data();
      size_t s = c.length();
      std::memcpy(data, bytes, s);
      return offset + s;
    } else if (type == Variant::Type::VECTOR2) {
      Vector2 v = value;
      offset = mem_write(Variant(v.x), offset);
      offset = mem_write(Variant(v.y), offset);
      return offset;
    } else if (type == Variant::Type::VECTOR3) {
      Vector3 v = value;
      offset = mem_write(Variant(v.x), offset);
      offset = mem_write(Variant(v.y), offset);
      offset = mem_write(Variant(v.z), offset);
      return offset;
    } else if (type == Variant::Type::ARRAY ||
              type == Variant::Type::POOL_INT_ARRAY ||
              type == Variant::Type::POOL_REAL_ARRAY ||
              type == Variant::Type::POOL_STRING_ARRAY ||
              type == Variant::Type::POOL_VECTOR2_ARRAY ||
              type == Variant::Type::POOL_VECTOR3_ARRAY) {
      Array v = value;
      for (int i = 0; i < v.size(); i++) offset = mem_write(v[i], offset);
      return offset;
    } else if (type == Variant::Type::POOL_BYTE_ARRAY) {
      PoolByteArray v = value;
      const uint8_t* bytes = v.read().ptr();
      size_t s = v.size();
      std::memcpy(data, bytes, s);
      return offset + s;
    }
    return offset;
  }

  void Wasm::map_names() {
    // Get exports and associated names
    wasm_exporttype_vec_t exports;
    wasm_module_exports(module, &exports);
    functions.clear();
    globals.clear();
    for (int i = 0; i < exports.size; i++) {
      const wasm_name_t* name = wasm_exporttype_name(exports.data[i]);
      const wasm_externkind_t kind = wasm_externtype_kind(wasm_exporttype_type(exports.data[i]));
      const String key = String(std::string(name->data, name->size).c_str());
      switch (kind) {
        case WASM_EXTERN_FUNC:
          functions[key] = i;
          break;
        case WASM_EXTERN_GLOBAL:
          globals[key] = i;
          break;
        case WASM_EXTERN_MEMORY:
          wasm_extern_vec_t e;
          wasm_instance_exports(instance, &e);
          memory = wasm_extern_as_memory(e.data[i]);
          break;
      }
    }
  }
}
