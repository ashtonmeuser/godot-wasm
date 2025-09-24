#include "./embind.h"

#include "modules/wasm/src/defer.h"
#include "modules/wasm/src/defs.h"
#include "wasi-shim.h"
#include "wasm.h"
#include <string>

namespace godot {
namespace {
typedef std::tuple<const std::vector<wasm_valkind_enum>, const std::vector<wasm_valkind_enum>, const wasm_func_callback_with_env_t> callback_signature;

wasm_trap_t *__register_function(void *env, const wasm_val_vec_t *args, wasm_val_vec_t *results) {
	FAIL_IF(args->size != 8, "Invalid arguments args_get", godot_wasm::wasi_result(results, __WASI_ERRNO_INVAL, "Invalid arguments\0"));
	auto wasm = (Wasm *)env;
	wasm_memory_t *memory = wasm->get_memory().ptr()->get_memory();
	if (memory == NULL) {
		return godot_wasm::wasi_result(results, __WASI_ERRNO_IO, "Invalid memory\0");
	}

	return godot_wasm::wasi_result(results);
}

wasm_trap_t *__register_bool(uint rawType, uint name, bool trueValue, bool falseValue) {}
wasm_trap_t *__register_bigint(uint primitiveType, uint name, uint size, uint minRange, uint maxRange) {}
wasm_trap_t *__register_float(uint rawType, uint name, uint size) {}
wasm_trap_t *__register_integer(uint primitiveType, uint name, uint size, uint minRange, uint maxRange) {}
wasm_trap_t *__register_memory_view(uint rawType, uint dataTypeIndex, uint name) {}
wasm_trap_t *__register_std_string(uint rawType, uint name) {}
wasm_trap_t *__register_void(uint rawType, uint name) {}
wasm_trap_t *__abort_js(uint rawType) {}
wasm_trap_t *__register_emval(uint rawType) {}
wasm_trap_t *__register_std_wstring(uint rawType, uint charSize, uint name) {}
wasm_trap_t *__register_std_string(uint rawType, uint charSize, uint name) {}
wasm_trap_t *__emscripten_resize_heap(uint requestedSize) {}
wasm_trap_t *__fd_close(uint fd) {}
wasm_trap_t *__fd_seek(uint fd, uint offset, uint whence, uint newOffset) {}

std::map<std::string, callback_signature> embind_sig{
	{ "env._embind_register_function", { { WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32, WASM_I32 }, {}, __register_function } }
};
} //namespace

namespace godot_wasm {
wasm_func_t *get_embind_callback(wasm_store_t *store, Wasm *wasm, const String name) {
	std::string key = std::string(name.utf8().get_data());
	return embind_sig.count(key) ? wasi_callback(store, wasm, embind_sig[key]) : NULL;
}
} //namespace godot_wasm
} //namespace godot
