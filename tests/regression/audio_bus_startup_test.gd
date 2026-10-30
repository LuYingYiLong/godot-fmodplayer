extends SceneTree


func _initialize() -> void:
	_run.call_deferred()


func _run() -> void:
	var mode: String = "valid"
	var output_dir: String
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--startup-mode="):
			mode = argument.trim_prefix("--startup-mode=")
		elif argument.begins_with("--output-dir="):
			output_dir = argument.trim_prefix("--output-dir=")
	var layout: FmodAudioBusLayout = FmodServer.get_audio_bus_layout()
	var passed: bool = layout != null and layout.validate() == OK
	if mode in ["missing", "invalid",]:
		passed = passed and layout.get_bus_ids() == PackedInt64Array([0,])
	else:
		passed = passed and layout.get_bus_count() == 2 and layout.find_bus_id(&"Music") == 1
		passed = passed and layout.get_bus_effect(1, 1) is FmodAudioEffectEQ10
		passed = passed and FmodServer.get_bus_effect_instance(1, 1) is FmodAudioEffectEQ10
	if mode == "pack":
		passed = passed and FileAccess.file_exists("res://assets/mixer/bus_eq.tres")
		passed = passed and not FileAccess.file_exists("res://assets/unselected.tres")
	var report: FileAccess = FileAccess.open(output_dir.path_join("startup_" + mode + ".json"), FileAccess.WRITE)
	if report != null:
		report.store_string(JSON.stringify({"passed": passed, "mode": mode, "buses": FmodServer.get_bus_names(),}, "\t"))
	if passed:
		print("TESTS_PASSED startup=", mode)
	else:
		push_error("Audio bus startup check failed: " + mode)
	quit(0 if passed else 1)
