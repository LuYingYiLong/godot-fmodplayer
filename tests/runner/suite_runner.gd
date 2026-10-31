extends RefCounted

const CHECKS: Array[Dictionary] = [
	{
		"name": "3D 空间音频与距离滤波",
		"script": preload("uid://bh81rpq0y6z7n"),
	},
	{
		"name": "混音矩阵",
		"script": preload("uid://b5tsw1j6s0dyw"),
	},
	{
		"name": "总线路由与效果器",
		"script": preload("uid://c0y0kdx4qn6l5"),
	},
	{
		"name": "频谱快照",
		"script": preload("uid://wsi7yl1eksc4"),
		"uses_output_dir": false,
	},
	{
		"name": "SoundGroup 播放限制",
		"script": preload("uid://bdgs1no6x4p8m"),
	},
	{
		"name": "FMOD API 与音频资源",
		"script": preload("uid://c0i5v0a75fpfe"),
		"uses_output_dir": false,
	},
	{
		"name": "Capture 缓冲区与声道顺序",
		"script": preload("uid://dhyb1t6prj2bc"),
	},
	{
		"name": "混响输出与尾音",
		"script": preload("uid://cpe7geegh4xgh"),
	},
]

var _output_dir: String
var _results: Array[Dictionary]


func run(output_dir: String) -> Dictionary:
	_output_dir = output_dir
	_results.clear()
	var directory_error: Error = DirAccess.make_dir_recursive_absolute(_output_dir)
	if directory_error != OK:
		return {"passed": false, "summary": "无法创建回归报告目录",}
	_configure_offline_audio(true)
	for index: int in CHECKS.size():
		if index == 1:
			_configure_offline_audio()
		var check: Dictionary = CHECKS[index]
		var test_script: Script = check["script"]
		var test: RefCounted = test_script.new()
		var passed: bool
		if bool(check.get("uses_output_dir", true)):
			passed = test.run(_output_dir)
		else:
			passed = test.run()
		_results.append({"name": check["name"], "passed": passed,})
		if not passed:
			return _finish(false)
	return _finish(true)


func _configure_offline_audio(distance_filter: bool = false) -> void:
	# 自动测试使用独立离线进程，手动场景保持正常音频输出
	var system: FmodSystem = FmodSystem.create_system()
	system.set_output(FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	system.set_software_format(48000, FmodSystem.FMOD_SPEAKER_MODE_STEREO, 0)
	system.set_dsp_buffer_size(1024, 4)
	var flags: int = FmodSystem.FMOD_INIT_FLAG_STREAM_FROM_UPDATE
	if distance_filter:
		flags |= FmodSystem.FMOD_INIT_FLAG_CHANNEL_DISTANCEFILTER
	system.init(32, flags)
	FmodServer.reset_main_system(system)
	print("TEST_AUDIO_CONFIG output=", system.get_output(), " ", system.get_software_format())


func _finish(passed: bool) -> Dictionary:
	var summary: String = "数值回归全部通过" if passed else "数值回归失败，后续项目未运行"
	for check: Dictionary in _results:
		summary += "\n[%s] %s" % ["通过" if check["passed"] else "失败", check["name"],]
	var result: Dictionary = {
		"passed": passed,
		"output_dir": _output_dir,
		"summary": summary,
		"checks": _results,
	}
	var report: FileAccess = FileAccess.open(_output_dir.path_join("summary.json"), FileAccess.WRITE)
	if report == null:
		result["passed"] = false
		result["summary"] = "无法保存回归报告"
	else:
		report.store_string(JSON.stringify(result, "\t"))
	return result
