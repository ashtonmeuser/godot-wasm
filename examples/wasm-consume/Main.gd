extends Control

onready var operand1: Range = $VBoxContainer/HBoxContainer/Operand1
onready var operand2: Range = $VBoxContainer/HBoxContainer/Operand2
onready var wasm: Wasm = Wasm.new()

func _ready():
	_load_wasm("res://add.wasm")
	for child in $VBoxContainer/HBoxContainer.get_children():
		if child is Range: child.connect("value_changed", self, "_handle_value_changed")
	_handle_value_changed()
	$VBoxContainer/Constant.text += String(wasm.global("global_const"))
	$VBoxContainer/Mutable.text += String(wasm.global("global_var"))

func _gui_input(event: InputEvent):
	if event is InputEventMouseButton and event.pressed:
		var focus_owner = get_focus_owner()
		if focus_owner: focus_owner.release_focus()

func _handle_value_changed(_value: float = 0.0):
	var args = [int(operand1.value), int(operand2.value)]
	var answer = wasm.function("add", args)
	$VBoxContainer/HBoxContainer/Answer.text = String(answer)

func _load_wasm(path: String):
	var file = File.new()
	file.open(path, File.READ)
	var buffer = file.get_buffer(file.get_len())
	wasm.load(buffer)
	file.close()
