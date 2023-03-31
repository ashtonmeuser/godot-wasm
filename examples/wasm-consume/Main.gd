extends Control

onready var wasm: Wasm = Wasm.new()

func _ready():
	_load_wasm("res://example.wasm")
	get_tree().create_timer(1.0).connect("timeout", self, "_suite")

func _gui_input(event: InputEvent): # Unfocus input
	if event is InputEventMouseButton and event.pressed:
		var focus_owner = get_focus_owner()
		if focus_owner: focus_owner.release_focus()

func _load_wasm(path: String):
	var file = File.new()
	file.open(path, File.READ)
	var buffer = file.get_buffer(file.get_len())
	wasm.load(buffer)
	file.close()

func _suite():
	var n = 1000
	var funcs = {
		"gdscript_fibonacci_20": [funcref(Benchmark, "fibonacci"), [20]],
		"gdscript_fibonacci_50": [funcref(Benchmark, "fibonacci"), [50]],
		"gdscript_sieve_1000": [funcref(Benchmark, "sieve"), [1000]],
		"gdscript_sieve_10000": [funcref(Benchmark, "sieve"), [10000]],
		"gdscript_sieve_100000": [funcref(Benchmark, "sieve"), [100000]],
		"wasm_fibonacci_20": [funcref(wasm, "function"), ["fibonacci", [20]]],
		"wasm_fibonacci_50": [funcref(wasm, "function"), ["fibonacci", [50]]],
		"wasm_sieve_1000": [funcref(wasm, "function"), ["sieve", [1000]]],
		"wasm_sieve_10000": [funcref(wasm, "function"), ["sieve", [10000]]],
		"wasm_sieve_100000": [funcref(wasm, "function"), ["sieve", [100000]]],
		"gdnative_fibonacci_20": [funcref(wasm, "fibonacci"), [20]],
		"gdnative_fibonacci_50": [funcref(wasm, "fibonacci"), [50]],
		"gdnative_sieve_1000": [funcref(wasm, "sieve"), [1000]],
		"gdnative_sieve_10000": [funcref(wasm, "sieve"), [10000]],
		"gdnative_sieve_100000": [funcref(wasm, "sieve"), [100000]],
	}
	for key in funcs.keys():
		var f = funcs[key][0]
		var args = funcs[key][1]
		var data = _benchmark(f, args, n)
		var path = _write_file(data, key)
		print(path)

func _benchmark(f: FuncRef, args: Array, n: int) -> Array:
	var data = []
	for _i in n:
		var t = OS.get_ticks_usec()
		f.call_funcv(args)
		t = OS.get_ticks_usec() - t
		data.append(t)
	return data

func _write_file(data: Array, filename: String) -> String:
	var file = File.new()
	file.open("user://%s.csv" % filename, File.WRITE)
	var path = file.get_path_absolute()
	for i in data: file.store_line(String(i))
	file.close()
	return path
