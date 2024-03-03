extends GodotWasmTestSuite

var DUMMY_IMPORT = [self, "_dummy"]

# Dummy import to supply to Wasm modules
func _dummy(a = "", b = "", c = "", d = ""):
	var message = "Dummy import %s %s %s %s" % [a, b, c, d]
	print(message.strip_edges())

func test_imports():
	var imports = { "functions": {
		"import.import_int": DUMMY_IMPORT,
		"import.import_float": DUMMY_IMPORT,
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
		"import.import_float": DUMMY_IMPORT,
	} }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import function import.import_int")
	# Invalid import target
	imports = { "functions": {
		"import.import_int": [0, "dummy"],
		"import.import_float": DUMMY_IMPORT,
	} }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import target import.import_int")
	# Invalid import method
	imports = { "functions": {
		"import.import_int": [self, 0],
		"import.import_float": DUMMY_IMPORT,
	} }
	error = wasm.instantiate(imports)
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Invalid import method import.import_int")

func test_callback_function():
	var imports = { "functions": {
		"import.import_int": DUMMY_IMPORT,
		"import.import_float": DUMMY_IMPORT,
	} }
	var wasm = load_wasm("import", imports)
	wasm.function("callback", [])
	expect_log("Dummy import -123")
	expect_log("Dummy import -12.34")

func test_inspect():
	# Import module post-instantiation
	var imports = { "functions": {
		"import.import_int": DUMMY_IMPORT,
		"import.import_float": DUMMY_IMPORT,
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
