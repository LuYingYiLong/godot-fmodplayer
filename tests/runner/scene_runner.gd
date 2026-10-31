extends Control

const SUITE_RUNNER: Script = preload("uid://bvy1vaxly634y")

@onready var _output: Label = $Output


func _ready() -> void:
	_output.text = "正在运行数值回归..."
	_run.call_deferred()


func _run() -> void:
	var runner: RefCounted = SUITE_RUNNER.new()
	var output_dir: String
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--output-dir="):
			output_dir = argument.trim_prefix("--output-dir=")
	if output_dir.is_empty():
		var timestamp: String = Time.get_datetime_string_from_system().replace(":", "-")
		output_dir = ProjectSettings.globalize_path("res://logs/runs/regression_" + timestamp)
	var result: Dictionary = runner.run(output_dir)
	var passed: bool = bool(result.get("passed", false))
	_output.text = String(result.get("summary", "数值回归未生成结果"))
	if not passed:
		push_error(_output.text)
	else:
		print("TESTS_PASSED output=", output_dir)
	if DisplayServer.get_name() == "headless":
		get_tree().quit(0 if passed else 1)
