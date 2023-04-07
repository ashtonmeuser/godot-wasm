#include "godot-wasm.h"

namespace {
  godot::Variant extract_variant(wasm_val_t value) {
    switch (value.kind) {
      case WASM_I32: return godot::Variant(value.of.i32);
      case WASM_I64: return godot::Variant(value.of.i64);
      case WASM_F32: return godot::Variant(value.of.f32);
      case WASM_F64: return godot::Variant(value.of.f64);
      case WASM_ANYREF: if (value.of.ref == NULL) return NULL_VARIANT;
      default: throw std::invalid_argument("Unsupported Wasm type");
    }
  }

  wasm_extern_t* get_export_data(const wasm_instance_t* instance, uint16_t index) {
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    return exports.data[index];
  }
}

namespace godot {
  void Wasm::_register_methods() {
    register_method("compile", &Wasm::compile);
    register_method("instantiate", &Wasm::instantiate);
    register_method("load", &Wasm::load);
    register_method("inspect", &Wasm::inspect);
    register_method("global", &Wasm::global);
    register_method("function", &Wasm::function);
    register_property<Wasm, Ref<StreamPeerWasm>>("reader", &Wasm::reader, NULL);
  }

  Wasm::Wasm() {
    engine = wasm_engine_new();
    store = wasm_store_new(engine);
    functions = Dictionary();
    globals = Dictionary();
    memory_index = 0;
    reader.instance();
  }

  Wasm::~Wasm() {
    if (instance) wasm_instance_delete(instance);
    if (module) wasm_module_delete(module);
    if (store != NULL) wasm_store_delete(store);
    if (engine != NULL) wasm_engine_delete(engine);
  }

  void Wasm::_init() { }

  godot_error Wasm::compile(PoolByteArray bytecode) {
    // Reset
    instance = NULL;
    reader->memory = NULL;

    // Load binary
    wasm_byte_vec_t wasm_bytes;
    wasm_byte_vec_new_uninitialized(&wasm_bytes, bytecode.size());
    for (uint64_t i = 0; i < bytecode.size(); i++) wasm_bytes.data[i] = bytecode[i];

    // Validate binary
    FAIL_IF(!wasm_module_validate(store, &wasm_bytes), "Invalid binary", GDERROR(ERR_INVALID_DATA));

    // Compile
    module = wasm_module_new(store, &wasm_bytes);
    wasm_byte_vec_delete(&wasm_bytes);
    FAIL_IF(module == NULL, "Compilation failed", GDERROR(ERR_COMPILATION_FAILED));

    // Map names to export indices
    map_names();

    return GDERROR(OK);
  }

  godot_error Wasm::instantiate() {
    // Declare imports and instantiate
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    instance = wasm_instance_new(store, module, &imports, NULL);
    FAIL_IF(instance == NULL, "Instantiation failed", GDERROR(ERR_CANT_CREATE));

    // Set stream peer memory reference
    reader->memory = wasm_extern_as_memory(get_export_data(instance, memory_index));

    return GDERROR(OK);
  }

  godot_error Wasm::load(PoolByteArray bytecode) {
    godot_error err = compile(bytecode);
    if (err != GDERROR(OK)) return err;
    return instantiate();
  }

  Dictionary Wasm::inspect() {
    // Validate module
    FAIL_IF(module == NULL, "Inspection failed", Dictionary());

    // Get memory export limits
    wasm_exporttype_vec_t exports;
    wasm_module_exports(module, &exports);
    const wasm_externtype_t* extern_type = wasm_exporttype_type(exports.data[memory_index]);
    wasm_memorytype_t* memory_type = wasm_externtype_as_memorytype((wasm_externtype_t*)extern_type);
    const wasm_limits_t* limits = wasm_memorytype_limits(memory_type);

    // Module info dictionary
    Dictionary dict;
    dict["functions"] = functions.keys();
    dict["globals"] = globals.keys();
    dict["memory_min"] = Variant(limits->min * PAGE_SIZE);
    dict["memory_max"] = Variant(limits->max);
    if (reader->memory != NULL) {
      dict["memory_current"] = Variant((int)wasm_memory_data_size(reader->memory));
    }

    return dict;
  }

  Variant Wasm::global(String name) {
    // Validate instance and global name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!globals.has(name), "Unknown global name", NULL_VARIANT);

    // Retrieve exported global
    const wasm_global_t* global = wasm_extern_as_global(get_export_data(instance, globals[name]));
    FAIL_IF(global == NULL, "Failed to retrieve global export", NULL_VARIANT);

    // Extract result
    wasm_val_t result;
    wasm_global_get(global, &result);
    try { return extract_variant(result); }
    catch (const std::invalid_argument& e) { FAIL(e.what(), NULL_VARIANT); }
  }

  Variant Wasm::function(String name, Array args) {
    // Validate instance and function name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!functions.has(name), "Unknown function name", NULL_VARIANT);

    // Retrieve exported function
    const wasm_func_t* func = wasm_extern_as_func(get_export_data(instance, functions[name]));
    FAIL_IF(func == NULL, "Failed to retrieve function export", NULL_VARIANT);

    // Construct args
    std::vector<wasm_val_t> vect;
    for (uint16_t i = 0; i < args.size(); i++) {
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
    try { return extract_variant(result); }
    catch (const std::invalid_argument& e) { FAIL(e.what(), NULL_VARIANT); }
  }

  uint64_t Wasm::mem_size() {
    FAIL_IF(instance == NULL, "Not instantiated", 0);
    wasm_memory_t* memory = wasm_extern_as_memory(get_export_data(instance, memory_index));
    FAIL_IF(memory == NULL, "No memory", 0);
    return wasm_memory_data_size(memory);
  }

  void Wasm::map_names() {
    // Get exports and associated names
    wasm_exporttype_vec_t exports;
    wasm_module_exports(module, &exports);
    functions.clear();
    globals.clear();
    for (uint16_t i = 0; i < exports.size; i++) {
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
          memory_index = i;
          break;
      }
    }
  }
}
