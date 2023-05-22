extends TestSuite
class_name GodotWasmTestSuite

const TYPE_FLOAT: int = TYPE_REAL
const PAGE_SIZE: int = 0b1 << 16
const PAGES_MAX: int = PAGE_SIZE * (PAGE_SIZE - 1)

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
static func _dummy_import(a = "", b = "", c = "", d = ""):
	var message = "Dummy import %s %s %s %s" % [a, b, c, d]
	print(message.strip_edges())
	return a

func dummy_import() -> Array:
	return [self, "_dummy_import"]

func make_bytes(data: Array):
	return PoolByteArray(data)

func get_cmdline_user_args() -> Array:
	return []
