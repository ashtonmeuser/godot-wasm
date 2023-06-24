extends GodotWasmTestSuite

func test_invalid_permissions():
	var wasm = load_wasm("hello")
	wasm.permissions = { "print": false }
	wasm.function("hello", [])
	expect_error("Failed calling function hello")
