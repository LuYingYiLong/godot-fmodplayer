extends Control

const CPU_PROFILE: Script = preload("uid://dhqnw0413pkvm")
const SPECTRUM_TEST: Script = preload("uid://wsi7yl1eksc4")
const SDK_SMOKE_TEST: Script = preload("uid://c0i5v0a75fpfe")

var _suite: String = "all"
var _output_dir: String
var _quick: bool


func _ready() -> void:
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--suite="):
			_suite = argument.trim_prefix("--suite=")
		elif argument.begins_with("--output-dir="):
			_output_dir = argument.trim_prefix("--output-dir=")
		elif argument == "--quick":
			_quick = true
	if _output_dir.is_empty():
		var timestamp: String = Time.get_datetime_string_from_system().replace(":", "-")
		_output_dir = ProjectSettings.globalize_path("res://logs/run_" + timestamp)
	var directory_error: Error = DirAccess.make_dir_recursive_absolute(_output_dir)
	if directory_error != OK:
		push_error("Cannot create test output directory: %s" % _output_dir)
		get_tree().quit(1)
		return
	$Output.text = "Running %s tests...\n%s" % [_suite, _output_dir]
	_run.call_deferred()


func _run() -> void:
	var summary: String = "FMOD tests\n\n"
	if _suite not in ["all", "spectrum", "profile",]:
		_finish_failure("Unknown test suite: " + _suite)
		return
	if _suite in ["all", "spectrum",]:
		if not SPECTRUM_TEST.new().run():
			_finish_failure("Spectrum snapshot regression failed")
			return
		summary += "Spectrum snapshot regression: passed\n"
	if _suite == "all":
		if not SDK_SMOKE_TEST.new().run():
			_finish_failure("FMOD SDK smoke test failed")
			return
		summary += "FMOD SDK smoke test: passed\n"
	if _suite in ["all", "profile",]:
		var results: Array[Dictionary] = CPU_PROFILE.new().run(_output_dir.path_join("timings.json"), _quick)
		for result: Dictionary in results:
			summary += "%s: %.3f us/block\n" % [result["name"], result["median_us_per_update"]]
	summary += "\nResults: " + _output_dir
	$Output.text = summary
	print("TESTS_PASSED output=", _output_dir)
	if DisplayServer.get_name() == "headless":
		get_tree().quit()


func _finish_failure(message: String) -> void:
	$Output.text = message
	push_error(message)
	get_tree().quit(1)
