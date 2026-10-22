extends RefCounted

var _system: FmodSystem
var _bus: FmodChannelGroup
var _checks: Array[Dictionary] = []
var _baseline: bool
var _failed: bool
var _source_frame: int
var _last_start: int
var _source_channels: int
var _expected_channels: int = 2


func run(output_dir: String, baseline: bool = false) -> bool:
	_baseline = baseline
	_system = FmodServer.get_main_system()
	_bus = _system.get_master_channel_group()
	var initial_dsps: int = _bus.get_num_dsps()
	var source: FmodDSP = _system.create_dsp("Capture stereo fixture")
	source.set_channel_format(2, FmodSystem.FMOD_SPEAKER_MODE_STEREO)
	source.set_read_callback(_generate_stereo)
	_bus.add_dsp(-1, source)
	var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	capture.buffer_length = 0.1
	capture.apply_to(_bus)
	var audio: FileAccess = FileAccess.open(output_dir.path_join("capture_steady.f32"), FileAccess.WRITE)
	assert(audio != null)
	for block: int in range(24):
		_mix()
		_expect(capture.can_get_buffer(1024) and not capture.can_get_buffer(1025), "Available frame query")
		var offset: int
		for count: int in [1, 17, 503, 503,]:
			var samples: PackedVector2Array = capture.get_buffer(count)
			_expect(samples.size() == count, "Partial read frame count")
			_check_samples(samples, _last_start + offset, "Steady/wrapped stereo order")
			var interleaved: PackedFloat32Array
			for sample: Vector2 in samples:
				interleaved.append(sample.x)
				interleaved.append(sample.y)
			audio.store_buffer(interleaved.to_byte_array())
			offset += count
		_expect(capture.get_frames_available() == 0, "Partial reads drain the block")
	audio.close()
	capture.clear_buffer()
	var first_start: int = _source_frame
	for block: int in range(8):
		_mix()
	var available: int = capture.get_frames_available()
	_expect(available == 4800, "Full buffer stores 4800 complete frames", true)
	_expect(capture.get_pushed_frames() == 8192, "Pushed frame counter")
	_expect(capture.get_discarded_frames() == 8192 - available, "Overflow counter balances complete frames", true)
	_check_samples(capture.get_buffer(available), first_start, "Overflow preserves oldest frames")
	for block: int in range(8):
		_mix()
		var samples: PackedVector2Array = capture.get_buffer(capture.get_frames_available())
		_expect(samples.size() == 1024, "Frame count after overflow")
		_check_samples(samples, _last_start, "Stereo alignment after overflow", true)
	capture.clear_buffer()
	_expect(capture.get_frames_available() == 0 and capture.get_pushed_frames() == 0 and capture.get_discarded_frames() == 0, "Clear resets queue and counters")
	_expect(capture.can_get_buffer(0) and capture.get_buffer(0).is_empty(), "Zero frame request")
	if not _baseline:
		_expect(not capture.can_get_buffer(-1) and capture.get_buffer(-1).is_empty(), "Negative frame request")
	_mix()
	_check_samples(capture.get_buffer(1000000), _last_start, "Oversized request clamps to available frames")
	capture.buffer_length = 0.0107
	_expect(capture.get_frames_available() == 0, "Resize clears queued data")
	capture.clear_buffer()
	for block: int in range(4):
		_mix()
		_expect(capture.get_frames_available() == 513, "Odd frame capacity")
		_check_samples(capture.get_buffer(513), _last_start, "Odd capacity overflow order")
	_expect(capture.get_discarded_frames() == 4 * (1024 - 513), "Repeated overflow counter")
	capture.remove_from_bus(_bus)
	capture.clear_buffer()
	capture.buffer_length = 0.1
	capture.apply_to(_bus)
	_mix()
	_check_samples(capture.get_buffer(1024), _last_start, "Reattached capture")
	_expected_channels = 1
	source.set_channel_format(1, FmodSystem.FMOD_SPEAKER_MODE_MONO)
	capture.clear_buffer()
	_mix()
	_check_samples(capture.get_buffer(1024), _last_start, "Mono input duplicates to stereo")
	_expected_channels = 4
	source.set_channel_format(4, FmodSystem.FMOD_SPEAKER_MODE_QUAD)
	capture.clear_buffer()
	_mix()
	_check_samples(capture.get_buffer(1024), _last_start, "Multichannel input keeps its first two channels")
	capture = null
	_expect(_bus.get_num_dsps() == initial_dsps + 1, "Dropping the resource detaches its DSP")
	_bus.remove_dsp(source)
	source.release()
	_expect(_bus.get_num_dsps() == initial_dsps, "Capture leaves no DSP nodes attached")
	var report: FileAccess = FileAccess.open(output_dir.path_join("capture_regression.json"), FileAccess.WRITE)
	assert(report != null)
	report.store_string(JSON.stringify({"baseline_mode": _baseline, "accepted": not _failed, "checks": _checks,}, "\t"))
	report.close()
	print("CAPTURE_REGRESSION accepted=", not _failed, " baseline=", _baseline)
	return not _failed


func _mix() -> void:
	_system.update()
	_expect(_source_frame - _last_start == 1024, "Fixture produces one 1024-frame block")
	_expect(_source_channels == _expected_channels, "Fixture channel format")


func _generate_stereo(samples: PackedFloat32Array, frames: int, channels: int) -> Dictionary:
	_last_start = _source_frame
	_source_channels = channels
	for frame: int in range(frames):
		var left: float = float((_source_frame + frame) % 4096) / 4096.0 - 0.5
		for channel: int in range(channels):
			if channel == 0:
				samples[frame * channels + channel] = left
			elif channel == 1:
				samples[frame * channels + channel] = 0.25 - left * 0.5
			else:
				samples[frame * channels + channel] = -0.125 * float(channel)
	_source_frame += frames
	return {"output": samples,}


func _check_samples(samples: PackedVector2Array, start: int, label: String, known_overflow: bool = false) -> void:
	var matched: bool = not samples.is_empty()
	for index: int in range(samples.size()):
		var left: float = float((start + index) % 4096) / 4096.0 - 0.5
		var right: float = left if _expected_channels == 1 else 0.25 - left * 0.5
		matched = matched and samples[index] == Vector2(left, right)
	_expect(matched, label, known_overflow)


func _expect(condition: bool, label: String, known_overflow: bool = false) -> void:
	var expected_failure: bool = not condition and _baseline and known_overflow
	_checks.append({"name": label, "passed": condition, "expected_baseline_failure": expected_failure,})
	if not condition:
		print("CAPTURE_CHECK_FAILED ", label, " expected_baseline=", expected_failure)
		_failed = _failed or not expected_failure
