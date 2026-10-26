extends Control

const REVERB_SCENE: PackedScene = preload("uid://bto84iuntkj2l")
const PLAYBACK_SCENE: PackedScene = preload("uid://jxwo0eyc6poh")
const AUTOMATED_SCENE: PackedScene = preload("uid://bq2vsmn68g7k3")

var _test_process: int
var _output_dir: String

@onready var _status: Label = %Status
@onready var _run_button: Button = %RunRegression
@onready var _process_poll: Timer = $ProcessPoll


func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_status.text = "手动场景使用正常音频输出；自动回归在独立离线进程中运行"
	print("MANUAL_TEST_MENU output=", FmodServer.get_main_system().get_output())


func _on_reverb_pressed() -> void:
	_open_scene(REVERB_SCENE)


func _on_playback_pressed() -> void:
	_open_scene(PLAYBACK_SCENE)


func _on_regression_pressed() -> void:
	if _test_process > 0:
		return
	var timestamp: String = Time.get_datetime_string_from_system().replace(":", "-")
	_output_dir = ProjectSettings.globalize_path("res://logs/runs/manual_regression_" + timestamp)
	if DirAccess.make_dir_recursive_absolute(_output_dir) != OK:
		_status.text = "无法创建测试日志目录"
		return
	var arguments: PackedStringArray = [
		"--headless",
		"--path", ProjectSettings.globalize_path("res://"),
		AUTOMATED_SCENE.resource_path,
		"--log-file", _output_dir.path_join("godot.log"),
		"--",
		"--suite=regression",
		"--output-dir=" + _output_dir,
	]
	_test_process = OS.create_process(OS.get_executable_path(), arguments, false)
	if _test_process < 0:
		_test_process = 0
		_status.text = "无法启动测试进程"
		return
	_run_button.disabled = true
	_status.text = "正在运行回归测试...\n" + _output_dir
	_process_poll.start()


func _on_process_poll_timeout() -> void:
	if _test_process <= 0 or OS.is_process_running(_test_process):
		return
	_process_poll.stop()
	_test_process = 0
	_run_button.disabled = false
	var report_path: String = _output_dir.path_join("summary.json")
	if not FileAccess.file_exists(report_path):
		_status.text = "测试进程未生成结果，请查看日志\n" + _output_dir
		return
	var result: Variant = JSON.parse_string(FileAccess.get_file_as_string(report_path))
	if result is Dictionary and bool(result.get("passed", false)):
		_status.text = "回归测试通过\n" + _output_dir
	else:
		_status.text = "回归测试失败，请查看日志\n" + _output_dir


func _on_logs_pressed() -> void:
	var logs_dir: String = ProjectSettings.globalize_path("res://logs")
	DirAccess.make_dir_recursive_absolute(logs_dir)
	OS.shell_open(logs_dir)


func _open_scene(scene: PackedScene) -> void:
	var error: Error = get_tree().change_scene_to_packed(scene)
	if error != OK:
		_status.text = "无法加载场景：%s" % scene.resource_path
