extends TestSuite
class_name GodotWasmTestSuite

const PAGE_SIZE: int = 0b1 << 16
const PAGES_MAX: int = PAGE_SIZE * (PAGE_SIZE - 1)

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
static func _dummy_import(a = "", b = "", c = "", d = ""):
	var message = "Dummy import %s %s %s %s" % [a, b, c, d]
	print(message.strip_edges())
	return a

func dummy_import() -> Array:
	return [self, "_dummy_import"]

func make_bytes(data: Array):
	return PackedByteArray(data)

func get_cmdline_user_args() -> Array:
	return OS.get_cmdline_user_args()
