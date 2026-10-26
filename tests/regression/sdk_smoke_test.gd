extends RefCounted

const AUDIO_FIXTURE_UID: String = "uid://c6kky7072rqpo"

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
	&"FmodAudioEffectRecord",
]

var _process_count: int
var _generator_count: int
var _short_output: bool


func run() -> bool:
	var system: FmodSystem = FmodServer.get_main_system()
	assert(is_instance_valid(system))
	assert(system.get_version()["version"] == "2.4.0")
	assert(system.get_output() == FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	var bus: FmodChannelGroup = system.get_master_channel_group()
	var format_dsp: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_FADER)
	format_dsp.set_input_channel_format(2, FmodSystem.FMOD_SPEAKER_MODE_STEREO)
	assert(format_dsp.get_input_channel_format()["num_channels"] == 2)
	assert(format_dsp.get_output_channel_format(2, FmodSystem.FMOD_SPEAKER_MODE_STEREO)["out_channels"] == 2)
	format_dsp.release()
	var oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	assert(is_instance_valid(oscillator))
	assert(oscillator.get_parameter_info(1).has("mapping"))
	assert(oscillator.get_info().has("mode"))
	assert(system.get_dsp_info_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)["plugin_sdk_version"] == 204)
	oscillator.set_parameter_float(1, 440.0)
	var channel: FmodChannel = system.play_dsp(oscillator, bus)
	assert(is_instance_valid(channel))

	for effect_class: StringName in EFFECT_CLASSES:
		var effect: FmodAudioEffect = ClassDB.instantiate(effect_class) as FmodAudioEffect
		assert(is_instance_valid(effect), "Missing effect: %s" % effect_class)
		effect.apply_to(bus)
		# 预热带有 FFT 窗口或延迟缓冲区的效果器
		for iteration: int in range(24):
			system.update()
		var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
		capture.apply_to(bus)
		for iteration: int in range(8):
			system.update()
		assert(capture.get_frames_available() > 0, "No captured frames for %s" % effect_class)
		var samples: PackedVector2Array = capture.get_buffer(mini(256, capture.get_frames_available()))
		var has_signal: bool
		for sample: Vector2 in samples:
			assert(sample.is_finite(), "Invalid sample from %s" % effect_class)
			has_signal = has_signal or sample.length_squared() > 0.00000001
		assert(has_signal, "Silent output from %s" % effect_class)
		if effect_class == &"FmodAudioEffectReverb":
			channel.set_paused(true)
			capture.clear_buffer()
			for iteration: int in range(4):
				system.update()
			var tail: PackedVector2Array = capture.get_buffer(mini(512, capture.get_frames_available()))
			var has_tail: bool
			for sample: Vector2 in tail:
				has_tail = has_tail or sample.length_squared() > 0.00000001
			assert(has_tail, "Reverb tail stopped when its input became idle")
			channel.set_paused(false)
		capture.remove_from_bus(bus)
		effect.remove_from_bus(bus)
		print("FMOD_SDK_SMOKE: ", effect_class, " passed")

	var custom: FmodDSP = system.create_dsp("SDK smoke callback")
	custom.set_process_callback(_process_audio)
	bus.add_dsp(-1, custom)
	for iteration: int in range(4):
		system.update()
	assert(_process_count > 0, "Script process callback was not invoked")
	bus.remove_dsp(custom)
	custom.release()
	channel.stop()
	oscillator.release()
	if not _test_204_api(system):
		return false
	print("FMOD_SDK_SMOKE: FMOD 2.04 SDK, DSP effects and script callback passed")
	return true


func _process_audio(samples: PackedFloat32Array, _frames: int, _channels: int, _output_channels: int) -> Dictionary:
	_process_count += 1
	return {"output": samples,}


