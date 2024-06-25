#include "extension.h"
#include "defer.h"

namespace godot {
  namespace {
    wasm_func_t* create_callback(wasm_store_t* store, const godot_wasm::Extension* extension, godot_wasm::extension_callback_signature signature) {
      auto p_types = new std::vector<wasm_valtype_t*>;
      auto r_types = new std::vector<wasm_valtype_t*>;
      for (auto &it: std::get<0>(signature)) p_types->push_back(wasm_valtype_new(it));
      for (auto &it: std::get<1>(signature)) r_types->push_back(wasm_valtype_new(it));
      wasm_valtype_vec_t params = { p_types->size(), p_types->data() };
      wasm_valtype_vec_t results = { r_types->size(), r_types->data() };
      wasm_functype_t* functype = wasm_functype_new(&params, &results);
      DEFER(wasm_functype_delete(functype));
      return wasm_func_new_with_env(store, functype, (wasm_func_callback_with_env_t)std::get<2>(signature), (void*)extension, NULL);
    }
  }

  namespace godot_wasm {
    Extension::Extension(const String name, Wasm* wasm_instance): wasm(wasm_instance), _target(0) {
      _name = std::string(name.utf8().get_data());
    }

    std::map<std::string, extension_callback_map>& Extension::get_extension_map() {
      static std::map<std::string, extension_callback_map> extensionMap;
      return extensionMap;
    }

    void Extension::register_extension(const std::string name, const extension_callback_map& callbacks) {
      get_extension_map().emplace(name, callbacks);
    }

    bool Extension::validate_extension(const String name) {
      std::string key = std::string(name.utf8().get_data());
      return get_extension_map().count(key);
    }

    wasm_func_t* Extension::get_callback(wasm_store_t* store, const String callback) const {
      std::string key = std::string(callback.utf8().get_data());
      auto extension_map = get_extension_map();
      if (!extension_map.count(_name)) return NULL;
      auto callback_map = extension_map[_name];
      return callback_map.count(key) ? create_callback(store, this, callback_map[key]) : NULL;
    }
  }
}