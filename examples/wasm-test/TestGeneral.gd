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
	var wasm = Wasm.new()
	var buffer = read_file("import")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	# Missing import
	var imports = {}
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Missing import function import.test_import")
	# Invalid import
	imports = { "functions": { "import.test_import": [] } }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import function import.test_import")
	# Invalid import target
	imports = { "functions": { "import.test_import": [0, "dummy"] } }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import target")
	# Invalid import method
	imports = { "functions": { "import.test_import": [self, 0] } }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import method")

func test_function():
	var wasm = load_wasm("simple")
	var result = wasm.function("add", [1, 2])
	expect_eq(result, 3)

func test_invalid_function():
	var wasm = load_wasm("simple")
	var result = wasm.function("asdf", [])
	expect_eq(result, null)
	expect_error("Unknown function name asdf")

func test_uninstantiated_function():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var result = wasm.function("add", [1, 2])
	expect_eq(result, null)
	expect_error("Not instantiated")

func test_invalid_function_args():
	var wasm = load_wasm("simple")
	var result = wasm.function("add", [{}, 2])
	expect_eq(result, null)
	expect_error("Unsupported Godot variant type")
	expect_error("Invalid argument type")

func test_callback_function():
	var imports = dummy_imports(["import.test_import"])
	var wasm = load_wasm("import", imports)
	wasm.function("callback", [])
	expect_log("Dummy import 123")

func test_global():
	var wasm = load_wasm("simple")
	var global_const = wasm.global("global_const")
	expect_eq(global_const, 1.6180339)
	var global_mut = wasm.global("global_mut")
	expect_eq(global_mut, 42)

func test_invalid_global():
	var wasm = load_wasm("simple")
	var result = wasm.global("asdf")
	expect_eq(result, null)
	expect_error("Unknown global name asdf")

func test_uninstantiated_global():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var result = wasm.global("global_const")
	expect_eq(result, null)
	expect_error("Not instantiated")

func test_inspect():
	# Pre-compile, simple module
	var wasm = Wasm.new()
	var inspect = wasm.inspect()
	expect_eq(inspect, {})
	expect_error("Inspection failed")
	# Pre-instantiation, simple module
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	inspect = wasm.inspect()
	expect(!inspect.has("memory_current"))
	expect_contains(inspect, "export_globals")
	expect_contains(inspect.export_globals, "global_const")
	expect_contains(inspect.export_globals, "global_mut")
	expect_eq(inspect.export_globals.get("global_const"), [TYPE_FLOAT, false])
	expect_eq(inspect.export_globals.get("global_mut"), [TYPE_INT, true])
	# Post-instantiation, import module
	var imports = dummy_imports(["import.test_import"])
	wasm = load_wasm("import", imports)
	inspect = wasm.inspect()
	expect_contains(inspect, "import_functions")
	expect_contains(inspect, "export_functions")
	expect_contains(inspect, "memory_min")
	expect_contains(inspect, "memory_max")
	expect_contains(inspect, "memory_current")
	expect_contains(inspect.import_functions, "import.test_import")
	expect_contains(inspect.export_functions, "callback")
	expect_eq(inspect.import_functions.get("import.test_import"), [[TYPE_INT], []])
	expect_eq(inspect.export_functions.get("callback"), [[], []])
	expect_empty()
