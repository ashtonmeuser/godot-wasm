#include <string>
#include <vector>
#include "wasm.h"
#include "wasi-shim.h"
#include "defer.h"
#include "store.h"

namespace godot {
  namespace godot_wasm {
    struct ContextExtern {
      uint16_t index; // Index within module imports/exports
      ContextExtern(uint16_t i) { index = i; }
    };

    struct ContextFuncImport: public ContextExtern {
      ObjectID target; // The ID of the object on which to invoke callback method
      String method; // External name; doesn't necessarily match import name
      std::vector<wasm_valkind_t> results; // Return types
      ContextFuncImport(uint16_t i, const wasm_functype_t* func_type): ContextExtern(i) {
        const wasm_valtype_vec_t* func_results = wasm_functype_results(func_type);
        for (uint16_t i = 0; i < func_results->size; i++) results.push_back(wasm_valtype_kind(func_results->data[i]));
      }
    };

    struct ContextFuncExport: public ContextExtern {
      size_t return_count; // Number of return values
      std::vector<wasm_valkind_t> params; // Param types
      ContextFuncExport(uint16_t i, const wasm_functype_t* func_type): ContextExtern(i) {
        const wasm_valtype_vec_t* func_params = wasm_functype_params(func_type);
        const wasm_valtype_vec_t* func_results = wasm_functype_results(func_type);
        for (uint16_t i = 0; i < func_params->size; i++) params.push_back(wasm_valtype_kind(func_params->data[i]));
        return_count = func_results->size;
      }
    };

    struct ContextMemory: public ContextExtern {
      bool import; // Import; not export
      ContextMemory(uint16_t i, bool import): ContextExtern(i), import(import) { }
    };
  }

  namespace {
    template <typename T> void unset(T*& p, void (*f)(T*)) {
      if (p == NULL) return;
      f(p);
      p = NULL;
    }

    template <typename T> void unset(T*& p) {
      if (p == NULL) return;
      delete p;
      p = NULL;
    }

