extends GodotWasmTestSuite

func test_compile():
	var wasm = Wasm.new()
	var buffer = read_file("wasi")
	var error = wasm.compile(buffer)
	expect_eq(error, OK)
	expect_empty()

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
	var args = get_cmdline_user_args()
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
	expect_within(result, time, 1000.0) # Within one second

func test_disable_extension():
	var wasm = Wasm.new()
	var buffer = read_file("wasi")
	expect_includes(wasm.extensions, "wasi_preview1")
	wasm.extensions = [] # Remove all extensions
	var error = wasm.load(buffer, {})
	expect_eq(error, ERR_CANT_CREATE)
	expect_error("Missing import function wasi_snapshot_preview1.args_get")
