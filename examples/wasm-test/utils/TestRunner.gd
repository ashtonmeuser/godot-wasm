extends Node
class_name TestRunner

enum LogLevel { Default, Success, Error, Title }
enum LogDestination { All, UI }

onready var _log_label: RichTextLabel = get_tree().get_current_scene() as RichTextLabel
onready var _log_file = File.new()

func _ready():
	record("Log dir: %s" % OS.get_user_data_dir())

	_log_file.open("user://logs/test.log", File.READ)

	var results = Results.new()

	var regex = Utils.make_regex("^Test\\w+\\.gd")
	var dir = Directory.new()
	if dir.open("res://") != OK: return get_tree().quit(1)
	dir.list_dir_begin()
	while true:
		var file = dir.get_next()
		if !file: break
		if regex.search(file) == null: continue
		var script = load(file) as Script
		if !inherits(script, TestSuite): continue
		var suite: TestSuite = script.new()
		# Run suite
		record("Running test suite: %s" % file)
		suite.connect("test_start", self, "handle_test_start", [results])
		suite.connect("test_error", self, "handle_test_error")
		suite.connect("test_pass", self, "handle_test_pass", [results])
		suite.connect("test_fail", self, "handle_test_fail", [results])
		suite.run(_log_file)

	record("Tests complete", LogLevel.Title)
	record("Passed: %d/%d" % [results.passed, results.total], LogLevel.Error if results.failed else LogLevel.Success)

	if !OS.get_cmdline_args().has("--keepalive=yes"): get_tree().quit(results.failed)

func _exit_tree():
	_log_file.close()

# Test event handlers

func handle_test_start(case: String, results: Results):
	record("Running test: %s" % case, LogLevel.Title)
	results.total += 1

func handle_test_error(message: String):
	record(message, LogLevel.Error)

func handle_test_pass(case: String, results: Results):
	record("Test passed: %s" % case, LogLevel.Success)
	results.passed += 1

func handle_test_fail(case: String, results: Results):
	record("Test failed: %s" % case, LogLevel.Error)
	results.failed += 1

# General utils

class Results:
	var total: int
	var passed: int
	var failed: int

func inherits(child: Script, parent: Script) -> bool:
	while child:
		if child == parent: return true
		child = child.get_base_script()
	return false

# Logging

func record(s, l: int = LogLevel.Default, d: int = LogDestination.All):
	if d != LogDestination.UI: record_console(s, l)
	record_ui(s, l)

func record_console(s, l: int):
	if l == LogLevel.Title: s = "----- %s -----" % s
	if l == LogLevel.Error: push_error(s)
	else: print(s)

func record_ui(s, l: int):
	if !_log_label: return
	if l == LogLevel.Success: s = "[color=green]%s[/color]" % s
	elif l == LogLevel.Error: s = "[color=red]%s[/color]" % s
	elif l == LogLevel.Title: s = "[b]%s[/b]" % s
	_log_label.bbcode_text += "%s\n" % s
