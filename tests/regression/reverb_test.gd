extends RefCounted


func run(output_dir: String) -> bool:
	var system: FmodSystem = FmodServer.get_main_system()
	var bus: FmodChannelGroup = system.get_master_channel_group()
	var checks: Array[Dictionary]
	for case_name: String in ["default", "highpass", "dynamic", "tail",]:
		var effect: FmodAudioEffectReverb = FmodAudioEffectReverb.new()
		if case_name == "highpass":
			effect.set_hpf(0.7)
			effect.set_spread(0.35)
		if case_name == "tail":
			effect.set_dry(0.0)
			effect.set_wet(1.0)
			effect.set_predelay_msec(40.0)
		effect.apply_to(bus)
		var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
		capture.apply_to(bus)
		var oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
		oscillator.set_parameter_float(1, 440.0)
		var channel: FmodChannel = system.play_dsp(oscillator, bus)
		var output: FileAccess = FileAccess.open(output_dir.path_join("reverb_" + case_name + ".f32"), FileAccess.WRITE)
		assert(output != null)
		var peak: float
		var tail_peak: float
		var frame_count: int
		var finite: bool = true
		var early_tail_energy: float
		var late_tail_energy: float
		for block: int in range(128):
			if case_name == "dynamic" and block % 16 == 0:
				var step: float = float(block) / 128.0
				effect.set_spread(step if block % 32 == 0 else 1.0 - step)
				effect.set_damping(step)
				effect.set_room_size(1.0 - step)
				effect.set_hpf(step)
				effect.set_predelay_msec(float((block / 16) % 2) * 500.0)
				effect.set_predelay_feedback(0.98 * step)
				effect.set_dry(1.0 - step)
				effect.set_wet(step)
			if case_name == "tail" and block == 32:
				channel.set_paused(true)
			system.update()
			var samples: PackedVector2Array = capture.get_buffer(capture.get_frames_available())
			var interleaved: PackedFloat32Array
			interleaved.resize(samples.size() * 2)
			for index: int in range(samples.size()):
				var sample: Vector2 = samples[index]
				finite = finite and sample.is_finite()
				peak = maxf(peak, maxf(absf(sample.x), absf(sample.y)))
				if block >= 40:
					tail_peak = maxf(tail_peak, maxf(absf(sample.x), absf(sample.y)))
				if block >= 40 and block < 64:
					early_tail_energy += sample.length_squared()
				elif block >= 104:
					late_tail_energy += sample.length_squared()
				interleaved[index * 2] = sample.x
				interleaved[index * 2 + 1] = sample.y
			frame_count += samples.size()
			output.store_buffer(interleaved.to_byte_array())
		output.close()
		channel.stop()
		oscillator.release()
		capture.remove_from_bus(bus)
		effect.remove_from_bus(bus)
		var passed: bool = finite and frame_count == 128 * 1024 and peak > 0.001
		if case_name == "tail":
			passed = passed and tail_peak > 0.00001 and late_tail_energy < early_tail_energy
		checks.append({"case": case_name, "frames": frame_count, "peak": peak, "tail_peak": tail_peak, "early_tail_energy": early_tail_energy, "late_tail_energy": late_tail_energy, "passed": passed,})
		print("REVERB_RENDER ", JSON.stringify(checks.back()))
		if not passed:
			push_error("Reverb render failed: " + case_name)
			return false
	var report: FileAccess = FileAccess.open(output_dir.path_join("reverb_render.json"), FileAccess.WRITE)
	assert(report != null)
	report.store_string(JSON.stringify(checks, "\t"))
	return true
