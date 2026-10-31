extends Control

const REVERB_SCENE: PackedScene = preload("uid://bto84iuntkj2l")
const PLAYBACK_SCENE: PackedScene = preload("uid://jxwo0eyc6poh")
const AUTOMATED_SCENE: PackedScene = preload("uid://bq2vsmn68g7k3")
const REGRESSION_TIMEOUT_MSEC: int = 120000

static var _last_output_dir: String

var _test_process: int
var _test_started_msec: int
var _output_dir: String

@onready var _status: Label = %Status
@onready var _run_button: Button = %RunRegression
@onready var _report_button: Button = %Logs
@onready var _scene_buttons: Array[Button] = [
	$Center/Options/Reverb,
	$Center/Options/Playback,
]
@onready var _process_poll: Timer = $ProcessPoll


func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_status.text = "选择场景试听，或运行 8 项数值回归\n数值回归不播放声音，完成后在这里显示逐项结果"
	if not _last_output_dir.is_empty() and DirAccess.dir_exists_absolute(_last_output_dir):
		_output_dir = _last_output_dir
		_report_button.disabled = false
		_read_report()


func _exit_tree() -> void:
	if _test_process > 0 and OS.is_process_running(_test_process):
		OS.kill(_test_process)


func _on_reverb_pressed() -> void:
	_open_scene(REVERB_SCENE)


func _on_playback_pressed() -> void:
	_open_scene(PLAYBACK_SCENE)


func _on_regression_pressed() -> void:
	if _test_process > 0:
		return
	var timestamp: String = Time.get_datetime_string_from_system().replace(":", "-")
	_output_dir = ProjectSettings.globalize_path(
		"res://logs/runs/regression_%s_%d" % [timestamp, Time.get_ticks_msec(),]
	)
	if DirAccess.make_dir_recursive_absolute(_output_dir) != OK:
		_status.text = "无法创建测试日志目录"
		return
	var arguments: PackedStringArray = [
		"--headless",
		"--path", ProjectSettings.globalize_path("res://"),
		AUTOMATED_SCENE.resource_path,
		"--log-file", _output_dir.path_join("godot.log"),
		"--",
		"--output-dir=" + _output_dir,
	]
	_test_process = OS.create_process(OS.get_executable_path(), arguments, false)
	if _test_process < 0:
		_test_process = 0
		_status.text = "无法启动测试进程"
		return
	_test_started_msec = Time.get_ticks_msec()
	_set_test_running(true)
	_status.text = "正在运行数值回归，请稍候..."
	_process_poll.start()


func _on_process_poll_timeout() -> void:
	if _test_process <= 0:
		return
	if OS.is_process_running(_test_process):
		if Time.get_ticks_msec() - _test_started_msec < REGRESSION_TIMEOUT_MSEC:
			return
		OS.kill(_test_process)
		_status.text = "数值回归超时，请点击「查看回归报告」检查 godot.log"
		_finish_test()
		return
	_finish_test()
	_read_report()


func _on_logs_pressed() -> void:
	if not _output_dir.is_empty():
		OS.shell_open(_output_dir)


func _read_report() -> void:
	var report_path: String = _output_dir.path_join("summary.json")
	if not FileAccess.file_exists(report_path):
		_status.text = "数值回归未完成，请点击「查看回归报告」检查 godot.log"
		return
	var result: Variant = JSON.parse_string(FileAccess.get_file_as_string(report_path))
	if result is Dictionary:
		_status.text = String(result.get("summary", "回归报告缺少结果"))
	else:
		_status.text = "回归报告无法读取，请点击「查看回归报告」检查 godot.log"


func _set_test_running(running: bool) -> void:
	_run_button.disabled = running
	_report_button.disabled = running
	for button: Button in _scene_buttons:
		button.disabled = running


func _finish_test() -> void:
	_process_poll.stop()
	_test_process = 0
	_last_output_dir = _output_dir
	_set_test_running(false)


func _open_scene(scene: PackedScene) -> void:
	var error: Error = get_tree().change_scene_to_packed(scene)
	if error != OK:
		_status.text = "无法加载场景：%s" % scene.resource_path
