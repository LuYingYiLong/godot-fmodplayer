extends RefCounted

var _failures: Array[String] = []


func run() -> bool:
	_failures.clear()
	var system: FmodSystem = FmodServer.get_main_system()
	var master: FmodChannelGroup = system.get_master_channel_group()
	var initial_dsp_count: int = master.get_num_dsps()
	var process_frame: int = Engine.get_process_frames()
	var analyzer: FmodAudioEffectSpectrumAnalyzer = FmodAudioEffectSpectrumAnalyzer.new()
	analyzer.immediate_mode = true
	_expect(analyzer.get_bin_count() == 0, "Unattached analyzer should have no bins")
	_expect(analyzer.get_rms() == 0.0, "Unattached analyzer should have zero RMS")
	var oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	oscillator.set_parameter_float(1, 440.0)
	var channel: FmodChannel = system.play_dsp(oscillator, master)
	analyzer.apply_to(master)
	_mix(system)
	var original_bins: int = analyzer.get_bin_count()
	var low: Vector2 = analyzer.get_magnitude_for_frequency_range(300.0, 600.0)
	var high: Vector2 = analyzer.get_magnitude_for_frequency_range(3500.0, 4500.0)
	_expect(original_bins > 0, "FFT should provide bins")
	_expect(low.x > high.x * 10.0 and low.x > 0.001, "440 Hz should dominate the low band")
	var rms: float = analyzer.get_rms()
	var centroid: float = analyzer.get_centroid()
	for iteration: int in range(64):
		_expect(analyzer.get_magnitude_for_frequency_range(300.0, 600.0).is_equal_approx(low), "Repeated queries should share a snapshot")
		_expect(is_equal_approx(analyzer.get_rms(), rms), "RMS should share the snapshot")
		_expect(is_equal_approx(analyzer.get_centroid(), centroid), "Centroid should share the snapshot")
	var average: Vector2 = analyzer.get_magnitude_for_frequency_range(300.0, 600.0, FmodAudioEffectSpectrumAnalyzer.MAGNITUDE_AVERAGE)
	_expect(average.x > 0.0 and average.x <= low.x, "Average magnitude should not exceed the peak")
	_expect(analyzer.get_magnitude_for_frequency_range(600.0, 300.0).is_equal_approx(low), "Reversed ranges should be equivalent")
	_expect(analyzer.get_magnitude_for_frequency_range(440.0, 440.0) == Vector2.ZERO, "Empty ranges should be zero")
	analyzer.update_spectrum()
	_expect(analyzer.get_magnitude_for_frequency_range(300.0, 600.0).is_equal_approx(low), "Explicit refresh should preserve unchanged data")
	oscillator.set_parameter_float(1, 4000.0)
	_mix(system)
	low = analyzer.get_magnitude_for_frequency_range(300.0, 600.0)
	high = analyzer.get_magnitude_for_frequency_range(3500.0, 4500.0)
	_expect(high.x > low.x * 10.0 and high.x > 0.001, "Audio-clock advancement should refresh within one Godot frame")
	_expect(Engine.get_process_frames() == process_frame, "Test must exercise multiple mixes within a single Godot frame")
	analyzer.set_fft_size(FmodAudioEffectSpectrumAnalyzer.FFT_SIZE_4096)
	_mix(system)
	_expect(analyzer.get_bin_count() > original_bins, "FFT-size changes should invalidate cached bins")
	analyzer.set_fft_size(FmodAudioEffectSpectrumAnalyzer.FFT_SIZE_1024)
	_mix(system)
	_expect(analyzer.get_bin_count() == original_bins, "Restoring FFT size should restore bin count")
	analyzer.remove_from_bus(master)
	_expect(analyzer.get_bin_count() == 0 and analyzer.get_rms() == 0.0, "Removal should clear the snapshot")
	analyzer.apply_to(master)
	_mix(system)
	_expect(analyzer.get_bin_count() == original_bins, "Reattachment should refresh the snapshot")
	var second_bus: FmodChannelGroup = system.create_channel_group("Snapshot rebind test")
	master.add_group(second_bus)
	var second_oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	second_oscillator.set_parameter_float(1, 4000.0)
	var second_channel: FmodChannel = system.play_dsp(second_oscillator, second_bus)
	analyzer.apply_to(second_bus)
	_mix(system)
	_expect(master.get_num_dsps() == initial_dsp_count, "Rebinding must remove the previous bus DSP")
	_expect(analyzer.get_magnitude_for_frequency_range(3500.0, 4500.0).x > 0.001, "Rebound analyzer should read its new bus")
	analyzer.remove_from_bus(second_bus)
	second_channel.stop()
	second_oscillator.release()
	second_bus.release()
	channel.stop()
	oscillator.release()
	var passed: bool = _failures.is_empty()
	print("SPECTRUM_SNAPSHOT_TEST: ", "passed" if passed else "failed", " failures=", _failures.size())
	return passed


func _mix(system: FmodSystem) -> void:
	# 让振荡器的频率平滑过渡完成，避免把过渡频谱误判为缓存过期
	for iteration: int in range(64):
		system.update()


func _expect(condition: bool, message: String) -> void:
	if not condition:
		_failures.append(message)
		push_error(message)
