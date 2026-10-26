extends Control

const SUITE_RUNNER: Script = preload("uid://bvy1vaxly634y")

@onready var _output: Label = $Output


func _ready() -> void:
	_output.text = "Running FMOD tests..."
	_run.call_deferred()


func _run() -> void:
	var runner: RefCounted = SUITE_RUNNER.new()
	var result: Dictionary = runner.run(OS.get_cmdline_user_args())
	var passed: bool = bool(result.get("passed", false))
	_output.text = String(result.get("summary", "Test runner failed"))
	if not passed:
		push_error(_output.text)
		get_tree().quit(1)
		return
	print("TESTS_PASSED output=", result["output_dir"])
	if DisplayServer.get_name() == "headless":
		get_tree().quit()
