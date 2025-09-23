#ifndef EMBIND_H
#define EMBIND_H
#include "./wasm.h"
#include "core/object/ref_counted.h"

namespace godot {
namespace godot_wasm {
class EmBind {
private:
	Wasm *_module;
	Ref<WasmMemory> _memory;

public:
	EmBind(Wasm *module);

	void __register_bool(uint rawType, uint name, bool trueValue, bool falseValue);
	void __register_bigint(uint primitiveType, uint name, uint size, uint minRange, uint maxRange);
	void __register_float(uint rawType, uint name, uint size);
	void __register_function(uint name, uint argCount, uint rawArgTypesAddr, uint signature, uint rawInvoker, uint fn, bool isAsync, bool isNonnullReturn);
	void __register_integer(uint primitiveType, uint name, uint size, uint minRange, uint maxRange);
	void __register_memory_view(uint rawType, uint dataTypeIndex, uint name);
	void __register_std_string(uint rawType, uint name);
	void __register_void(uint rawType, uint name);
	void __abort_js(uint rawType);
	void __register_emval(uint rawType);
	void __register_std_wstring(uint rawType, uint charSize, uint name);
	void __register_std_string(uint rawType, uint charSize, uint name);
	void __emscripten_resize_heap(uint requestedSize);
	void __fd_close(uint fd);
	void __fd_seek(uint fd, uint offset, uint whence, uint newOffset);
};
} //namespace godot_wasm
} //namespace godot
#endif
