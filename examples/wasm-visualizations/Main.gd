extends Node

const sizes: PoolVector2Array = PoolVector2Array([Vector2(50, 50), Vector2(100, 100), Vector2(200, 200), Vector2(400, 400), Vector2(800, 800), Vector2(1600, 1600)])
const modules: PoolStringArray = PoolStringArray(["interference", "mandelbrot", "life", "wave", "sort"])

var texture: ImageTexture = ImageTexture.new()
var image: Image = Image.new()
var module: String = modules[0]
var size: Vector2 = sizes[2]
var ticks: int = 0
onready var wasm: Wasm = Wasm.new()

func _ready():
	$TextureRect.texture = texture
	seed(OS.get_system_time_msecs())
	_change_module()
	_change_size()
	_show_info()

func _input(event):
	if event.is_action_pressed("ui_up"): _change("size", 1)
	elif event.is_action_pressed("ui_down"): _change("size", -1)
	elif event.is_action_pressed("ui_left"): _change("module", -1)
	elif event.is_action_pressed("ui_right"): _change("module", 1)
	elif event.is_action_pressed("toggle_fps_cap"): _change("fps_cap")

func _gui_input(event):
	if !(event is InputEventMouseButton) or !event.pressed: return
	if get_node_or_null("Intro"): $Intro.queue_free()
	if !("interact" in wasm.inspect().export_functions): return
	var p = _localize_aspect_fit(event.position, $TextureRect.rect_size, size)
	if p.x < 0.0 or p.x > 1.0 or p.y < 0.0 or p.y > 1.0: return
	wasm.function("interact", [p.x, p.y, 1.0 if event.button_index == BUTTON_LEFT else 0.0 ])

func _process(_delta):
	$"%LabelFPS".text = "%d FPS" % Performance.get_monitor(Performance.TIME_FPS)
	wasm.function("update", [ticks, OS.get_ticks_msec() / 1000.0])
	ticks += 1

func _load_wasm(path: String):
	var file = File.new()
	file.open(path, File.READ)
	var buffer = file.get_buffer(file.get_len())
	var imports = { "functions": {
		"env.abort": [self, "_abort"],
		"env.draw_image": [self, "_draw_image"],
		"env.draw_pixel": [self, "_draw_pixel"],
		"env.seed": [self, "_seed"],
	} }
	wasm.load(buffer, imports)
	file.close()

func _change(property: String, increment: int = 0):
	if get_node_or_null("Intro"): $Intro.queue_free()
	call_deferred("_change_%s" % property, increment)
	call_deferred("_show_info")

func _change_module(increment: int = 0):
	var index = modules.find(module) + increment
	if index < 0 or index >= len(modules): return
	module = modules[index]
	_load_wasm("Modules/%s.wasm" % module)
	wasm.function("resize", [int(size.x), int(size.y)])
	ticks = 0

func _change_size(increment: int = 0):
	var index = sizes.find(size) + increment
	if index < 0 or index >= len(sizes): return
	size = sizes[index]
	wasm.function("resize", [int(size.x), int(size.y)])
	image.create(int(size.x), int(size.y), false, Image.FORMAT_RGBA8)
	ticks = 0

func _change_fps_cap(_increment: int = 0):
	Engine.target_fps = 0 if Engine.target_fps else 60

func _show_info():
	$"%LabelModule".text = module
	$"%LabelSize".text = "%d X %d" % [size.x, size.y]
	$Tween.remove_all()
	$Tween.interpolate_property($Info, "modulate:a", $Info.modulate.a, 1.0, 0.2, Tween.TRANS_CUBIC, Tween.EASE_IN_OUT, 0.0)
	$Tween.interpolate_property($Info, "rect_position:y", $Info.rect_position.y, 12, 0.2, Tween.TRANS_CUBIC, Tween.EASE_IN_OUT, 0.0)
	$Tween.interpolate_property($Info, "modulate:a", 1.0, 0.0, 0.6, Tween.TRANS_CUBIC, Tween.EASE_IN_OUT, 6.0)
	$Tween.interpolate_property($Info, "rect_position:y", 12, -98, 0.6, Tween.TRANS_CUBIC, Tween.EASE_IN_OUT, 6.0)
	$Tween.start()

func _localize_aspect_fit(p: Vector2, outer: Vector2, inner: Vector2):
	var scale = outer.y / inner.y if outer.aspect() > inner.aspect() else outer.x / inner.x
	var scaled = inner * scale
	var offset = (outer - scaled) / 2
	return (p - offset) / scaled

# Wasm module imports

func _abort(a: int, b: int, c: int, d: int) -> void: # Throw error from Wasm module
	push_error("Abort from Wasm module: %d %d %d %d" % [a, b, c, d])

func _draw_image(p: int, s: int) -> void: # Draw the entire image from Wasm memory
	image.lock()
	image.data.data = wasm.stream.seek(p).get_data(s)[1]
	image.unlock()
	texture.create_from_image(image, 0)

func _draw_pixel(x: int, y: int, r: int, g: int, b: int, a: int) -> void: # Draw a single pixel with color component values
	image.lock()
	image.set_pixel(x, y, Color8(r, g, b, a))
	image.unlock()
	texture.create_from_image(image, 0)

func _seed() -> float: # Provide a random seed to the Wasm module
	return randf()
