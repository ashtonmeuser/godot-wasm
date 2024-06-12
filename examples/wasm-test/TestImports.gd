extends GodotWasmTestSuite

func test_imports():
	var imports = { "functions": {
		"import.import_int": dummy_import(),
		"import.import_float": dummy_import(),
	} }
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
	expect_error("Missing import function import.import_(int|float)")
	# Invalid import
	imports = { "functions": {
		"import.import_int": [],
		"import.import_float": dummy_import(),
	} }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import function import.import_int")
	# Invalid import target
	imports = { "functions": {
		"import.import_int": [0, "dummy"],
		"import.import_float": dummy_import(),
	} }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import target import.import_int")
	# Invalid import method
	imports = { "functions": {
		"import.import_int": [self, 0],
		"import.import_float": dummy_import(),
	} }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import method import.import_int")

func test_instantiation_freed_target():
	var target = ImportTarget.new()
	var imports = { "functions": {
		"import.import_int": target.dummy_import(),
		"import.import_float": target.dummy_import(),
	} }
	var wasm = Wasm.new()
	var buffer = read_file("import")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	target.free() # Free target before instantiation
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import target import.import_(int|float)")

func test_invocation_freed_target():
	var target = ImportTarget.new()
	var imports = { "functions": {
		"import.import_int": target.dummy_import(),
		"import.import_float": target.dummy_import(),
	} }
	var wasm = load_wasm("import", imports)
	target.free() # Free target before invocation
	wasm.function("callback", [])
	expect_error("Failed to retrieve import function target")
	expect_error("Failed calling function callback")

func test_import_method_missing():
	var imports = { "functions": {
		"import.import_int": [self, "invalid_method_0"],
		"import.import_float": [self, "invalid_method_1"],
	} }
	var wasm = load_wasm("import", imports)
	wasm.function("callback", [])
	expect_error(".*::invalid_method_0': Method not found.*")
	expect_error(".*::invalid_method_1': Method not found.*")

func test_callback_function():
	var imports = { "functions": {
		"import.import_int": dummy_import(),
		"import.import_float": dummy_import(),
	} }
	var wasm = load_wasm("import", imports)
	wasm.function("callback", [])
	expect_log("Dummy import -123")
	expect_log("Dummy import -12.34")

func test_inspect():
	# Import module post-instantiation
	var imports = { "functions": {
		"import.import_int": dummy_import(),
		"import.import_float": dummy_import(),
	} }
	var wasm = load_wasm("import", imports)
	var inspect = wasm.inspect()
	var expected = {
		"import_functions": {
			"import.import_float": [[TYPE_FLOAT], []],
			"import.import_int": [[TYPE_INT], []],
		},
		"export_globals": {},
		"export_functions": {
			"_initialize": [[], []],
			"callback": [[], []],
		},
		"memory": {
			"min": 0,
			"max": PAGES_MAX,
			"current": 0,
		}
	}
	expect_eq(inspect, expected)
	expect_empty()
