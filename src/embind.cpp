#include "./embind.h"

namespace godot {
namespace godot_wasm {
EmBind::EmBind(Wasm *module) {
	_module = module;
	_memory = module->get_memory();
}
} //namespace godot_wasm
} //namespace godot