    inline wasm_val_t error_value(const char* message) {
      PRINT_ERROR(message);
      wasm_val_t value;
      value.kind = WASM_EXTERNREF;
      value.of.ref = NULL;
      return value;
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

    wasm_val_t encode_variant(Variant variant, wasm_valkind_t kind) {
      wasm_val_t value;
      value.kind = kind;
      switch (variant.get_type()) {
        case Variant::INT:
          switch (kind) {
            case WASM_I32:
              value.of.i32 = (int32_t)variant;
              return value;
            case WASM_I64:
              value.of.i64 = (int64_t)variant;
              return value;
            default:
              return error_value("Invalid target type for integer variant");
          }
        case Variant::FLOAT:
          switch (kind) {
            case WASM_F32:
              value.of.f32 = (float32_t)variant;
              return value;
            case WASM_F64:
              value.of.f64 = (float64_t)variant;
              return value;
            default:
              return error_value("Invalid target type for float variant");
          }
        default:
          return error_value("Unsupported Godot variant type");
      }
    }

    String decode_name(const wasm_name_t* name) {
      return String(std::string(name->data, name->size).c_str());
    }

    inline Variant dict_safe_get(const Dictionary &d, String k, Variant e) {
      return d.has(k) && d[k].get_type() == e.get_type() ? d[k] : e;
    }

    template <typename T> inline T* dict_safe_get(const Dictionary &d, String k) {
      return d.has(k) && d[k].get_type() == Variant::OBJECT ? Object::cast_to<T>(d[k]) : NULL;
    }

    godot_error extract_results(Variant variant, const godot_wasm::ContextFuncImport* context, wasm_val_vec_t* results) {
      FAIL_IF(results->size != context->results.size(), "Incompatible return value(s)", ERR_INVALID_DATA);
      if (results->size <= 0) return OK;
      if (variant.get_type() == Variant::ARRAY) {
        Array array = variant.operator Array();
        if ((size_t)array.size() != results->size) return ERR_PARAMETER_RANGE_ERROR;
        for (uint16_t i = 0; i < results->size; i++) {
          results->data[i] = encode_variant(array[i], context->results[i]);
          if (results->data[i].kind == WASM_EXTERNREF) return ERR_INVALID_DATA;
        }
        return OK;
      } else if (results->size == 1) {
        results->data[0] = encode_variant(variant, context->results[0]);
        return results->data[0].kind == WASM_EXTERNREF ? ERR_INVALID_DATA : OK;
      } else return ERR_INVALID_DATA;
    }

    Variant::Type get_value_type(const wasm_valkind_t& kind) {
      switch (kind) {
        case WASM_I32: case WASM_I64: return Variant::INT;
        case WASM_F32: case WASM_F64: return Variant::FLOAT;
        default: FAIL("Unsupported value kind", Variant::NIL);
      }
    }

    wasm_externtype_t* get_extern_type(const wasm_module_t* module, uint16_t index, bool import) {
      if (import) {
        wasm_importtype_vec_t imports;
        DEFER(wasm_importtype_vec_delete(&imports));
        wasm_module_imports(module, &imports);
        return wasm_externtype_copy((wasm_externtype_t*)wasm_importtype_type(imports.data[index]));
      } else {
        wasm_exporttype_vec_t exports;
        DEFER(wasm_exporttype_vec_delete(&exports));
        wasm_module_exports(module, &exports);
        return wasm_externtype_copy((wasm_externtype_t*)wasm_exporttype_type(exports.data[index]));
      }
    }

    Dictionary get_memory_limits(const wasm_module_t* module, const godot_wasm::ContextMemory* context) {
      Dictionary dict;
      if (context == NULL) return dict;
      wasm_externtype_t* type = get_extern_type(module, context->index, context->import);
      DEFER(wasm_externtype_delete(type));
      wasm_memorytype_t* memory_type = wasm_externtype_as_memorytype(type);
      auto limits = wasm_memorytype_limits(memory_type);
      dict["min"] = limits->min * PAGE_SIZE;
      dict["max"] = limits->max * PAGE_SIZE;
      return dict;
    }

    Array get_extern_signature(const wasm_module_t* module, uint16_t index, bool import) {
      // Grab the extern from module imports or exports
      wasm_externtype_t* type = get_extern_type(module, index, import);
      DEFER(wasm_externtype_delete(type));

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
      godot_wasm::ContextFuncImport* context = (godot_wasm::ContextFuncImport*)env;
      Array params = Array();
      // TODO: Check if args and results match expected sizes
      for (uint16_t i = 0; i < args->size; i++) params.push_back(decode_variant(args->data[i]));
      Object* target = INSTANCE_FROM_ID(context->target);
      FAIL_IF(target == nullptr, "Failed to retrieve import function target", trap("Failed to retrieve import function target\0"));
      Variant variant = target->callv(context->method, params);
      godot_error error = extract_results(variant, context, results);
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
      register_property<Wasm, Ref<WasmMemory>>("memory", &Wasm::memory, NULL);
      register_property<Wasm, Dictionary>("permissions", &Wasm::permissions, Dictionary());
    #else
      ClassDB::bind_method(D_METHOD("compile", "bytecode"), &Wasm::compile);
      ClassDB::bind_method(D_METHOD("instantiate", "import_map"), &Wasm::instantiate);
      ClassDB::bind_method(D_METHOD("load", "bytecode", "import_map"), &Wasm::load);
      ClassDB::bind_method(D_METHOD("inspect"), &Wasm::inspect);
      ClassDB::bind_method(D_METHOD("global", "name"), &Wasm::global);
      ClassDB::bind_method(D_METHOD("function", "name", "args"), &Wasm::function, DEFVAL(Array()));
      ClassDB::bind_method(D_METHOD("set_permissions"), &Wasm::set_permissions);
      ClassDB::bind_method(D_METHOD("get_permissions"), &Wasm::get_permissions);
      ClassDB::bind_method(D_METHOD("has_permission", "permission"), &Wasm::has_permission);
      ClassDB::bind_method(D_METHOD("get_memory"), &Wasm::get_memory);
      ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "permissions"), "set_permissions", "get_permissions");
      ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "memory"), "", "get_memory");
    #endif
  }

  Wasm::Wasm() {
    module = NULL;
    instance = NULL;
    memory_context = NULL;
    reset_instance(); // Set initial state
  }

  Wasm::~Wasm() {
    reset_instance();
    unset(module, wasm_module_delete);
  }

  void Wasm::_init() { }

  void Wasm::exit(int32_t code) {
    reset_instance(); // Reset instance state
    code ? PRINT_ERROR("Module exited with error " + String::num_int64(code)) : PRINT("Module exited successfully");
    // TODO: Emit signal
  }

  void Wasm::reset_instance() {
    unset(instance, wasm_instance_delete);
    unset(memory_context);
    memory = Ref<WasmMemory>(NULL);
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

  Ref<WasmMemory> Wasm::get_memory() const {
    return memory;
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
    DEFER(wasm_byte_vec_delete(&wasm_bytes));
    wasm_byte_vec_new(&wasm_bytes, bytecode.size(), (const wasm_byte_t *)bytecode.ptr());

    // Validate binary
    FAIL_IF(!wasm_module_validate(STORE, &wasm_bytes), "Invalid binary", ERR_INVALID_DATA);

    // Compile
    module = wasm_module_new(STORE, &wasm_bytes);
    FAIL_IF(module == NULL, "Compilation failed", ERR_COMPILATION_FAILED);

    // Map names to export indices
    FAIL_IF(map_names(), "Failed to parse module imports or exports", ERR_COMPILATION_FAILED);

    return OK;
  }

  godot_error Wasm::instantiate(const Dictionary import_map) {
    // Prepare module externs
    std::map<uint16_t, wasm_extern_t*> extern_map;

    // Construct import functions
    const Dictionary& functions = dict_safe_get(import_map, "functions", Dictionary());
    for (const auto &it: import_funcs) {
      if (!functions.keys().has(it.first)) {
        // Attempt to use default WASI import
        auto callback = godot_wasm::get_wasi_callback(STORE, this, it.first);
        FAIL_IF(callback == NULL, "Missing import function " + it.first, ERR_CANT_CREATE);
        extern_map[it.second.index] = wasm_func_as_extern(callback);
        continue;
      }
      const Array& import = dict_safe_get(functions, it.first, Array());
      FAIL_IF(import.size() != 2, "Invalid import function " + it.first, ERR_CANT_CREATE);
      FAIL_IF(import[0].get_type() != Variant::OBJECT, "Invalid import target " + it.first, ERR_CANT_CREATE);
      FAIL_IF(!INSTANCE_VALIDATE(import[0]), "Invalid import target " + it.first, ERR_CANT_CREATE);
      FAIL_IF(import[1].get_type() != Variant::STRING, "Invalid import method " + it.first, ERR_CANT_CREATE);
      godot_wasm::ContextFuncImport* context = (godot_wasm::ContextFuncImport*)&it.second;
      context->target = import[0].operator Object*()->get_instance_id();
      context->method = import[1];
      extern_map[it.second.index] = wasm_func_as_extern(create_callback(context));
    }

    // Configure import memory
    WasmMemory* import_memory = NULL;
    if (memory_context && memory_context->import) {
      import_memory = dict_safe_get<WasmMemory>(import_map, "memory");
      FAIL_IF(import_memory == NULL, "Missing import memory", ERR_CANT_CREATE);
      FAIL_IF(import_memory->get_memory() == NULL, "Invalid import memory", ERR_CANT_CREATE);
      // TODO: Validate memory limits
      extern_map[memory_context->index] = wasm_extern_copy(wasm_memory_as_extern(import_memory->get_memory()));
    }

    // Sort imports by index
    std::vector<wasm_extern_t*> extern_list;
    for (auto &it: extern_map) extern_list.push_back(it.second); // Maps iterate over sorted keys

    wasm_extern_vec_t imports;
    DEFER(wasm_extern_vec_delete(&imports));
    wasm_extern_vec_new(&imports, extern_list.size(), extern_list.data());

    // Instantiate with imports
    instance = wasm_instance_new(STORE, module, &imports, NULL);
    FAIL_IF(instance == NULL, "Instantiation failed", ERR_CANT_CREATE);

    // Set memory reference
    if (import_memory) {
      memory = Ref<WasmMemory>(import_memory);
    } else if (memory_context && !memory_context->import) {
      wasm_extern_vec_t exports;
      DEFER(wasm_extern_vec_delete(&exports));
      wasm_instance_exports(instance, &exports);
      wasm_extern_t* data = exports.data[memory_context->index];
      INSTANTIATE_REF(memory);
      memory->set_memory(wasm_extern_as_memory(wasm_extern_copy(data)));
    }

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
    Dictionary dict_memory = memory != NULL && memory->get_memory() ? memory->inspect() : get_memory_limits(module, memory_context);
    if (memory_context != NULL) dict_memory["import"] = memory_context->import;
    dict["memory"] = dict_memory;
    return dict;
  }

  Variant Wasm::global(String name) const {
    // Validate instance and global name
    FAIL_IF(instance == NULL, "Not instantiated", NULL_VARIANT);
    FAIL_IF(!export_globals.count(name), "Unknown global name " + name, NULL_VARIANT);

    // Retrieve exported global
    wasm_extern_vec_t exports;
    DEFER(wasm_extern_vec_delete(&exports));
    wasm_instance_exports(instance, &exports);
    wasm_extern_t* data = exports.data[export_globals.at(name).index];
    const wasm_global_t* global = wasm_extern_as_global(data);
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
    wasm_extern_vec_t exports;
    DEFER(wasm_extern_vec_delete(&exports));
    wasm_instance_exports(instance, &exports);
    godot_wasm::ContextFuncExport context = export_funcs.at(name);
    wasm_extern_t* data = exports.data[context.index];
    const wasm_func_t* func = wasm_extern_as_func(data);
    FAIL_IF(func == NULL, "Failed to retrieve function export " + name, NULL_VARIANT);

    // Validate argument count
    FAIL_IF(context.params.size() != args.size(), "Incorrect number of arguments supplied", NULL_VARIANT);

    // Construct args
    std::vector<wasm_val_t> args_vec;
    for (uint16_t i = 0; i < args.size(); i++) {
      Variant variant = args[i];
      wasm_val_t value = encode_variant(variant, context.params[i]);
      FAIL_IF(value.kind == WASM_EXTERNREF, "Invalid argument type", NULL_VARIANT);
      args_vec.push_back(value);
    }
    wasm_val_vec_t f_args;
    DEFER(wasm_val_vec_delete(&f_args));
    wasm_val_vec_new(&f_args,  args_vec.size(), args_vec.data());

    // Construct return values
    wasm_val_vec_t f_results;
    DEFER(wasm_val_vec_delete(&f_results));
    wasm_val_vec_new_uninitialized(&f_results,  context.return_count);

    // Call function
    FAIL_IF(wasm_func_call(func, &f_args, &f_results), "Failed calling function " + name, NULL_VARIANT);

    // Extract result(s)
    if (context.return_count == 0) return NULL_VARIANT;
    if (context.return_count == 1) return decode_variant(f_results.data[0]);
    Array results = Array();
    for (uint16_t i = 0; i < context.return_count; i++) results.append(decode_variant(f_results.data[i]));
    return results;
  }

  godot_error Wasm::map_names() {
    // Module imports
    wasm_importtype_vec_t imports;
    DEFER(wasm_importtype_vec_delete(&imports));
    wasm_module_imports(module, &imports);
    for (uint16_t i = 0; i < imports.size; i++) {
      const wasm_externtype_t* type = wasm_importtype_type(imports.data[i]);
      const wasm_externkind_t kind = wasm_externtype_kind(type);
      const String key = decode_name(wasm_importtype_module(imports.data[i])) + "." + decode_name(wasm_importtype_name(imports.data[i]));
      switch (kind) {
        case WASM_EXTERN_FUNC: {
          const wasm_functype_t* func_type = wasm_externtype_as_functype((wasm_externtype_t*)type);
          import_funcs.emplace(key, godot_wasm::ContextFuncImport(i, func_type));
          break;
        } case WASM_EXTERN_MEMORY:
          memory_context = new godot_wasm::ContextMemory(i, true);
          break;
        default: FAIL("Import type not implemented", ERR_INVALID_DATA);
      }
    }

    // Module exports
    wasm_exporttype_vec_t exports;
    DEFER(wasm_exporttype_vec_delete(&exports));
    wasm_module_exports(module, &exports);
    for (uint16_t i = 0; i < exports.size; i++) {
      const wasm_externtype_t* type = wasm_exporttype_type(exports.data[i]);
      const wasm_externkind_t kind = wasm_externtype_kind(type);
      const String key = decode_name(wasm_exporttype_name(exports.data[i]));
      switch (kind) {
        case WASM_EXTERN_FUNC: {
          const wasm_functype_t* func_type = wasm_externtype_as_functype((wasm_externtype_t*)type);
          export_funcs.emplace(key, godot_wasm::ContextFuncExport(i, func_type));
          break;
        } case WASM_EXTERN_GLOBAL:
          export_globals.emplace(key, godot_wasm::ContextExtern(i));
          break;
        case WASM_EXTERN_MEMORY:
          if (memory_context == NULL) memory_context = new godot_wasm::ContextMemory(i, false); // Favour import memory
          break;
        default: FAIL("Export type not implemented", ERR_INVALID_DATA);
      }
    }

    return OK;
  }

  wasm_func_t* Wasm::create_callback(godot_wasm::ContextFuncImport* context) {
    wasm_importtype_vec_t imports;
    DEFER(wasm_importtype_vec_delete(&imports));
    wasm_module_imports(module, &imports);
    const wasm_externtype_t* type = wasm_importtype_type(imports.data[context->index]);
    const wasm_functype_t* func_type = wasm_externtype_as_functype((wasm_externtype_t*)type);
    return wasm_func_new_with_env(STORE, func_type, callback_wrapper, context, NULL);
  }
}
