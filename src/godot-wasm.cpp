#include "godot-wasm.h"

namespace {
  struct context_extern {
    uint16_t index;
  };

  struct context_callback: public context_extern {
    godot::Object* target;
    godot::String method; // External name; doesn't necessarily match import name
  };

  godot::Variant decode_variant(wasm_val_t value) {
    switch (value.kind) {
      case WASM_I32: return godot::Variant(value.of.i32);
      case WASM_I64: return godot::Variant(value.of.i64);
      case WASM_F32: return godot::Variant(value.of.f32);
      case WASM_F64: return godot::Variant(value.of.f64);
      case WASM_ANYREF: if (value.of.ref == NULL) return NULL_VARIANT;
      default: throw std::invalid_argument("Unsupported Wasm type");
    }
  }

  wasm_val_t encode_variant(godot::Variant variant) {
    switch (variant.get_type()) {
      case godot::Variant::INT: return WASM_I64_VAL((int64_t)variant);
      case godot::Variant::REAL: return WASM_F64_VAL((float64_t)variant);
      default: throw std::invalid_argument("Unsupported Godot variant type");
    }
  }

  godot::String decode_name(const wasm_name_t* name) {
    return godot::String(std::string(name->data, name->size).c_str());
  }

  void push_results(godot::Variant variant, wasm_val_vec_t* results) { // TODO: Rename
    if (results->size <= 0) return;
    if (variant.get_type() == godot::Variant::ARRAY) {
      godot::Array array = (godot::Array)variant;
      if (array.size() != results->size) throw std::length_error("Results length mismatch");
      for (uint16_t i = 0; i < results->size; i++) results->data[i] = encode_variant(array[i]);
    } else if (results->size == 1) {
      results->data[0] = encode_variant(variant);
    } else throw std::runtime_error("Unable to parse result variants");
  }

