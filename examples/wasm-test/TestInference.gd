extends GodotWasmTestSuite

var DUMMY_IMPORTS: Dictionary = {
	"functions": {
		"inference.echo_i32": [self, "_dummy"],
		"inference.echo_i64": [self, "_dummy"],
		"inference.echo_f32": [self, "_dummy"],
		"inference.echo_f64": [self, "_dummy"],
	}
}

# Dummy import to supply to Wasm modules
func _dummy(v): return v

func test_param_types():
	var wasm = load_wasm("inference", DUMMY_IMPORTS)
	expect_eq(wasm.function("add_i32", [3, -1]), 2)
	expect_eq(wasm.function("add_i64", [3, -1]), 2)
	expect_approx(wasm.function("add_f32", [3.5, -1.2]), 2.3)
	expect_approx(wasm.function("add_f64", [3.5, -1.2]), 2.3)
