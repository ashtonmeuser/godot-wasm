extends RefCounted
class_name TestSuite

signal test_start(case: String)
signal test_error(message: String)
signal test_pass(case: String)
signal test_fail(case: String)

var _runner: TestRunner # Responsible for running this test suite
var _log_file: FileAccess # Log file used to check for output and/or errors
var _error: bool = false # If the current test case has failed

func run(r: TestRunner, f: FileAccess):
	_runner = r
	_log_file = f

	# Run all tests
	for method in get_method_list():
		if !method.name.begins_with("test_"): continue
		_error = false # Reset test case error flag
		emit_signal("test_start", method.name) # Alert runner of test case
		_log_file.seek_end() # Go to end of log file
		call(method.name) # Run test
		if _error: emit_signal("test_fail", method.name)
		else: emit_signal("test_pass", method.name)

# Test condition utils

func expect_log(s: String):
	_log_file.seek(_log_file.get_position()) # HACK: Not sure why this works
	var line = _log_file.get_line()
	var regex: RegEx = RegEx.new()
	if regex.compile("^%s$" % s): _fail("Invalid regex: %s" % s)
	if regex.search(line) == null: _fail("Expect log: %s != %s" % [line, s])

func expect_empty():
	expect_log("")

func expect_error(s: String):
	expect_log("(USER )?ERROR: " + s)
	# Account for error stack trace
	var regex = RegEx.create_from_string("^\\s+at:\\s")
	var position = _log_file.get_position()
	while _log_file.get_position() < _log_file.get_length():
		if !regex.search(_log_file.get_line()): break
		position = _log_file.get_position()
	_log_file.seek(position)

func expect(a):
	if !a: _fail("Expect truthy: %s" % a)

func expect_eq(a, b):
	if a != b: _fail("Expect equal: %s != %s" % [a, b])

func expect_ne(a, b):
	if a == b: _fail("Expect not equal: %s == %s" % [a, b])

func expect_contains(o, v: String):
	if o is Dictionary:
		if !o.keys().has(v): _fail("Expect contains: %s ∉ %s" % [v, o.keys()])
	elif o is Array:
		if !o.has(v): _fail("Expect contains: %s ∉ %s" % [v, o])
	else: _fail("Expect contains: Invalid object")

# General utils

func _fail(message: String):
	_error = true # Flag test case failed
	emit_signal("test_error", message)
	print_stack()
