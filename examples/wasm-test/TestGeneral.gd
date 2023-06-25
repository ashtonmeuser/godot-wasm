extends GodotWasmTestSuite

func test_compile():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	expect_empty()

func test_multi_compile():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	for _i in 5:
		var error = wasm.compile(buffer)
		expect_eq(error, OK)
	expect_empty()

func test_instantiate():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	error = wasm.instantiate({})
	expect_eq(error, OK)
	expect_empty()

func test_multi_instantiate():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	for _i in 5:
		error = wasm.instantiate({})
		expect_eq(error, OK)
	expect_empty()

func test_load():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var error = wasm.load(buffer, {})
	expect_eq(error, OK)
	expect_empty()

func test_invalid_binary():
	var wasm = Wasm.new()
	var buffer = "asdf".to_utf8_buffer()
	var error = wasm.compile(buffer)
	expect_eq(error, ERR_INVALID_DATA)
	expect_error("Invalid binary")

func test_imports():
	var imports = dummy_imports(["import.test_import"])
	load_wasm("import", imports)
	expect_empty()

func test_invalid_imports():
	load_wasm("import", {}, ERR_CANT_CREATE)
	expect_error("Missing import function import.test_import")

func test_function():
	var wasm = load_wasm("simple")
	var result = wasm.function("add", [1, 2])
	expect_eq(result, 3)

func test_invalid_function():
	var wasm = load_wasm("simple")
	wasm.function("asdf", [])
	expect_error("Unknown function name asdf")

func test_callback_function():
	var imports = dummy_imports(["import.test_import"])
	var wasm = load_wasm("import", imports)
	wasm.function("callback", [])
	expect_log("Dummy import 123")

func test_inspect():
	var imports = dummy_imports(["import.test_import"])
	var wasm = load_wasm("import", imports)
	var inspect = wasm.inspect()
	expect_contains(inspect, "import_functions")
	expect_contains(inspect, "export_functions")
	expect_contains(inspect, "memory_min")
	expect_contains(inspect, "memory_max")
	expect_contains(inspect.import_functions, "import.test_import")
	expect_contains(inspect.export_functions, "callback")
	expect_eq(inspect.import_functions["import.test_import"], [[TYPE_INT], []])
	expect_eq(inspect.export_functions.callback, [[], []])
	expect_empty()
