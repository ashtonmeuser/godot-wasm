extends TestSuite
class_name GodotWasmTestSuite

# Test suite overrides

func expect_error(s: String):
	super.expect_error("(Godot Wasm: )?" + s)

# General utils

func load_wasm(f: String, imports: Dictionary = {}, e: Error = OK) -> Wasm:
	var wasm = Wasm.new()
	var buffer = read_file(f)
	var error = wasm.load(buffer, imports)
	expect_eq(error, e)
	return wasm

func read_file(f: String) -> PackedByteArray:
	return FileAccess.get_file_as_bytes("res://wasm/%s.wasm" % f)

# Dummy import to supply to Wasm modules
static func dummy(a = "", b = "", c = "", d = ""):
	var message = "Dummy import %s %s %s %s" % [a, b, c, d]
	print(message.strip_edges())

func dummy_imports(functions: Array = []) -> Dictionary:
	var imports = { "functions": {} }
	for function in functions:
		imports.functions[function] = [self, "dummy"]
	return imports

func make_bytes(data: Array):
	return PackedByteArray(data)

func get_cmdline_user_args() -> Array:
	return OS.get_cmdline_user_args()
