extends GodotWasmTestSuite

func test_disable_wasi():
	var wasm = Wasm.new()
	wasm.extension("wasi", null)
	var buffer = read_file("wasi")
	var error = wasm.load(buffer, {})
	expect_eq(error, ERR_CANT_CREATE)
	wasm.extension("wasi", wasm) # Reset to default
	error = wasm.load(buffer, {})
	expect_eq(error, OK)

func test_invalid_target():
	var wasm = Wasm.new()
	var target = Node.new()
	var error = wasm.extension("wasi", target)
	expect_eq(error, OK)
	target.free()
	error = wasm.extension("wasi", target)
	expect_eq(error, ERR_INVALID_DATA)
