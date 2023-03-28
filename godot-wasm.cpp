#include "godot-wasm.h"

using namespace godot;

void Wasm::_register_methods() {
  register_method("load", &Wasm::load);
  register_method("global", &Wasm::global);
  register_method("function", &Wasm::function);
}

Wasm::Wasm() {
  engine = wasm_engine_new();
  store = wasm_store_new(engine);
  module = NULL;
  instance = NULL;
  names = Dictionary();
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
  names = map_names(module);

  return GODOT_OK;
}

Variant Wasm::global(String name) {
  // Validate function name
  ERR_FAIL_COND_V(!names.has(name), Variant());

  // Retrieve exports
  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  const wasm_global_t* global = wasm_extern_as_global(exports.data[(int)names[name]]);
  ERR_FAIL_NULL_V(global, Variant());

  // Extract result
  wasm_val_t result;
  wasm_global_get(global, &result);
  return extract_variant(result);
}

Variant Wasm::function(String name, Array args) {
  // Validate function name
  ERR_FAIL_COND_V(!names.has(name), Variant());

  // Retrieve exports
  wasm_extern_vec_t exports;
  wasm_instance_exports(instance, &exports);
  const wasm_func_t* func = wasm_extern_as_func(exports.data[(int)names[name]]);
  ERR_FAIL_NULL_V(func, Variant());

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
  wasm_val_vec_t f_args = {vect.size(), vect.data()};
  wasm_val_vec_t f_results = WASM_ARRAY_VEC(results_val);
  ERR_FAIL_COND_V(wasm_func_call(func, &f_args, &f_results), Variant());

  // Extract result
  wasm_val_t result = results_val[0];
  return extract_variant(result);
}

Dictionary Wasm::map_names(wasm_module_t* module) {
  wasm_exporttype_vec_t exports;
  wasm_module_exports(module, &exports);
  Dictionary dict;
  for (int i = 0; i < exports.size; i++) {
    const wasm_name_t* name = wasm_exporttype_name(exports.data[i]);
    dict[String(std::string(name->data, name->size).c_str())] = i;
  }
  return dict;
}

Variant Wasm::extract_variant(wasm_val_t value) {
  switch (value.kind) {
    case WASM_I64: return Variant(value.of.i64);
    case WASM_F64: return Variant(value.of.f64);
    default: ERR_FAIL_V(Variant());
  }
}
