extends GodotWasmTestSuite

func test_inspect_compiled():
	var wasm = Wasm.new()
	var buffer = read_file("simple")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	var inspect = wasm.inspect()
	var expected = {
		"import_functions": {},
		"export_globals": {
			"global_const": [TYPE_FLOAT, false],
			"global_mut": [TYPE_INT, true],
		},
		"export_functions": {
			"_initialize": [[], []],
			"add": [[TYPE_INT, TYPE_INT], [TYPE_INT]]
		},
		"memory": {}
	}
	expect_eq(inspect, expected)

func test_inspect_memory():
	# Unexported memory instantiated
	var wasm = load_wasm("simple")
	var inspect = wasm.inspect()
	expect_eq(inspect.get("memory"), {})
	# Exported memory pre-instantiation
	var buffer = read_file("memory")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	inspect = wasm.inspect()
	var expected = {
		"min": PAGE_SIZE,
		"max": PAGES_MAX,
	}
	expect_eq(inspect.get("memory"), expected)
	# Exported memory post-instantiation
	error = wasm.instantiate({})
	expect_eq(error, OK)
	inspect = wasm.inspect()
	expected = {
		"min": PAGE_SIZE,
		"max": PAGES_MAX,
		"current": PAGE_SIZE,
	}
	expect_eq(inspect.get("memory"), expected)

func test_module_memory():
	var wasm = load_wasm("memory")
	wasm.function("store_byte", [0xFF, 0])
	var result = wasm.function("load_byte", [0])
	expect_eq(result, 0xFF)

func test_memory_ops():
	var wasm = load_wasm("memory")
	var offset = wasm.global("offset")
	# Write memory, read module
	wasm.memory.seek(offset).put_u8(0xF0)
	var result = wasm.function("load_byte", [0])
	expect_eq(result, 0xF0)
	# Write module, read memory
	wasm.function("store_byte", [0xF1, 0])
	result = wasm.memory.seek(offset).get_u8()
	expect_eq(result, 0xF1)
	# Raw bytes
	var data = make_bytes([0xF0, 0xF1, 0xF2, 0xF3])
	var error = wasm.memory.seek(offset).put_data(data)
	expect_eq(error, OK)
	result = wasm.memory.seek(offset).get_data(4)
	expect_eq(result.size(), 2)
	expect_eq(result.front(), OK)
	expect_eq(result.back(), data)

func test_memory_size():
	# TODO: Available bytes always returns zero
	var wasm = load_wasm("memory")
	expect_eq(wasm.memory.get_available_bytes(), 0)

func test_memory_position():
	var wasm = load_wasm("memory")
	var offset = wasm.global("offset")
	expect_eq(wasm.memory.get_position(), 0)
	wasm.memory.seek(offset)
	expect_eq(wasm.memory.get_position(), offset)

func test_memory_position_increment():
	var wasm = load_wasm("memory")
	var offset = wasm.global("offset")
	# Auto-increment raw bytes
	var data = make_bytes([0xF0, 0xF1, 0xF2, 0xF3])
	var error = wasm.memory.seek(offset).put_data(data)
	expect_eq(error, OK)
	expect_eq(wasm.memory.get_position(), offset + 4)
	# Auto-increment by byte
	wasm.memory.seek(offset)
	for i in 4:
		var result = wasm.memory.get_u8()
		expect_eq(result, data[i])
	expect_eq(wasm.memory.get_position(), offset + 4)
	# Auto-increment by chunk
	wasm.memory.seek(offset)
	for i in range(0, 4, 2):
		var result = wasm.memory.get_u16()
		expect_eq(result, data[i] | data[i + 1] << 8)
	expect_eq(wasm.memory.get_position(), offset + 4)

func test_memory_marshal():
	var wasm = load_wasm("memory")
	var offset = wasm.global("offset")
	var data = { "array": [0, 1, 2], "string": "value" }
	wasm.memory.seek(offset).put_var(data)
	var result = wasm.memory.seek(offset).get_var()
	expect_eq(result, data)

func test_string_sum():
	var wasm = load_wasm("memory")
	var offset = wasm.global("offset")
	var string = "test_string" # String to store in Wasm memory
	var sum = 0 # Sum of character UTF8 values
	for c in Utils.to_utf8(string): sum += c
	# Write string to memory using StreamPeer interface
	# Note that Godot string marshalling prepends 32-bit string length
	# See https://docs.godotengine.org/en/stable/classes/class_streampeer.html#class-streampeer-method-put-string
	wasm.memory.seek(offset).put_string(string)
	var result = wasm.function("ascii_sum", [])
	expect_eq(result, sum)

func test_resize():
	var wasm = load_wasm("memory")
	var memory = wasm.inspect().get("memory").get("current")
	expect_eq(memory, PAGE_SIZE * 1)
	wasm.function("resize", [PAGE_SIZE])
	memory = wasm.inspect().get("memory").get("current")
	expect_eq(memory, PAGE_SIZE * 3)
