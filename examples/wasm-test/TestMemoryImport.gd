extends GodotWasmTestSuite

func test_exteranl_memory():
	var memory = WasmMemory.new()
	var error = memory.grow(100)
	expect_eq(error, OK)
	error = memory.grow(-1)
	expect_eq(error, FAILED)

func test_grow_memory():
	var memory = WasmMemory.new()
	var inspect = memory.inspect()
	expect_eq(inspect, {})
	memory.grow(1)
	inspect = memory.inspect()
	var expected = {
		"min": PAGE_SIZE,
		"max": PAGES_MAX,
		"current": PAGE_SIZE,
	}
	expect_eq(inspect, expected)
	memory.grow(1)
	inspect = memory.inspect()
	# Cannot test minimum as Wasmer & Wasmtime behave differently
	# Wasmer updates minimum to new size while Wasmtime does not
	expect_eq(inspect.get("max"), PAGES_MAX)
	expect_eq(inspect.get("current"), PAGE_SIZE * 2)

func test_module_memory():
	var wasm = Wasm.new()
	var buffer = read_file("memory")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	expect_eq(wasm.memory, null)
	error = wasm.instantiate({})
	expect_eq(error, OK)
	expect(wasm.memory is StreamPeer)
	expect_empty()

func test_memory_import():
	var memory = WasmMemory.new()
	var error = memory.grow(100)
	expect_eq(error, OK)
	var imports = { "memory": memory }
	var wasm = load_wasm("memory-import", imports)
	expect(wasm.memory == memory)
	var offset = wasm.global("offset")
	memory.seek(offset).put_u8(0xFF)
	var result = wasm.function("load_byte", [0])
	expect_eq(result, 0xFF)

func test_share_memory():
	var memory = WasmMemory.new()
	var error = memory.grow(100)
	expect_eq(error, OK)
	var imports = { "memory": memory }
	var wasm_a = load_wasm("memory-import", imports)
	var wasm_b = load_wasm("memory-import", imports)
	expect(wasm_a.memory == wasm_b.memory)
	wasm_a.function("store_byte", [0xFF, 0])
	var result = wasm_b.function("load_byte", [0])
	expect_eq(result, 0xFF)
