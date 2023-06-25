extends GodotWasmTestSuite

func test_fd_write():
	var wasm = load_wasm("wasi")
	wasm.function("fd_write", [])
	expect_log("Test fd_write")

func test_proc_exit():
	var wasm = load_wasm("wasi")
	wasm.function("proc_exit", [0])
	expect_log("Module exited successfully")
	wasm = load_wasm("wasi")
	wasm.function("proc_exit", [1])
	expect_error("Module exited with error 1")

func test_args_get():
	var args = OS.get_cmdline_user_args()
	var wasm = load_wasm("wasi")
	var result = wasm.function("args_get", [])
	for arg in args: expect_log(arg.trim_prefix("--"))
	expect_eq(result, args.size())

func test_environ_get():
	var wasm = load_wasm("wasi")
	var result = wasm.function("environ_get", [])
	expect_eq(result, 0)

func test_random_get():
	var wasm = load_wasm("wasi")
	var result = wasm.function("random_get", [])
	expect(abs(result) <= 0xFF)

func test_clock_time_get():
	var time = Time.get_unix_time_from_system() * 1000
	var wasm = load_wasm("wasi")
	var result = wasm.function("clock_time_get", [])
	expect(abs(time - result) < 10.0) # Within ten seconds

func test_permissions():
	var wasm = load_wasm("wasi")
	expect_contains(wasm.permissions, "print")
	expect_eq(wasm.permissions.get("print"), true)
	wasm.permissions = { "print": false }
	expect_eq(wasm.permissions.get("print"), false)
	var permission = wasm.has_permission("print")
	expect_eq(permission, false)
	wasm.permissions = { "print": true }
	permission = wasm.has_permission("print")
	expect_eq(permission, true)

func test_invalid_permissions():
	# TODO: No exit permissions causes crash on proc_exit
	# TODO: Args & env not affected by permissions
	var wasm = load_wasm("wasi")
	wasm.permissions = { "print": false }
	wasm.function("fd_write", [])
	expect_error("Failed calling function fd_write")
	wasm.permissions = { "print": true, "random": false }
	wasm.function("random_get", [])
	expect_error("Failed calling function random_get")
	wasm.permissions = { "random": true, "time": false }
	wasm.function("clock_time_get", [])
	expect_error("Failed calling function clock_time_get")
