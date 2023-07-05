extends TestSuite
class_name GodotWasmTestSuite

# Test suite overrides

func expect_error(s: String):
	.expect_error("(Godot Wasm: )?" + s)

# General utils

func load_wasm(f: String, imports: Dictionary = {}, e: int = OK) -> Wasm:
	var wasm = Wasm.new()
	var buffer = read_file(f)
	var error = wasm.load(buffer, imports)
	expect_eq(error, e)
	return wasm

func read_file(f: String) -> PoolByteArray:
	var file = File.new()
	file.open("res://wasm/%s.wasm" % f, File.READ)
	return file.get_buffer(file.get_len())

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
	return PoolByteArray(data)

func get_cmdline_user_args() -> Array:
	return []
