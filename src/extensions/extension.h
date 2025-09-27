#ifndef GODOT_WASM_EXTENSION_H
#define GODOT_WASM_EXTENSION_H

#include <map>
#include <string>
#include <vector>
#include <wasm.h>
#include "../defs.h"
#include "../defer.h"
#include "../store.h"

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    typedef wasm_trap_t* (*extension_callback_t)(Wasm*, const wasm_val_vec_t*, wasm_val_vec_t*);
    typedef std::tuple<std::vector<wasm_valkind_enum>, std::vector<wasm_valkind_enum>, extension_callback_t> callback_signature;

    class Extension {
      private:
        Wasm* wasm; // Non-owning pointer to the Wasm instance
        std::map<std::string, callback_signature> signatures;

        wasm_func_t* create_callback(const callback_signature& signature) {
          const auto& param_types = std::get<0>(signature);
          const auto& result_types = std::get<1>(signature);

          wasm_valtype_vec_t params;
          wasm_valtype_vec_new_uninitialized(&params, param_types.size());
          for (size_t i = 0; i < param_types.size(); i++) params.data[i] = wasm_valtype_new(param_types[i]);
          DEFER(wasm_valtype_vec_delete(&params));

          wasm_valtype_vec_t results;
          wasm_valtype_vec_new_uninitialized(&results, result_types.size());
          for (size_t i = 0; i < result_types.size(); i++) results.data[i] = wasm_valtype_new(result_types[i]);
          DEFER(wasm_valtype_vec_delete(&results));

          wasm_functype_t* functype = wasm_functype_new(&params, &results);
          DEFER(wasm_functype_delete(functype));

          extension_callback_t callback = std::get<2>(signature);

          return wasm_func_new_with_env(STORE, functype, (wasm_func_callback_with_env_t)callback, wasm, NULL);
        }

      protected:
        void register_callback(
          const String& import_name,
          const std::vector<wasm_valkind_enum>& param_types,
          const std::vector<wasm_valkind_enum>& result_types,
          extension_callback_t callback
        ) {
          std::string key = std::string(import_name.utf8().get_data());
          signatures[key] = std::make_tuple(param_types, result_types, callback);
        }

      public:
        Extension(Wasm* wasm_instance): wasm(wasm_instance) {}
        virtual ~Extension() {}

        virtual wasm_func_t* get_callback(const String &name) final {
          std::string key = std::string(name.utf8().get_data());
          if (signatures.count(key)) {
            return create_callback(signatures[key]);
          }
          return NULL;
        }
    };
  }
}

#endif
