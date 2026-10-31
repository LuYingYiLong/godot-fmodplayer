@tool
extends EditorPlugin

const PANNER_TEST: Script = preload("uid://baniq7dsj03n6")
const EQ_TEST: Script = preload("uid://bytlq7sae4d6k")
const FILTER_TEST: Script = preload("uid://bykw2cfx5q3ua")
const COMPRESSOR_TEST: Script = preload("uid://bu0tqv2p3knw7")
const CAPTURE_TEST: Script = preload("uid://boc5jv28m3rxa")
const AMPLIFY_TEST: Script = preload("uid://bqm8sx7e5r1kc")
const CHORUS_TEST: Script = preload("uid://c1n6f5rdq3pxv")
const REVERB_TEST: Script = preload("uid://cjv5y8trh2m6p")
const PITCH_SHIFT_TEST: Script = preload("uid://cx2gm7dnh5qb4")
const PHASER_TEST: Script = preload("uid://c48swpv3bn2yh")
const STEREO_ENHANCE_TEST: Script = preload("uid://c53r8xnm6w2jt")
const DELAY_TEST: Script = preload("uid://b57gm8xq3nf2w")
const REMAINING_EFFECTS_TEST: Script = preload("uid://c47pw9yt2fn8k")
const STREAM_WAVEFORM_TEST: Script = preload("uid://bws4h8t2gr7n5")
const BUS_DOCK_TEST: Script = preload("uid://bm4q1s8vz9d6e")
const DOCUMENTATION_TEST: Script = preload("uid://d170o8ibcsgsb")
const LOCALIZATION_TEST: Script = preload("uid://c6m5gu6m5ogp4")

const EXPECTED: Dictionary = {
	"Show Startup Banner": "显示启动横幅",
	"Dsp Buffer Size": "DSP 缓冲区大小",
	"Init Flags": "初始化标志",
}


func _enter_tree() -> void:
	get_tree().create_timer(180.0).timeout.connect(_on_timeout)
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
	# 离屏编辑器没有系统鼠标进入事件，初始化 GUI 鼠标状态
	get_viewport().notification(Node.NOTIFICATION_VP_MOUSE_ENTER)
	passed = await BUS_DOCK_TEST.new().run_startup(output_dir, tree) and passed
	passed = await PANNER_TEST.new().run(output_dir, tree) and passed
	passed = await EQ_TEST.new().run(output_dir, tree) and passed
	passed = await FILTER_TEST.new().run(output_dir, tree) and passed
	passed = await COMPRESSOR_TEST.new().run(output_dir, tree) and passed
	passed = await AMPLIFY_TEST.new().run(output_dir, tree) and passed
	passed = await CHORUS_TEST.new().run(output_dir, tree) and passed
	passed = await REVERB_TEST.new().run(output_dir, tree) and passed
	passed = await PITCH_SHIFT_TEST.new().run(output_dir, tree) and passed
	passed = await PHASER_TEST.new().run(output_dir, tree) and passed
	passed = await STEREO_ENHANCE_TEST.new().run(output_dir, tree) and passed
	passed = await DELAY_TEST.new().run(output_dir, tree) and passed
	passed = await REMAINING_EFFECTS_TEST.new().run(output_dir, tree) and passed
	passed = await CAPTURE_TEST.new().run(output_dir, tree) and passed
	passed = await STREAM_WAVEFORM_TEST.new().run(output_dir, tree) and passed
	passed = await BUS_DOCK_TEST.new().run(output_dir, tree) and passed
	passed = await DOCUMENTATION_TEST.new().run(output_dir, tree) and passed
	passed = await LOCALIZATION_TEST.new().run(output_dir, tree) and passed
	# 在 FMOD 系统退出前释放测试历史和检查器持有的音频资源
	EditorInterface.edit_resource(Resource.new())
	EditorInterface.get_editor_undo_redo().clear_history()
	await tree.process_frame
	await tree.process_frame
	if passed:
		print("TESTS_PASSED output=", output_dir)
	else:
		push_error("FMOD editor checks failed; see JSON reports")
	tree.quit(0 if passed else 1)


func _on_timeout() -> void:
	push_error("Editor translation test timed out")
	get_tree().quit(1)
