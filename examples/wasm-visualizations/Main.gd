extends Node

export var texture: ImageTexture = ImageTexture.new()
export var image: Image = Image.new()
export var size: Vector2 = Vector2(200, 200)
onready var wasm: Wasm = Wasm.new()

func _ready():
	_load_wasm("Modules/interference.wasm")
	wasm.function("resize", [int(size.x), int(size.y)])
	image.create(int(size.x), int(size.y), false, Image.FORMAT_RGBA8)
	$TextureRect.texture = texture

func _process(_delta):
	$FPS.text = "%d FPS" % Performance.get_monitor(Performance.TIME_FPS)
	wasm.function("update", [OS.get_ticks_msec() / 1000.0])

func _load_wasm(path: String):
	var file = File.new()
	file.open(path, File.READ)
	var buffer = file.get_buffer(file.get_len())
	var imports = { "functions": {
		"env.abort": [self, "_error"],
		"env.draw_image": [self, "_draw_image"],
		"env.draw_pixel": [self, "_draw_pixel"],
	} }
	wasm.load(buffer, imports)
	file.close()

# Wasm module imports

func _error(a: int, b: int, c: int, d: int):
	# Throw error from Wasm module
	push_error("Abort from Wasm module: %d %d %d %d" % [a, b, c, d])

func _draw_image(p: int, s: int):
	# Draw the entire image from Wasm memory
	image.lock()
	image.data.data = wasm.stream.seek(p).get_data(s)[1]
	image.unlock()
	texture.create_from_image(image, 0)

func _draw_pixel(x: int, y: int, r: int, g: int, b: int, a: int):
	# Draw a single pixel with color component values
	pass
