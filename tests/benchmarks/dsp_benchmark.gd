extends RefCounted

# 测试项目启用 NOSOUND_NRT、STREAM_FROM_UPDATE、48 kHz 和立体声
# 每项效果器独立运行，计时包含基础混音和脚本开销，不代表实际游戏的热点占比
# 由自动测试入口运行，结果保存在 tests/logs/runs 的本次运行目录

const EFFECT_CLASSES: Array[StringName] = [
	&"FmodAudioEffectAmplify",
	&"FmodAudioEffectFilter",
	&"FmodAudioEffectEQ6",
	&"FmodAudioEffectEQ10",
	&"FmodAudioEffectEQ21",
	&"FmodAudioEffectChorus",
	&"FmodAudioEffectDelay",
	&"FmodAudioEffectDistortion",
	&"FmodAudioEffectPhaser",
	&"FmodAudioEffectPitchShift",
	&"FmodAudioEffectReverb",
	&"FmodAudioEffectCompressor",
	&"FmodAudioEffectHardLimiter",
	&"FmodAudioEffectStereoEnhance",
	&"FmodAudioEffectPanner",
	&"FmodAudioEffectSpectrumAnalyzer",
	&"FmodAudioEffectCapture",
]

var _system: FmodSystem
var _bus: FmodChannelGroup
var _results: Array[Dictionary]
var _trial_usec: int = 200000
var _output_path: String
var _checksum: float
var _callback_count: int


