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
	var buffer = Utils.to_utf8("asdf")
	var error = wasm.compile(buffer)
	expect_eq(error, ERR_INVALID_DATA)
	expect_error("Invalid binary")

func test_function():
	var wasm = load_wasm("simple")
	var result = wasm.function("add", [1, 2])
	expect_eq(result, 3)
	expect_empty()

func test_invalid_function():
	var wasm = load_wasm("simple")
	var result = wasm.function("asdf", [])
	expect_eq(result, null)
	expect_error("Unknown function name asdf")

func test_uninstantiated_function():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	var result = wasm.function("add", [1, 2])
	expect_eq(result, null)
	expect_error("Not instantiated")

func test_invalid_function_arg_type():
	var wasm = load_wasm("simple")
	var result = wasm.function("add", [{}, 2])
	expect_eq(result, null)
	expect_error("Unsupported Godot variant type")
	expect_error("Invalid argument type")

func test_invalid_function_arg_count():
	var wasm = load_wasm("simple")
	var result = wasm.function("add", [1])
	expect_eq(result, null)
	expect_error("Incorrect number of arguments supplied")

func test_function_default_args():
	if !Utils.godot_4(): return # Default arguments are unsupported in GDNative
	var wasm = load_wasm("simple")
	var result = wasm.function("count") # Expects no arguments
	expect_eq(result, 1)
	result = wasm.function("add", [1, 2])
	expect_eq(result, 3)
	result = wasm.function("add") # Expects arguments
	expect_eq(result, null)
	expect_error("Incorrect number of arguments supplied")

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
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	var result = wasm.global("global_const")
	expect_eq(result, null)
	expect_error("Not instantiated")

func test_inspect():
	# Simple module pre-compile
	var wasm = Wasm.new()
	var inspect = wasm.inspect()
	expect_eq(inspect, {})
	expect_error("Inspection failed")
	# Simple module pre-instantiation
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	inspect = wasm.inspect()
	var expected = {
		"import_functions": {},
		"export_globals": {
			"global_const": [TYPE_FLOAT, false],
			"global_mut": [TYPE_INT, true],
		},
		"export_functions": {
			"_initialize": [[], []],
			"add": [[TYPE_INT, TYPE_INT], [TYPE_INT]],
			"count": [[], [TYPE_INT]],
		},
		"memory": {},
	}
	expect_eq(inspect, expected)
