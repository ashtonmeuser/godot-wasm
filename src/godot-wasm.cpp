#include <string>
#include <vector>
#include "godot-wasm.h"
#include "wasi-shim.h"

namespace godot {
  namespace godot_wasm {
    struct context_extern {
      uint16_t index;
    };

    struct context_callback: public context_extern {
      Object* target;
      String method; // External name; doesn't necessarily match import name
      context_callback() { }
      context_callback(uint16_t i): context_extern { i } { }
    };
  }

  namespace {
    template <typename T> void unset(T* p, void (*f)(T*)) {
      if (p == NULL) return;
      f(p);
      p = NULL;
    }

    Variant decode_variant(wasm_val_t value) {
      switch (value.kind) {
        case WASM_I32: return Variant(value.of.i32);
        case WASM_I64: return Variant(value.of.i64);
        case WASM_F32: return Variant(value.of.f32);
        case WASM_F64: return Variant(value.of.f64);
        default: FAIL("Unsupported Wasm type", NULL_VARIANT);
      }
    }

    wasm_val_t encode_variant(Variant variant) {
      wasm_val_t value;
      switch (variant.get_type()) {
        case Variant::INT:
          value.kind = WASM_I64;
          value.of.i64 = (int64_t)variant;
          break;
        case Variant::FLOAT:
          value.kind = WASM_F64;
          value.of.f64 = (float64_t)variant;
          break;
        default:
          PRINT_ERROR("Unsupported Godot variant type");
          value.kind = WASM_ANYREF;
          value.of.ref = NULL;
      }
      return value;
    }

    String decode_name(const wasm_name_t* name) {
      return String(std::string(name->data, name->size).c_str());
    }

    inline Variant dict_safe_get(const Dictionary &d, String k, Variant e) {
      return d.has(k) && d[k].get_type() == e.get_type() ? d[k] : e;
    }

    godot_error extract_results(Variant variant, wasm_val_vec_t* results) {
      if (results->size <= 0) return OK;
      if (variant.get_type() == Variant::ARRAY) {
        Array array = variant.operator Array();
        if ((size_t)array.size() != results->size) return ERR_PARAMETER_RANGE_ERROR;
        for (uint16_t i = 0; i < results->size; i++) {
          results->data[i] = encode_variant(array[i]);
          if (results->data[i].kind == WASM_ANYREF) return ERR_INVALID_DATA;
        }
        return OK;
      } else if (results->size == 1) {
        results->data[0] = encode_variant(variant);
        return results->data[0].kind == WASM_ANYREF ? ERR_INVALID_DATA : OK;
      } else return ERR_INVALID_DATA;
    }

    wasm_extern_t* get_export_data(const wasm_instance_t* instance, uint16_t index) {
      wasm_extern_vec_t exports;
      wasm_instance_exports(instance, &exports);
      return exports.data[index];
    }

    Variant::Type get_value_type(const wasm_valkind_t& kind) {
      switch (kind) {
        case WASM_I32: case WASM_I64: return Variant::INT;
        case WASM_F32: case WASM_F64: return Variant::FLOAT;
        default: FAIL("Unsupported value kind", Variant::NIL);
      }
    }

