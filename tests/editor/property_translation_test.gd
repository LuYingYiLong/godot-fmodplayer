@tool
extends EditorPlugin

const EXPECTED: Dictionary = {
	"Show Startup Banner": "显示启动横幅",
	"Dsp Buffer Size": "DSP 缓冲区大小",
	"Init Flags": "初始化标志",
}


func _enter_tree() -> void:
	get_tree().create_timer(60.0).timeout.connect(_on_timeout)
	_run.call_deferred()


func _run() -> void:
	var tree: SceneTree = get_tree()
	while EditorInterface.get_resource_filesystem().is_scanning():
		await tree.process_frame
	await tree.process_frame
	var output_dir: String
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--output-dir="):
			output_dir = argument.trim_prefix("--output-dir=")
	if output_dir.is_empty():
		var timestamp: String = Time.get_datetime_string_from_system().replace(":", "-")
		output_dir = ProjectSettings.globalize_path("res://logs/runs/editor_" + timestamp)
	if DirAccess.make_dir_recursive_absolute(output_dir) != OK:
		push_error("Cannot create editor test output directory")
		tree.quit(1)
		return
	TranslationServer.set_locale("zh_Hans")
	var domain: TranslationDomain = TranslationServer.get_or_add_domain("godot.properties")
	var checks: Array[Dictionary]
	var passed: bool = true
	for key: String in EXPECTED:
		var actual: String = String(domain.translate(key))
		var expected: String = String(EXPECTED[key])
		var matched: bool = actual == expected
		checks.append({
			"key": key,
			"actual": actual,
			"expected": expected,
			"passed": matched,
		})
		passed = passed and matched
		print("FMOD_TRANSLATION_TEST ", key, "=", matched)
	var report: FileAccess = FileAccess.open(output_dir.path_join("editor_translation.json"), FileAccess.WRITE)
	if report == null:
		push_error("Cannot write editor translation report")
		tree.quit(1)
		return
	report.store_string(JSON.stringify({
		"passed": passed,
		"checks": checks,
	}, "\t"))
	report.close()
	if passed:
		print("TESTS_PASSED output=", output_dir)
	else:
		push_error("FMOD property translations did not match")
	tree.quit(0 if passed else 1)


func _on_timeout() -> void:
	push_error("Editor translation test timed out")
	get_tree().quit(1)
