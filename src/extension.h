#ifndef GODOT_WASM_EXTENSION_H
#define GODOT_WASM_EXTENSION_H

#include <map>
#include <wasm.h>
#include "defs.h"

#define REGISTER_EXTENSION(name, map) struct ExtensionRegistrar { ExtensionRegistrar() { godot_wasm::Extension::register_extension(name, map); } }; static ExtensionRegistrar _extension_registrar;

namespace godot {
  class Wasm; // Forward declare to avoid circular dependency

  namespace godot_wasm {
    class Extension; // Forward declare to avoid circular dependency

    typedef wasm_trap_t* (*extension_callback)(Extension* env, const wasm_val_vec_t* args, wasm_val_vec_t* results);
    typedef std::tuple<const std::vector<wasm_valkind_enum>, const std::vector<wasm_valkind_enum>, const extension_callback> extension_callback_signature;
    typedef std::map<std::string, extension_callback_signature> extension_callback_map;

    class Extension {
      private:
        std::string _name;
        uint64_t _target;
        static std::map<std::string, extension_callback_map>& get_extension_map();

      public:
        Wasm* wasm;
        Extension(const String name, Wasm* wasm);
        Extension(const String name, Wasm* wasm, const Object* target);
        static bool validate_extension(const String name);
        static void register_extension(const std::string name, const extension_callback_map& callbacks);
        wasm_func_t* get_callback(wasm_store_t* store, const String callback) const;
    };
  }
}

#endif