    Array get_extern_signature(const wasm_module_t* module, uint16_t index, bool import) {
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
          Array signature, param_types, result_types;
          for (uint16_t i = 0; i < func_params->size; i++) param_types.append(get_value_type(wasm_valtype_kind(func_params->data[i])));
          for (uint16_t i = 0; i < func_results->size; i++) result_types.append(get_value_type(wasm_valtype_kind(func_results->data[i])));
          signature.append(param_types);
          signature.append(result_types);
          return signature;
        } case WASM_EXTERN_GLOBAL: {
          wasm_globaltype_t* global_type = wasm_externtype_as_globaltype((wasm_externtype_t*)type);
          Array signature;
          signature.append(get_value_type(wasm_valtype_kind(wasm_globaltype_content(global_type))));
          signature.append(Variant(wasm_globaltype_mutability(global_type) == WASM_VAR ? true : false));
          return signature;
        } default: FAIL("Unsupported extern type", Array());
      }
    }

    wasm_trap_t* trap(const char* message) {
      wasm_message_t trap_message;
      wasm_name_new_from_string_nt(&trap_message, message);
      return wasm_trap_new(NULL, &trap_message);
    }

    wasm_trap_t* callback_wrapper(void* env, const wasm_val_vec_t* args, wasm_val_vec_t* results) {
      // This is invoked by Wasm module calls to imported functions
      // Must be free function so context is passed via the env void pointer
      godot_wasm::context_callback* context = (godot_wasm::context_callback*)env;
      Array params = Array();
      // TODO: Check if args and results match expected sizes
      for (uint16_t i = 0; i < args->size; i++) params.push_back(decode_variant(args->data[i]));
      // TODO: Ensure target is valid and has method
      Variant variant = context->target->callv(context->method, params);
      godot_error error = extract_results(variant, results);
      if (error) FAIL("Extracting import function results failed", trap("Extracting import function results failed\0"));
      return NULL;
    }
  }

  void Wasm::REGISTRATION_METHOD() {
    #ifdef GDNATIVE
      register_method("compile", &Wasm::compile);
      register_method("instantiate", &Wasm::instantiate);
      register_method("load", &Wasm::load);
      register_method("inspect", &Wasm::inspect);
      register_method("global", &Wasm::global);
      register_method("function", &Wasm::function);
      register_method("has_permission", &Wasm::has_permission);
      register_property<Wasm, Ref<StreamPeerWasm>>("stream", &Wasm::stream, NULL);
      register_property<Wasm, Dictionary>("permissions", &Wasm::permissions, Dictionary());
    #else
      ClassDB::bind_method(D_METHOD("compile", "bytecode"), &Wasm::compile);
      ClassDB::bind_method(D_METHOD("instantiate", "import_map"), &Wasm::instantiate);
      ClassDB::bind_method(D_METHOD("load", "bytecode", "import_map"), &Wasm::load);
      ClassDB::bind_method(D_METHOD("inspect"), &Wasm::inspect);
      ClassDB::bind_method(D_METHOD("global", "name"), &Wasm::global);
      ClassDB::bind_method(D_METHOD("function", "name", "args"), &Wasm::function);
      ClassDB::bind_method(D_METHOD("set_permissions"), &Wasm::set_permissions);
      ClassDB::bind_method(D_METHOD("get_permissions"), &Wasm::get_permissions);
      ClassDB::bind_method(D_METHOD("has_permission"), &Wasm::has_permission);
      ClassDB::bind_method(D_METHOD("get_stream"), &Wasm::get_stream);
      ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "permissions"), "set_permissions", "get_permissions");
      ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream"), "", "get_stream");
    #endif
  }

  Wasm::Wasm() {
    engine = wasm_engine_new();
    store = wasm_store_new(engine);
    INSTANTIATE_REF(stream);
    reset_instance(); // Set initial state
  }

  Wasm::~Wasm() {
    reset_instance();
    unset(module, wasm_module_delete);
    unset(store, wasm_store_delete);
    unset(engine, wasm_engine_delete);
  }

  void Wasm::_init() { }

  void Wasm::exit(int32_t code) {
    reset_instance(); // Reset instance state
    code ? PRINT_ERROR("Module exited with error " + String::num_int64(code)) : PRINT("Module exited successfully");
    // TODO: Emit signal
  }

  void Wasm::reset_instance() {
    // TODO: Use nullptr
    // TODO: Free memory
    unset(instance, wasm_instance_delete);
    unset(stream->memory, wasm_memory_delete);
    memory_index = -1;
    import_funcs.clear();
    export_globals.clear();
    export_funcs.clear();
    permissions.clear();
    permissions["print"] = true;
    permissions["time"] = true;
    permissions["random"] = true;
    permissions["args"] = true;
    permissions["exit"] = true;
  }

  Ref<StreamPeerWasm> Wasm::get_stream() const {
    return stream;
  };

  void Wasm::set_permissions(const Dictionary &update) {
    for (auto i = 0; i < permissions.keys().size(); i++) {
      Variant key = permissions.keys()[i];
      permissions[key] = dict_safe_get(update, key, permissions[key]);
    }
  }

  Dictionary Wasm::get_permissions() const {
    return permissions;
  }

  bool Wasm::has_permission(String permission) const {
    return dict_safe_get(permissions, permission, false);
  }

  godot_error Wasm::compile(PackedByteArray bytecode) {
    reset_instance(); // Reset instance
    unset(module, wasm_module_delete); // Reset module

    // Load binary
    wasm_byte_vec_t wasm_bytes;
    wasm_byte_vec_new_uninitialized(&wasm_bytes, bytecode.size());
    memcpy(wasm_bytes.data, BYTE_ARRAY_POINTER(bytecode), bytecode.size());

    // Validate binary
    FAIL_IF(!wasm_module_validate(store, &wasm_bytes), "Invalid binary", ERR_INVALID_DATA);

    // Compile
    module = wasm_module_new(store, &wasm_bytes);
    wasm_byte_vec_delete(&wasm_bytes);
    FAIL_IF(module == NULL, "Compilation failed", ERR_COMPILATION_FAILED);

    // Map names to export indices
    FAIL_IF(map_names(), "Failed to parse module imports or exports", ERR_COMPILATION_FAILED);

    return OK;
  }

  godot_error Wasm::instantiate(const Dictionary import_map) {
    // Construct import functions
    std::map<uint16_t, wasm_extern_t*> extern_map;
    for (const auto &it: import_funcs) {
      const Dictionary& functions = dict_safe_get(import_map, "functions", Dictionary());
      if (!functions.keys().has(it.first)) {
        // Attempt to use default WASI import
        godot_wasm::wasi_callback callback = godot_wasm::get_wasi_import(it.first);
        FAIL_IF(callback == NULL, "Missing import function " + it.first, ERR_CANT_CREATE);
        extern_map[it.second.index] = callback(store, this);
        continue;
      }
      const Array& import = dict_safe_get(functions, it.first, Array());
      FAIL_IF(import.size() != 2, "Invalid import function " + it.first, ERR_CANT_CREATE);
      FAIL_IF(import[0].get_type() != Variant::OBJECT, "Invalid import target", ERR_CANT_CREATE);
      FAIL_IF(import[1].get_type() != Variant::STRING, "Invalid import method", ERR_CANT_CREATE);
      godot_wasm::context_callback* context = (godot_wasm::context_callback*)&it.second;
      context->target = import[0];
      context->method = import[1];
      extern_map[it.second.index] = wasm_func_as_extern(create_callback(context));
    }

    // Sort imports by index
    std::vector<wasm_extern_t*> extern_list;
    for (auto &it: extern_map) extern_list.push_back(it.second);
    wasm_extern_vec_t imports = { extern_list.size(), extern_list.data() };

    // Instantiate with imports
    instance = wasm_instance_new(store, module, &imports, NULL);
    FAIL_IF(instance == NULL, "Instantiation failed", ERR_CANT_CREATE);

    // Set stream peer memory reference
    if (memory_index >= 0) stream->memory = wasm_extern_as_memory(get_export_data(instance, memory_index));

    // Call exported WASI initialize function
    if (export_funcs.count("_initialize")) function("_initialize", Array());

    return OK;
  }

  godot_error Wasm::load(PackedByteArray bytecode, const Dictionary import_map) {
    // Compile and instantiate in one go
    godot_error err = compile(bytecode);
    if (err != OK) return err;
    return instantiate(import_map);
  }

  Dictionary Wasm::inspect() const {
    // Validate module
    FAIL_IF(module == NULL, "Inspection failed", Dictionary());

    // Get memory export limits
    wasm_limits_t* limits = NULL; // Unknown if memory not exported
    if (memory_index >= 0) {
      wasm_exporttype_vec_t exports;
      wasm_module_exports(module, &exports);
      const wasm_externtype_t* extern_type = wasm_exporttype_type(exports.data[memory_index]);
      wasm_memorytype_t* memory_type = wasm_externtype_as_memorytype((wasm_externtype_t*)extern_type);
      limits = (wasm_limits_t*)wasm_memorytype_limits(memory_type);
    }

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
    if (limits != NULL) dict["memory_min"] = Variant((uint64_t)limits->min * PAGE_SIZE);
    if (limits != NULL) dict["memory_max"] = Variant((uint64_t)limits->max * PAGE_SIZE);
    if (stream->memory != NULL) dict["memory_current"] = Variant((uint64_t)wasm_memory_data_size(stream->memory));

    return dict;
  }

  Variant Wasm::global(String name) const {
    // Validate instance and global name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!export_globals.count(name), "Unknown global name " + name, NULL_VARIANT);

    // Retrieve exported global
    const wasm_global_t* global = wasm_extern_as_global(get_export_data(instance, export_globals.at(name).index));
    FAIL_IF(global == NULL, "Failed to retrieve global export " + name, NULL_VARIANT);

    // Extract result
    wasm_val_t result;
    wasm_global_get(global, &result);
    return decode_variant(result);
  }

  Variant Wasm::function(String name, Array args) const {
    // Validate instance and function name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!export_funcs.count(name), "Unknown function name " + name, NULL_VARIANT);

    // Retrieve exported function
    const wasm_func_t* func = wasm_extern_as_func(get_export_data(instance, export_funcs.at(name).index));
    FAIL_IF(func == NULL, "Failed to retrieve function export " + name, NULL_VARIANT);

    // Construct args
    std::vector<wasm_val_t> vect;
    for (uint16_t i = 0; i < args.size(); i++) {
      Variant variant = args[i];
      wasm_val_t value = encode_variant(variant);
      FAIL_IF(value.kind == WASM_ANYREF, "Invalid argument type", NULL_VARIANT);
      vect.push_back(value);
    }

    // Call function
    wasm_val_t results_val[1]; // Only one return value supported
    results_val[0].kind = WASM_ANYREF;
    results_val[0].of.ref = NULL;
    wasm_val_vec_t f_args = { vect.size(), vect.data() };
    wasm_val_vec_t f_results = WASM_ARRAY_VEC(results_val);
    FAIL_IF(wasm_func_call(func, &f_args, &f_results), "Failed calling function " + name, NULL_VARIANT);

    // Extract result
    wasm_val_t result = results_val[0];
    return result.kind == WASM_ANYREF ? NULL_VARIANT : decode_variant(result);
  }

  uint64_t Wasm::mem_size() const {
    FAIL_IF(instance == NULL, "Not instantiated", 0);
    FAIL_IF(stream->memory == NULL, "No memory", 0);
    return wasm_memory_data_size(stream->memory);
  }

  godot_error Wasm::map_names() {
    // Module imports
    wasm_importtype_vec_t imports;
    wasm_module_imports(module, &imports);
    for (uint16_t i = 0; i < imports.size; i++) {
      const wasm_externtype_t* type = wasm_importtype_type(imports.data[i]);
      const wasm_externkind_t kind = wasm_externtype_kind(type);
      const String key = decode_name(wasm_importtype_module(imports.data[i])) + "." + decode_name(wasm_importtype_name(imports.data[i]));
      switch (kind) {
        case WASM_EXTERN_FUNC:
          import_funcs[key] = { i };
          break;
        default: FAIL("Import type not implemented", ERR_INVALID_DATA);
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
          export_funcs[key] = { i };
          break;
        case WASM_EXTERN_GLOBAL:
          export_globals[key] = { i };
          break;
        case WASM_EXTERN_MEMORY:
          memory_index = i;
          break;
        default: FAIL("Export type not implemented", ERR_INVALID_DATA);
      }
    }

    return OK;
  }

  wasm_func_t* Wasm::create_callback(godot_wasm::context_callback* context) {
    wasm_importtype_vec_t imports;
    wasm_module_imports(module, &imports);
    const wasm_externtype_t* type = wasm_importtype_type(imports.data[context->index]);
    const wasm_functype_t* func_type = wasm_externtype_as_functype((wasm_externtype_t*)type);
    wasm_func_t* func = wasm_func_new_with_env(store, func_type, callback_wrapper, context, NULL);
    return func;
  }
}
