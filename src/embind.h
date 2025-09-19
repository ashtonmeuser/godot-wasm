#ifndef EMBIND_H
#define EMBIND_H
#include "core/object/ref_counted.h"
#include "./wasm-memory.h"
#include "./wasm.h"

namespace godot {
namespace godot_wasm {
class EmBind {
private:
	Wasm *_module;
	Ref<WasmMemory> _memory;

public:
	EmBind(Wasm *module);

	void __embind_register_bool(uint rawType, uint name, bool trueValue, bool falseValue);
	void __embind_register_bigint(uint primitiveType, uint name, uint size, uint minRange, uint maxRange);
	void __embind_register_float(uint rawType, uint name, uint size);
	void __embind_register_function(uint name, uint argCount, uint rawArgTypesAddr, uint signature, uint rawInvoker, uint fn, bool isAsync, bool isNonnullReturn);
	void __embind_register_integer(uint primitiveType, uint name, uint size, uint minRange, uint maxRange);
	void __embind_register_memory_view(uint rawType, uint dataTypeIndex, uint name);
	void __embind_register_std_string(uint rawType, uint name);
	void __embind_register_void(uint rawType, uint name);
	void __embind_register_emval(uint rawType);
	void __embind_register_std_wstring(uint rawType, uint charSize, uint name);
};
} //namespace godot_wasm
} //namespace godot
#endif
