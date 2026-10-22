extends SceneTree

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

var _read_count: int


func _initialize() -> void:
	var system: FmodSystem = FmodServer.get_main_system()
	assert(is_instance_valid(system))
	assert(system.get_version()["version"] == "2.4.0")
	assert(system.get_output() == FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	var bus: FmodChannelGroup = system.get_master_channel_group()
	var format_dsp: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_FADER)
	format_dsp.set_channel_format(2, FmodSystem.FMOD_SPEAKER_MODE_STEREO)
	assert(format_dsp.get_channel_format()["num_channels"] == 2)
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
	custom.set_read_callback(_read_audio)
	bus.add_dsp(-1, custom)
	for iteration: int in range(4):
		system.update()
	assert(_read_count > 0, "Legacy script read callback was not invoked")
	bus.remove_dsp(custom)
	custom.release()
	channel.stop()
	oscillator.release()
	print("FMOD_SDK_SMOKE: FMOD 2.04 SDK, DSP effects and script callback passed")
	quit()


func _read_audio(samples: PackedFloat32Array, _frames: int, _channels: int) -> Dictionary:
	_read_count += 1
	return {"output": samples,}