func _test_204_api(system: FmodSystem) -> bool:
	assert(not ClassDB.class_has_method(&"FmodDSP", &"set_read_callback"))
	assert(not ClassDB.class_has_method(&"FmodDSP", &"set_shouldiprocess_callback"))
	assert(not ClassDB.class_has_method(&"FmodDSP", &"set_channel_format"))
	assert(FmodSound.FMOD_SOUND_TYPE_MEDIACODEC == 21)
	assert(FmodSystem.FMOD_OUTPUT_TYPE_AUDIOWORKLETP == 20)
	assert(FmodSystem.FMOD_OUTPUT_TYPE_PHASE == 21)
	var source: FmodDSP = system.create_dsp("204 generator", FmodDSP.DSP_MODE_GENERATOR, 0, 0, 2)
	assert(source.get_info()["mode"] == FmodDSP.DSP_MODE_GENERATOR)
	assert(not source.get_info().has("config_width"))
	source.set_process_callback(_generate_audio)
	source.set_active(true)
	var sink: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_FADER)
	var connection: FmodDSPConnection = system.create_dsp_connection()
	connection.set_mix(-6.0)
	assert(sink.add_input_preallocated(source, connection) == connection)
	assert(is_equal_approx(connection.get_mix(), -6.0))
	assert(sink.get_num_inputs() == 1)
	assert(connection.get_input().get_info()["name"] == "204 generator")
	var bus: FmodChannelGroup = system.create_channel_group("204 generator bus")
	system.get_master_channel_group().add_group(bus)
	bus.add_dsp(-1, sink)
	var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	capture.apply_to(bus)
	for iteration: int in range(4):
		system.update()
	assert(_generator_count > 0, "Generator process callback was not invoked")
	var samples: PackedVector2Array = capture.get_buffer(capture.get_frames_available())
	assert(samples.size() > 0)
	var expected: float = db_to_linear(-6.0) * 0.25
	assert(is_equal_approx(samples[-1].x, expected))
	assert(is_equal_approx(samples[-1].y, -expected))
	_short_output = true
	capture.clear_buffer()
	system.update()
	samples = capture.get_buffer(capture.get_frames_available())
	assert(samples.size() > 1)
	assert(is_equal_approx(samples[0].x, db_to_linear(-6.0) * 0.125))
	assert(samples[-1] == Vector2.ZERO, "Short callback output must be padded with zeros")
	capture.remove_from_bus(bus)
	bus.remove_dsp(sink)
	sink.disconnect_all(true, false)
	sink.release()
	source.release()
	bus.release()
	var description: Dictionary = system.get_dsp_info_by_type(FmodDSP.DSP_TYPE_FADER)
	assert(description.has_all(["mode", "input_channels", "output_channels", "default_cooldown",]))
	assert(not description.has("has_read"))
	var stream: FmodAudioStream = FmodAudioStream.new()
	assert(stream.resampler_method == FmodSystem.FMOD_RESAMPLER_DEFAULT)
	stream.resampler_method = FmodSystem.FMOD_RESAMPLER_SPLINE
	assert(stream.resampler_method == FmodSystem.FMOD_RESAMPLER_SPLINE)
	# 使用已有导入资源的文件数据验证全部低层创建入口
	var resource_path: String = ResourceUID.get_id_path(ResourceUID.text_to_id(AUDIO_FIXTURE_UID))
	var audio_path: String = ProjectSettings.globalize_path(resource_path)
	var data: PackedByteArray = FileAccess.get_file_as_bytes(audio_path)
	assert(not data.is_empty())
	stream.audio_data = data
	assert(stream.preload())
	var cached_sound: FmodSound = stream.get_sound()
	stream.resampler_method = FmodSystem.FMOD_RESAMPLER_CUBIC
	assert(not stream.is_preloaded(), "Changing resampler must invalidate the cached sound")
	assert(stream.preload())
	assert(stream.get_sound() != cached_sound)
	cached_sound = null
	stream.clear()
	var from_memory: FmodSound = system.create_sound_from_memory(data, 0, FmodSystem.FMOD_RESAMPLER_CUBIC)
	assert(is_instance_valid(from_memory))
	assert(from_memory.get_length(FmodSystem.FMOD_TIME_UNIT_MS) > 0)
	from_memory = null
	var from_file: FmodSound = system.create_sound_from_file(audio_path, 0, FmodSystem.FMOD_RESAMPLER_SPLINE)
	assert(is_instance_valid(from_file))
	from_file = null
	var from_res: FmodSound = system.create_sound_from_res(resource_path, 0, FmodSystem.FMOD_RESAMPLER_LINEAR)
	assert(is_instance_valid(from_res))
	from_res = null
	var from_stream: FmodSound = system.create_stream_from_file(audio_path, 0, FmodSystem.FMOD_RESAMPLER_NOINTERP)
	assert(is_instance_valid(from_stream))
	from_stream = null
	print("FMOD_SDK_204: generator, preallocated connection and per-sound resamplers passed")
	return true


func _generate_audio(samples: PackedFloat32Array, frames: int, input_channels: int, output_channels: int) -> Dictionary:
	assert(samples.is_empty() and input_channels == 0)
	assert(output_channels == 2)
	_generator_count += 1
	if _short_output:
		return {"output": PackedFloat32Array([0.125, -0.125,]),}
	var output: PackedFloat32Array
	output.resize(frames * output_channels)
	for frame: int in range(frames):
		output[frame * 2] = 0.25
		output[frame * 2 + 1] = -0.25
	return {"output": output, "cooldown": FmodDSP.DSP_COOLDOWN_INFINITE, "silence": false, "bypass": false,}
