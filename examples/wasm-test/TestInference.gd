extends GodotWasmTestSuite

func test_param_types():
	var wasm = load_wasm("inference", { "functions": {
		"inference.echo_i32": dummy_import(),
		"inference.echo_i64": dummy_import(),
		"inference.echo_f32": dummy_import(),
		"inference.echo_f64": dummy_import(),
	} })
	expect_eq(wasm.function("add_i32", [3, -1]), 2)
	expect_eq(wasm.function("add_i64", [3, -1]), 2)
	expect_approx(wasm.function("add_f32", [3.5, -1.2]), 2.3)
	expect_approx(wasm.function("add_f64", [3.5, -1.2]), 2.3)
