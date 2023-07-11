extends GodotWasmTestSuite

func test_multivalue_inspect():
	var wasm = Wasm.new()
	var buffer = read_file("special")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	var inspect = wasm.inspect()
	expect_eq(inspect.export_functions.get("multi_return"), [[TYPE_INT, TYPE_INT], [TYPE_INT, TYPE_INT]])

func test_multivalue_invoke():
	var wasm = load_wasm("special")
	var result = wasm.function("multi_return", [123, 456])
	expect_type(result, TYPE_ARRAY)
	expect_eq(result, [456, 123])
