extends Control

onready var wasm: Wasm = Wasm.new()

func _ready():
	$"%PrimeLimit".connect("value_changed", self, "_benchmark")
	$"%MemoryType".connect("item_selected", self, "_update_memory_type")
	for node in $"%MemoryInput".get_children() + [$"%MemoryOffset"]:
		node.connect("value_changed" if node is Range else "text_changed", self, "_update_memory")
	for item in ["Int", "Float", "String"]: $"%MemoryType".add_item(item)

	_load_wasm("res://example.wasm")
	_update_memory()
	_benchmark()

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
	_update_info()

func _update_info():
	var info = wasm.inspect()
	if !info: return $"%InfoText".set("text", "Error")
	$"%InfoText".bbcode_text = "[b]Globals[/b]\n[indent]%s\n[/indent][b]Functions[/b]\n[indent]%s\n[/indent][b]Memory[/b]\n[indent]%d B[/indent]" % [
		PoolStringArray(info.globals).join("\n"),
		PoolStringArray(info.functions).join("\n"),
		info.memory,
	]

func _update_memory_type(index: int):
	for child in $"%MemoryInput".get_children():
		child.visible = child.get_index() == index
	_update_memory()

func _update_memory(_value = 0):
	var input = $"%MemoryInput".get_child($"%MemoryType".selected)
	var offset = int($"%MemoryOffset".value)
	var value # Hold variant to be written to memory
	match(input.get_index()):
		0: value = int(input.value)
		1: value = input.value
		2: value = input.text
	wasm.mem_write(value, offset)
	wasm.function("update_memory", [])
	$"%GlobalValue".text = "%016X" % wasm.global("memory_value")
	$"%ReadValue".text = "%016X" % wasm.mem_read(TYPE_INT, 0, 0)

func _benchmark(_value = 0):
	var limit: int = $"%PrimeLimit".value
	var t_gdscript = OS.get_ticks_usec()
	var v_gdscript = Benchmark.sieve(limit)
	t_gdscript = OS.get_ticks_usec() - t_gdscript
	var t_wasm = OS.get_ticks_usec()
	var v_wasm = wasm.function("sieve", [limit])
	t_wasm = OS.get_ticks_usec() - t_wasm
	$"%PrimeAnswer".text = String(v_gdscript) if v_gdscript == v_wasm else "?"
	$"%TimeGDScript".text = "%.3f ms" % (t_gdscript / 1000.0)
	$"%TimeWasm".text = "%.3f ms" % (t_wasm / 1000.0)
