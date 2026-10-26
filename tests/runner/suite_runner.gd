extends RefCounted

const BENCHMARK: Script = preload("uid://dhqnw0413pkvm")
const SPECTRUM_TEST: Script = preload("uid://wsi7yl1eksc4")
const SDK_SMOKE_TEST: Script = preload("uid://c0i5v0a75fpfe")
const CAPTURE_TEST: Script = preload("uid://dhyb1t6prj2bc")
const REVERB_TEST: Script = preload("uid://cpe7geegh4xgh")
const SUITES: Array[String] = [
	"all",
	"regression",
	"smoke",
	"spectrum",
	"profile",
	"reverb",
	"capture",
]

var _suite: String = "all"
var _output_dir: String
var _quick: bool
var _capture_baseline: bool


func run(arguments: PackedStringArray) -> Dictionary:
	for argument: String in arguments:
		if argument.begins_with("--suite="):
			_suite = argument.trim_prefix("--suite=")
		elif argument.begins_with("--output-dir="):
			_output_dir = argument.trim_prefix("--output-dir=")
		elif argument == "--quick":
			_quick = true
		elif argument == "--capture-baseline":
			_capture_baseline = true
	if _output_dir.is_empty():
		var timestamp: String = Time.get_datetime_string_from_system().replace(":", "-")
		_output_dir = ProjectSettings.globalize_path("res://logs/runs/run_" + timestamp)
	var directory_error: Error = DirAccess.make_dir_recursive_absolute(_output_dir)
	if directory_error != OK:
		return _finish(false, "Cannot create test output directory: " + _output_dir)
	if _suite not in SUITES:
		return _finish(false, "Unknown test suite: " + _suite)
	if _capture_baseline and _suite != "capture":
		return _finish(false, "Capture baseline mode requires the capture suite")
	_configure_offline_audio()
	var summary: String = "FMOD tests\n\n"
	if _suite in ["all", "regression", "spectrum",]:
		if not SPECTRUM_TEST.new().run():
			return _finish(false, "Spectrum snapshot regression failed")
		summary += "Spectrum snapshot regression: passed\n"
	if _suite in ["all", "regression", "smoke",]:
		if not SDK_SMOKE_TEST.new().run():
			return _finish(false, "FMOD SDK smoke test failed")
		summary += "FMOD SDK smoke test: passed\n"
	if _suite in ["all", "regression", "capture",]:
		if not CAPTURE_TEST.new().run(_output_dir, _capture_baseline):
			return _finish(false, "Capture regression failed")
		if _capture_baseline:
			summary += "Capture regression: baseline diagnostics completed\n"
		else:
			summary += "Capture regression: passed\n"
		if _suite == "capture":
			var capture_results: Array[Dictionary] = BENCHMARK.new().run_capture(
				_output_dir.path_join("capture_timings.json"), _quick
			)
			summary += _timing_summary(capture_results)
	if _suite in ["all", "regression", "reverb",]:
		if not REVERB_TEST.new().run(_output_dir):
			return _finish(false, "Reverb output/tail regression failed")
		summary += "Reverb output/tail regression: passed\n"
		if _suite == "reverb":
			var reverb_results: Array[Dictionary] = BENCHMARK.new().run_reverb(
				_output_dir.path_join("reverb_timings.json"), _quick
			)
			summary += _timing_summary(reverb_results)
	if _suite in ["all", "profile",]:
		var results: Array[Dictionary] = BENCHMARK.new().run(
			_output_dir.path_join("timings.json"), _quick
		)
		summary += _timing_summary(results)
	return _finish(true, summary + "\nResults: " + _output_dir)


func _configure_offline_audio() -> void:
	# 自动测试使用独立离线进程，手动场景保持正常音频输出
	var system: FmodSystem = FmodSystem.create_system()
	system.set_output(FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	system.set_software_format(48000, FmodSystem.FMOD_SPEAKER_MODE_STEREO, 0)
	system.set_dsp_buffer_size(1024, 4)
	system.init(32, FmodSystem.FMOD_INIT_FLAG_STREAM_FROM_UPDATE)
	FmodServer.reset_main_system(system)
	print("TEST_AUDIO_CONFIG output=", system.get_output(), " ", system.get_software_format())


func _timing_summary(results: Array[Dictionary]) -> String:
	var summary: String
	for result: Dictionary in results:
		summary += "%s: %.3f us/block\n" % [result["name"], result["median_us_per_update"]]
	return summary


func _finish(passed: bool, summary: String) -> Dictionary:
	var result: Dictionary = {
		"suite": _suite,
		"passed": passed,
		"quick": _quick,
		"output_dir": _output_dir,
		"summary": summary,
	}
	if DirAccess.dir_exists_absolute(_output_dir):
		var report: FileAccess = FileAccess.open(_output_dir.path_join("summary.json"), FileAccess.WRITE)
		if report == null:
			result["passed"] = false
			result["summary"] = "Cannot write test summary: " + _output_dir
		else:
			report.store_string(JSON.stringify(result, "\t"))
	return result
