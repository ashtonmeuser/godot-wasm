extends Control

const info_template = "[b]Import Globals[/b]\n%s[b]Imports Functions[/b]\n%s[b]Export Globals[/b]\n%s[b]Export Functions[/b]\n%s[b]Memory[/b]\n[indent]Min %s\nMax %s%s[/indent]"
var callback_count: int
@onready var wasm: Wasm = Wasm.new()

func _ready():
	$"%PrimeLimit".connect("value_changed", Callable(self, "_benchmark"))
	$"%MemoryType".connect("item_selected", Callable(self, "_update_memory_type"))
	$"%CallbackButton".connect("pressed", Callable(wasm, "function").bind("invoke_callback", []))
	for node in $"%MemoryInput".get_children() + [$"%MemoryOffset"]:
		node.connect("value_changed" if node is Range else "text_changed", Callable(self, "_update_memory"))
	for item in ["Int", "Float", "String"]: $"%MemoryType".add_item(item)

	_load_wasm("res://example.wasm")
	_update_memory()
	_benchmark()

func _gui_input(event: InputEvent): # Unfocus input
	if event is InputEventMouseButton and event.pressed:
		var focus_owner = get_viewport().gui_get_focus_owner()
		if focus_owner: focus_owner.release_focus()

func _load_wasm(path: String):
	var file = FileAccess.open(path, FileAccess.READ)
	var buffer = file.get_buffer(file.get_length())
	var imports = { # Import format module.name
		"functions": { "index.callback": [self, "callback"] },
	}
	wasm.load(buffer, imports)
	file.close()
	_update_info()

func callback(value: int):
	callback_count += 1
	$"%CallbackCount".text = "%d" % callback_count
	print("Callback invoked with value %d" % value)
	return callback_count

func _update_info():
	var info = wasm.inspect()
	if info.is_empty():
		$"%InfoText".set("text", "Error")
		return
	$"%InfoText".text = info_template % [
		_pretty_signatures({}),
		_pretty_signatures(info.import_functions),
		_pretty_signatures(info.export_globals),
		_pretty_signatures(info.export_functions),
		_pretty_bytes(info.memory_min),
		_pretty_bytes(info.memory_max),
		"\nCurrent %s" % _pretty_bytes(info.memory_current) if "memory_current" in info else "",
	]

func _update_memory_type(index: int):
	for child in $"%MemoryInput".get_children():
		child.visible = child.get_index() == index
	_update_memory()

func _update_memory(_value = 0):
	var input = $"%MemoryInput".get_child($"%MemoryType".selected)
	var offset = int($"%MemoryOffset".value)
	wasm.stream.seek(offset)
	match(input.get_index()):
		0: wasm.stream.put_64(int(input.value))
		1: wasm.stream.put_double(input.value)
		2: wasm.stream.put_data(input.text.to_utf8_buffer())
	wasm.function("update_memory", [])
	$"%GlobalValue".text = _hex(wasm.global("memory_value"))
	$"%ReadValue".text = _hex(wasm.stream.seek(0).get_64()) # Seek allows chaining

func _hex(i: int) -> String: # Format bytes without leading negative sign
	if i >= 0: return "%016X" % i
	i = i ^ 0x7FFFFFFFFFFFFFFF + 0x1 # Two's complement
	return "%X%015X" % [(i >> 60) | 0x8, i & 0x0FFFFFFFFFFFFFFF]

func _pretty_signatures(signatures: Dictionary) -> String: # Indented, line-separated string
	if signatures.keys().is_empty(): return ""
	var rows = PackedStringArray()
	for key in signatures.keys():
		var signature = signatures[key]
		assert(signature is Array and len(signature) == 2) #,"Invalid signature")
		if signature[0] is Array and signature[1] is Array: # Function signature (param and result types)
			var func_signature = ""
			if signature[0].is_empty(): func_signature += "V"
			else: for type in signature[0]: func_signature += "I" if type == TYPE_INT else "F"
			func_signature += "→"
			if signature[1].is_empty(): func_signature += "V"
			else: for type in signature[1]: func_signature += "I" if type == TYPE_INT else "F"
			rows.append("%s [code][color=#FFF5]%s[/color][/code]" % [key, func_signature])
		elif signature[0] is int and signature[1] is bool: # Global signature (type and mutability)
			rows.append("%s [code][color=#FFF5]%s(%s)[/color][/code]" % [key, "I" if signature[0] == TYPE_INT else "F", "M" if signature[1] else "C"])
	return "[indent]%s[/indent]\n" % "\n".join(rows)

func _pretty_bytes(i: int) -> String: # Format bytes without leading negative sign
	for unit in ["", "Ki", "Mi"]:
		if abs(i) < 1024.0: return "%d %sB" % [i, unit]
		i = int(round(i / 1024.0))
	return "%d GiB" % i

func _benchmark(_value = 0):
	var limit: int = $"%PrimeLimit".value
	var t_gdscript = Time.get_ticks_usec()
	var v_gdscript = Benchmark.sieve(limit)
	t_gdscript = Time.get_ticks_usec() - t_gdscript
	var t_wasm = Time.get_ticks_usec()
	var v_wasm = wasm.function("sieve", [limit])
	t_wasm = Time.get_ticks_usec() - t_wasm
	$"%PrimeAnswer".text = ("%d" % v_gdscript) if v_gdscript == v_wasm else "?"
	$"%TimeGDScript".text = "%.3f ms" % (t_gdscript / 1000.0)
	$"%TimeWasm".text = "%.3f ms" % (t_wasm / 1000.0)