  wasm_extern_t* get_export_data(const wasm_instance_t* instance, uint16_t index) {
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);
    return exports.data[index];
  }

  godot::Variant::Type get_value_type(const wasm_valkind_t& kind) {
    switch (kind) {
      case WASM_I32: case WASM_I64: return godot::Variant::Type::INT;
      case WASM_F32: case WASM_F64: return godot::Variant::Type::REAL;
      default: throw std::invalid_argument("Unsupported value kind");
    }
  }

  godot::Array get_extern_signature(const wasm_module_t* module, uint16_t index, bool import) {
    // Grab the extern from module imports or exports
    const wasm_externtype_t* type;
    if (import) {
      wasm_importtype_vec_t imports;
      wasm_module_imports(module, &imports);
      type = wasm_importtype_type(imports.data[index]);
    } else {
      wasm_exporttype_vec_t exports;
      wasm_module_exports(module, &exports);
      type = wasm_exporttype_type(exports.data[index]);
    }

    // Generate a signature for extern
    switch (wasm_externtype_kind(type)) {
      case WASM_EXTERN_FUNC: {
        wasm_functype_t* func_type = wasm_externtype_as_functype((wasm_externtype_t*)type);
        const wasm_valtype_vec_t* func_params = wasm_functype_params(func_type);
        const wasm_valtype_vec_t* func_results = wasm_functype_results(func_type);
        godot::Array param_types, result_types;
        for (uint16_t i = 0; i < func_params->size; i++) param_types.append(get_value_type(wasm_valtype_kind(func_params->data[i])));
        for (uint16_t i = 0; i < func_results->size; i++) result_types.append(get_value_type(wasm_valtype_kind(func_results->data[i])));
        return godot::Array().make(param_types, result_types);
      } case WASM_EXTERN_GLOBAL: {
        wasm_globaltype_t* global_type = wasm_externtype_as_globaltype((wasm_externtype_t*)type);
        const godot::Variant variant_type = get_value_type(wasm_valtype_kind(wasm_globaltype_content(global_type)));
        const godot::Variant variant_mutability = godot::Variant(wasm_globaltype_mutability(global_type) == WASM_VAR ? true : false);
        return godot::Array().make(variant_type, variant_mutability);
      } default: throw std::invalid_argument("Extern type has no signature");
    }
  }

  wasm_trap_t* make_trap(const std::exception& e) { // TODO: Rename
    wasm_message_t message;
    wasm_name_new_from_string_nt(&message, e.what());
    return wasm_trap_new(NULL, &message);
  }

  wasm_trap_t* callback_wrapper(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
    // This is invoked by Wasm module calls to imported functions
    // Must be free function so context is passed via the env void pointer
    context_callback* context = (context_callback*)env;
    godot::Array params = godot::Array();
    // TODO: Check if args and results match expected sizes
    for (uint16_t i = 0; i < args->size; i++) params.push_back(decode_variant(args->data[i]));
    // TODO: Ensure target is valid and has method
    godot::Variant variant = context->target->callv(context->method, params);
    try { push_results(variant, results); }
    catch (const std::invalid_argument& e) { FAIL(e.what(), make_trap(e)); }
    catch (const std::length_error& e) { FAIL(e.what(), make_trap(e)); }
    return NULL;
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
    register_property<Wasm, Ref<StreamPeerWasm>>("stream", &Wasm::stream, NULL);
  }

  Wasm::Wasm() {
    engine = wasm_engine_new();
    store = wasm_store_new(engine);
    module = NULL;
    instance = NULL;
    memory_index = 0;
    stream.instance();
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
    stream->memory = NULL;
    import_funcs.clear();
    export_globals.clear();
    export_funcs.clear();

    // Load binary
    wasm_byte_vec_t wasm_bytes;
    wasm_byte_vec_new_uninitialized(&wasm_bytes, bytecode.size());
    memcpy(wasm_bytes.data, bytecode.read().ptr(), bytecode.size());

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

  godot_error Wasm::instantiate(const Dictionary import_map) {
    // Wire up imports
    std::vector<wasm_extern_t*> externs;
    for (const auto &tuple: import_funcs) {
      const Array& import = ((Dictionary)import_map["functions"])[tuple.first];
      FAIL_IF(import.size() != 2, "Invalid or missing import function " + tuple.first, GDERROR(ERR_CANT_CREATE));
      FAIL_IF(import[0].get_type() != Variant::OBJECT, "Invalid import target", GDERROR(ERR_CANT_CREATE));
      FAIL_IF(import[1].get_type() != Variant::STRING, "Invalid import method", GDERROR(ERR_CANT_CREATE));
      context_callback* context = (context_callback*)&tuple.second;
      context->target = import[0];
      context->method = import[1];
      externs.push_back(wasm_func_as_extern(create_callback(context)));
    }
    // TODO: Reorder by import index
    wasm_extern_vec_t imports = { externs.size(), externs.data() };

    // Instantiate with imports
    instance = wasm_instance_new(store, module, &imports, NULL);
    FAIL_IF(instance == NULL, "Instantiation failed", GDERROR(ERR_CANT_CREATE));

    // Set stream peer memory reference
    stream->memory = wasm_extern_as_memory(get_export_data(instance, memory_index));

    return GDERROR(OK);
  }

  godot_error Wasm::load(PoolByteArray bytecode, const Dictionary import_map) {
    // Compile and instantiate in one go
    godot_error err = compile(bytecode);
    if (err != GDERROR(OK)) return err;
    return instantiate(import_map);
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

    // Module extern names and signatures
    Dictionary import_func_sigs, export_global_sigs, export_func_sigs;
    for (const auto &tuple: import_funcs) import_func_sigs[tuple.first] = get_extern_signature(module, tuple.second.index, true);
    for (const auto &tuple: export_globals) export_global_sigs[tuple.first] = get_extern_signature(module, tuple.second.index, false);
    for (const auto &tuple: export_funcs) export_func_sigs[tuple.first] = get_extern_signature(module, tuple.second.index, false);

    // Module info dictionary
    Dictionary dict;
    dict["import_functions"] = import_func_sigs;
    dict["export_globals"] = export_global_sigs;
    dict["export_functions"] = export_func_sigs;
    dict["memory_min"] = Variant(limits->min * PAGE_SIZE);
    dict["memory_max"] = Variant(limits->max);
    if (stream->memory != NULL) dict["memory_current"] = Variant((int)wasm_memory_data_size(stream->memory));

    return dict;
  }

  Variant Wasm::global(String name) {
    // Validate instance and global name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!export_globals.count(name), "Unknown global name " + name, NULL_VARIANT);

    // Retrieve exported global
    const wasm_global_t* global = wasm_extern_as_global(get_export_data(instance, export_globals[name].index));
    FAIL_IF(global == NULL, "Failed to retrieve global export " + name, NULL_VARIANT);

    // Extract result
    wasm_val_t result;
    wasm_global_get(global, &result);
    try { return decode_variant(result); }
    catch (const std::invalid_argument& e) { FAIL(e.what(), NULL_VARIANT); }
  }

  Variant Wasm::function(String name, Array args) {
    // Validate instance and function name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!export_funcs.count(name), "Unknown function name " + name, NULL_VARIANT);

    // Retrieve exported function
    const wasm_func_t* func = wasm_extern_as_func(get_export_data(instance, export_funcs[name].index));
    FAIL_IF(func == NULL, "Failed to retrieve function export " + name, NULL_VARIANT);

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
    FAIL_IF(wasm_func_call(func, &f_args, &f_results), "Failed calling function " + name, NULL_VARIANT);

    // Extract result
    wasm_val_t result = results_val[0];
    try { return decode_variant(result); }
    catch (const std::invalid_argument& e) { FAIL(e.what(), NULL_VARIANT); }
  }

  uint64_t Wasm::mem_size() {
    FAIL_IF(instance == NULL, "Not instantiated", 0);
    FAIL_IF(stream->memory == NULL, "No memory", 0);
    return wasm_memory_data_size(stream->memory);
  }

  void Wasm::map_names() {
    // Module imports
    wasm_importtype_vec_t imports;
    wasm_module_imports(module, &imports);
    for (uint16_t i = 0; i < imports.size; i++) {
      const wasm_externtype_t* type = wasm_importtype_type(imports.data[i]);
      const wasm_externkind_t kind = wasm_externtype_kind(type);
      const String key = decode_name(wasm_importtype_module(imports.data[i])) + "." + decode_name(wasm_importtype_name(imports.data[i]));
      switch (kind) {
        case WASM_EXTERN_FUNC:
          import_funcs[key] = context_callback { { i } };
          break;
        default: throw std::invalid_argument("Import type not implemented");
      }
    }

    // Module exports
    wasm_exporttype_vec_t exports;
    wasm_module_exports(module, &exports);
    for (uint16_t i = 0; i < exports.size; i++) {
      const wasm_externtype_t* type = wasm_exporttype_type(exports.data[i]);
      const wasm_externkind_t kind = wasm_externtype_kind(type);
      const String key = decode_name(wasm_exporttype_name(exports.data[i]));
      switch (kind) {
        case WASM_EXTERN_FUNC:
          export_funcs[key] = context_extern { i };
          break;
        case WASM_EXTERN_GLOBAL:
          export_globals[key] = context_extern { i };
          break;
        case WASM_EXTERN_MEMORY:
          memory_index = i;
          break;
        default: throw std::invalid_argument("Export type not implemented");
      }
    }
  }

  wasm_func_t* Wasm::create_callback(context_callback* context) {
    wasm_importtype_vec_t imports;
    wasm_module_imports(module, &imports);
    const wasm_externtype_t* type = wasm_importtype_type(imports.data[context->index]);
    const wasm_functype_t* func_type = wasm_externtype_as_functype((wasm_externtype_t*)type);
    wasm_func_t* func = wasm_func_new_with_env(store, func_type, callback_wrapper, context, NULL);
    return func;
  }
}
