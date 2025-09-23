#include "./embind.h"

namespace godot {
namespace godot_wasm {
EmBind::EmBind(Wasm *module) {
	_module = module;
	_memory = module->get_memory();
}

void EmBind::__register_bool(uint rawType, uint name, bool trueValue, bool falseValue) {}
void EmBind::__register_bigint(uint primitiveType, uint name, uint size, uint minRange, uint maxRange) {}
void EmBind::__register_float(uint rawType, uint name, uint size) {}
void EmBind::__register_function(uint name, uint argCount, uint rawArgTypesAddr, uint signature, uint rawInvoker, uint fn, bool isAsync, bool isNonnullReturn) {}
void EmBind::__register_integer(uint primitiveType, uint name, uint size, uint minRange, uint maxRange) {}
void EmBind::__register_memory_view(uint rawType, uint dataTypeIndex, uint name) {}
void EmBind::__register_std_string(uint rawType, uint name) {}
void EmBind::__register_void(uint rawType, uint name) {}
void EmBind::__abort_js(uint rawType) {}
void EmBind::__register_emval(uint rawType) {}
void EmBind::__register_std_wstring(uint rawType, uint charSize, uint name) {}
void EmBind::__register_std_string(uint rawType, uint charSize, uint name) {}
void EmBind::__emscripten_resize_heap(uint requestedSize) {}
void EmBind::__fd_close(uint fd) {}
void EmBind::__fd_seek(uint fd, uint offset, uint whence, uint newOffset) {}

} //namespace godot_wasm
} //namespace godot
