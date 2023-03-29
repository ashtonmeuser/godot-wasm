#include "godot-wasm.h"

using namespace godot;

#define NULL_VARIANT Variant()

void Wasm::_register_methods() {
  register_method("load", &Wasm::load);
  register_method("inspect", &Wasm::inspect);
  register_method("global", &Wasm::global);
  register_method("function", &Wasm::function);
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
  ERR_FAIL_COND_V(!wasm_module_validate(store, &wasm_bytes), GODOT_ERR_INVALID_DATA);

  // Compile
  module = wasm_module_new(store, &wasm_bytes);
  wasm_byte_vec_delete(&wasm_bytes);
  ERR_FAIL_NULL_V(module, GODOT_ERR_COMPILATION_FAILED);

  // Instantiate
  wasm_extern_vec_t imports = WASM_EMPTY_VEC;
  instance = wasm_instance_new(store, module, &imports, NULL);
  ERR_FAIL_NULL_V(instance, GODOT_ERR_CANT_CREATE);

  // Map names to export indices
  map_names();

  return GODOT_OK;
}

Dictionary Wasm::inspect() {
  Dictionary dict;
  ERR_FAIL_NULL_V(memory, dict);
  dict["functions"] = functions.keys();
  dict["globals"] = globals.keys();
  dict["memory"] = Variant((int)wasm_memory_data_size(memory));
  return dict;
}

Variant Wasm::global(String name) {
  // Validate instance and global name
  ERR_FAIL_NULL_V(instance, NULL_VARIANT);
  ERR_FAIL_COND_V(!globals.has(name), NULL_VARIANT);

  // Retrieve exports
  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  const wasm_global_t* global = wasm_extern_as_global(exports.data[(int)globals[name]]);
  ERR_FAIL_NULL_V(global, NULL_VARIANT);

  // Extract result
  wasm_val_t result;
  wasm_global_get(global, &result);
  return extract_variant(result);
}

Variant Wasm::function(String name, Array args) {
  // Validate instance and function name
  ERR_FAIL_NULL_V(instance, NULL_VARIANT);
  ERR_FAIL_COND_V(!functions.has(name), NULL_VARIANT);

  // Retrieve exports
  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  const wasm_func_t* func = wasm_extern_as_func(exports.data[(int)functions[name]]);
  ERR_FAIL_NULL_V(func, NULL_VARIANT);

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
      default:
        ERR_FAIL_V(Variant());
    }
  }

  // Call function
  wasm_val_t results_val[1] = { WASM_INIT_VAL };
  wasm_val_vec_t f_args = { vect.size(), vect.data() };
  wasm_val_vec_t f_results = WASM_ARRAY_VEC(results_val);
  ERR_FAIL_COND_V(wasm_func_call(func, &f_args, &f_results), NULL_VARIANT);

  // Extract result
  wasm_val_t result = results_val[0];
  return extract_variant(result);
}

godot_error Wasm::map_names() {
  // Validate module and instance
  ERR_FAIL_NULL_V(module, GODOT_FAILED);
  ERR_FAIL_NULL_V(instance, GODOT_FAILED);

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
  return GODOT_OK;
}

Variant Wasm::extract_variant(wasm_val_t value) {
  switch (value.kind) {
    case WASM_I32: return Variant(value.of.i32);
    case WASM_I64: return Variant(value.of.i64);
    case WASM_F32: return Variant(value.of.f32);
    case WASM_F64: return Variant(value.of.f64);
    case WASM_ANYREF: if (value.of.ref == NULL) return NULL_VARIANT;
    default: ERR_FAIL_V(NULL_VARIANT);
  }
}