func run(output_path: String, quick: bool = false) -> Array[Dictionary]:
	_output_path = output_path
	_trial_usec = 10000 if quick else 200000
	_results.clear()
	_system = FmodServer.get_main_system()
	assert(_system.get_output() == FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	_bus = _system.get_master_channel_group()
	var oscillator: FmodDSP = _system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	oscillator.set_parameter_float(1, 440.0)
	var channel: FmodChannel = _system.play_dsp(oscillator, _bus)
	print("PROFILE_CONFIG ", _system.get_software_format(), " ", _system.get_dsp_buffer_size())
	_measure("Baseline", null)
	for effect_class: StringName in EFFECT_CLASSES:
		var effect: FmodAudioEffect = ClassDB.instantiate(effect_class) as FmodAudioEffect
		_measure(String(effect_class), effect)
	_measure("Spectrum64Queries", FmodAudioEffectSpectrumAnalyzer.new(), 64)
	_measure("Spectrum256Queries", FmodAudioEffectSpectrumAnalyzer.new(), 256)
	_measure("CaptureConsumed", FmodAudioEffectCapture.new(), -1)
	_measure("RecordActive", FmodAudioEffectRecord.new())
	var script_dsp: FmodDSP = _system.create_dsp("Profile script passthrough")
	script_dsp.set_process_callback(_read_audio)
	_bus.add_dsp(-1, script_dsp)
	_measure("ScriptPassthrough", null)
	_bus.remove_dsp(script_dsp)
	script_dsp.release()
	channel.stop()
	oscillator.release()
	print("PROFILE_FINISHED checksum=", _checksum, " callbacks=", _callback_count)
	return _results


func run_capture(output_path: String, quick: bool = false) -> Array[Dictionary]:
	_output_path = output_path
	_trial_usec = 10000 if quick else 200000
	_results.clear()
	_system = FmodServer.get_main_system()
	assert(_system.get_output() == FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	_bus = _system.get_master_channel_group()
	var oscillator: FmodDSP = _system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	oscillator.set_parameter_float(1, 440.0)
	var channel: FmodChannel = _system.play_dsp(oscillator, _bus)
	_measure("Baseline", null)
	var full: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	full.buffer_length = 0.1
	_measure("CaptureOverflow", full)
	_measure("CaptureWriteOnly", FmodAudioEffectCapture.new(), -3)
	_measure("CaptureReadEveryBlock", FmodAudioEffectCapture.new(), -1)
	_measure("CaptureReadEvery8Blocks", FmodAudioEffectCapture.new(), -2)
	channel.stop()
	oscillator.release()
	return _results


func run_reverb(output_path: String, quick: bool = false) -> Array[Dictionary]:
	_output_path = output_path
	_trial_usec = 10000 if quick else 300000
	_results.clear()
	_system = FmodServer.get_main_system()
	assert(_system.get_output() == FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	_bus = _system.get_master_channel_group()
	var oscillator: FmodDSP = _system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	oscillator.set_parameter_float(1, 440.0)
	var channel: FmodChannel = _system.play_dsp(oscillator, _bus)
	_measure("Baseline", null)
	_measure("ReverbDefault", FmodAudioEffectReverb.new())
	var highpass: FmodAudioEffectReverb = FmodAudioEffectReverb.new()
	highpass.set_hpf(0.7)
	highpass.set_spread(0.35)
	_measure("ReverbHighpass", highpass)
	var room: FmodAudioEffectReverb = FmodAudioEffectReverb.new()
	room.set_room_size(1.0)
	room.set_damping(0.0)
	room.set_spread(0.0)
	room.set_predelay_msec(0.0)
	_measure("ReverbWideRoom", room)
	channel.stop()
	oscillator.release()
	return _results


func _measure(label: String, effect: FmodAudioEffect, queries: int = 0) -> void:
	print("PROFILE_STAGE ", label)
	if effect != null:
		effect.apply_to(_bus)
	for iteration: int in range(64):
		_system.update()
	var analyzer: FmodAudioEffectSpectrumAnalyzer = effect as FmodAudioEffectSpectrumAnalyzer
	var capture: FmodAudioEffectCapture = effect as FmodAudioEffectCapture
	var record: FmodAudioEffectRecord = effect as FmodAudioEffectRecord
	var frequency_bounds: PackedFloat64Array
	for band: int in range(queries + 1):
		frequency_bounds.append(20.0 * pow(1000.0, float(band) / float(maxi(queries, 1))))
	var times: Array[float]
	var frame_counts: Array[int]
	for trial: int in range(5):
		if record != null:
			record.set_recording_active(true)
		if capture != null:
			capture.clear_buffer()
		var clock_before: int = _bus.get_dsp_clock()["dsp_lock"]
		var updates: int
		var elapsed: int
		var start: int = Time.get_ticks_usec()
		while elapsed < _trial_usec and updates < 4096:
			for batch: int in range(32):
				if queries == -3:
					capture.clear_buffer()
				_system.update()
				if queries > 0:
					for band: int in range(queries):
						_checksum += analyzer.get_magnitude_for_frequency_range(frequency_bounds[band], frequency_bounds[band + 1]).x
				elif queries == -1 or (queries == -2 and batch % 8 == 7):
					var frames: PackedVector2Array = capture.get_buffer(capture.get_frames_available())
					if not frames.is_empty():
						_checksum += frames[0].x
			updates += 32
			elapsed = Time.get_ticks_usec() - start
		var clock_after: int = _bus.get_dsp_clock()["dsp_lock"]
		times.append(float(elapsed) / float(updates))
		frame_counts.append((clock_after - clock_before) / updates)
		if record != null:
			record.set_recording_active(false)
	times.sort()
	var sample_rate: int = _system.get_software_format()["sample_rate"]
	var frames_per_update: int = frame_counts[2]
	var result: Dictionary = {
		"name": label,
		"median_us_per_update": times[2],
		"trial_us_per_update": times,
		"min_us_per_update": times[0],
		"max_us_per_update": times[4],
		"frames_per_update": frames_per_update,
		"sample_rate": sample_rate,
		"one_core_realtime_percent": times[2] * float(sample_rate) / float(maxi(frames_per_update, 1)) / 10000.0,
	}
	_results.append(result)
	print("PROFILE_RESULT ", JSON.stringify(result))
	if not _output_path.is_empty():
		var output: FileAccess = FileAccess.open(_output_path, FileAccess.WRITE)
		assert(output != null)
		output.store_string(JSON.stringify(_results, "\t"))
	if effect != null:
		effect.remove_from_bus(_bus)


func _read_audio(samples: PackedFloat32Array, _frames: int, _channels: int, _output_channels: int) -> Dictionary:
	_callback_count += 1
	return {"output": samples,}
