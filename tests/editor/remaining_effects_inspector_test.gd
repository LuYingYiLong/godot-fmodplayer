@tool
extends RefCounted

const EXPECTED_PREVIEWS: Dictionary[String, String] = {
	"FmodAudioEffectAmplify": "FmodAudioEffectAmplifyPreview",
	"FmodAudioEffectCapture": "FmodAudioEffectCapturePreview",
	"FmodAudioEffectChorus": "FmodAudioEffectChorusPreview",
	"FmodAudioEffectCompressor": "FmodAudioEffectCompressorPreview",
	"FmodAudioEffectDelay": "FmodAudioEffectDelayPreview",
	"FmodAudioEffectDistortion": "FmodAudioEffectDistortionPreview",
	"FmodAudioEffectEQ": "FmodAudioEffectEQPreview",
	"FmodAudioEffectEQ6": "FmodAudioEffectEQPreview",
	"FmodAudioEffectEQ10": "FmodAudioEffectEQPreview",
	"FmodAudioEffectEQ21": "FmodAudioEffectEQPreview",
	"FmodAudioEffectFilter": "FmodAudioEffectFilterPreview",
	"FmodAudioEffectHardLimiter": "FmodAudioEffectHardLimiterPreview",
	"FmodAudioEffectPanner": "FmodAudioEffectPannerPreview",
	"FmodAudioEffectPhaser": "FmodAudioEffectPhaserPreview",
	"FmodAudioEffectPitchShift": "FmodAudioEffectPitchShiftPreview",
	"FmodAudioEffectReverb": "FmodAudioEffectReverbPreview",
	"FmodAudioEffectSpectrumAnalyzer": "FmodAudioEffectSpectrumAnalyzerPreview",
	"FmodAudioEffectStereoEnhance": "FmodAudioEffectStereoEnhancePreview",
}

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int
var _fixture_level: float = 0.1


func run(output_dir: String, tree: SceneTree) -> bool:
	await _test_distortion(output_dir, tree)
	await _test_limiter(output_dir, tree)
	await _test_coverage(tree)
	_test_limiter_audio()
	await _test_spectrum(output_dir, tree)
	var report: FileAccess = FileAccess.open(output_dir.path_join("remaining_effects_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_distortion(output_dir: String, tree: SceneTree) -> void:
	var effect: FmodAudioEffectDistortion = FmodAudioEffectDistortion.new()
	effect.mode = FmodAudioEffectDistortion.MODE_ATAN
	effect.drive = 0.5
	effect.pre_gain = 6.0
	effect.post_gain = 12.0
	var preview: EditorProperty = await _inspect(effect, "FmodAudioEffectDistortionPreview", tree)
	if preview == null:
		return
	_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty(), "Distortion redraws on demand")
	await _capture(preview, output_dir.path_join("distortion.png"), tree)
	_changes = 0
	effect.changed.connect(_changed)
	for iteration: int in range(2):
		effect.mode = FmodAudioEffectDistortion.MODE_WAVESHAPE
		effect.pre_gain = 9.0
		effect.drive = 0.6
		effect.post_gain = -3.0
		effect.keep_hf_hz = 8000.0
		effect.oversample = 8
		_expect(_changes == 6, "Distortion parameters notify only on change, pass %d" % iteration)
	var lofi: Image
	for mode: int in range(6):
		effect.mode = mode as FmodAudioEffectDistortion.Mode
		effect.drive = 0.75
		effect.pre_gain = 6.0
		effect.post_gain = 0.0
		var image: Image = await _capture(preview, output_dir.path_join("distortion_mode_%d.png" % mode), tree)
		if mode == 2:
			lofi = image
		if mode == 5 and lofi != null and image != null:
			_expect(lofi.get_data() == image.get_data(), "LoFi and BitCrush previews match their shared runtime formula")
	effect.mode = FmodAudioEffectDistortion.MODE_ATAN
	effect.drive = 0.25
	await _settle(tree)
	await _exercise_drag(preview, effect, &"drive", _distortion_center(preview), 0.75, 1.0, 0.0, 1.0, tree)
	effect.pre_gain = 0.0
	await _settle(tree)
	await _exercise_drag(preview, effect, &"pre_gain", _distortion_center(preview), 12.0, 120.0, -60.0, 60.0, tree, true)
	effect.post_gain = 0.0
	await _settle(tree)
	await _exercise_drag(preview, effect, &"post_gain", _distortion_center(preview), -12.0, 104.0, -80.0, 24.0, tree, false, true)
	effect.mode = FmodAudioEffectDistortion.MODE_OVERDRIVE
	effect.pre_gain = 0.0
	await _settle(tree)
	await _exercise_drag(preview, effect, &"pre_gain", _distortion_center(preview), 6.0, 120.0, -60.0, 60.0, tree)
	effect.mode = FmodAudioEffectDistortion.MODE_WAVESHAPE
	effect.drive = 1.0
	effect.pre_gain = 60.0
	effect.post_gain = 24.0
	await _capture(preview, output_dir.path_join("distortion_max.png"), tree)
	effect.post_gain = -80.0
	await _capture(preview, output_dir.path_join("distortion_silent.png"), tree)
	var path: String = output_dir.path_join("distortion_preview.tres")
	_expect(ResourceSaver.save(effect, path) == OK, "Distortion resource saves")
	var restored: FmodAudioEffectDistortion = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectDistortion
	_expect(restored != null and restored.mode == effect.mode and restored.drive == 1.0 and restored.pre_gain == 60.0 and restored.post_gain == -80.0, "Distortion restores saved parameters")
	await _check_disconnect(effect, tree)


func _test_limiter(output_dir: String, tree: SceneTree) -> void:
	var effect: FmodAudioEffectHardLimiter = FmodAudioEffectHardLimiter.new()
	effect.pre_gain_db = 6.0
	effect.ceiling_db = -6.0
	var preview: EditorProperty = await _inspect(effect, "FmodAudioEffectHardLimiterPreview", tree)
	if preview == null:
		return
	_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty(), "HardLimiter redraws on demand")
	await _capture(preview, output_dir.path_join("hard_limiter.png"), tree)
	_changes = 0
	effect.changed.connect(_changed)
	for iteration: int in range(2):
		effect.pre_gain_db = 9.0
		effect.ceiling_db = -12.0
		effect.release = 0.2
		_expect(_changes == 3, "Limiter parameters notify only on change, pass %d" % iteration)
	effect.pre_gain_db = 6.0
	effect.ceiling_db = -6.0
	await _settle(tree)
	await _exercise_drag(preview, effect, &"pre_gain_db", _limiter_point(preview, -56.0, -50.0), 12.0, 86.0, -24.0, 24.0, tree)
	await _exercise_drag(preview, effect, &"ceiling_db", _limiter_point(preview, 0.0, -6.0), -12.0, 86.0, -24.0, 0.0, tree)
	for pre_gain: float in [-24.0, 24.0,]:
		for ceiling: float in [-24.0, 0.0,]:
			effect.pre_gain_db = pre_gain
			effect.ceiling_db = ceiling
			await _capture(preview, output_dir.path_join("hard_limiter_%s_%s.png" % [pre_gain, ceiling,]), tree)
	var path: String = output_dir.path_join("hard_limiter_preview.tres")
	_expect(ResourceSaver.save(effect, path) == OK, "Limiter resource saves")
	var restored: FmodAudioEffectHardLimiter = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectHardLimiter
	_expect(restored != null and restored.pre_gain_db == 24.0 and restored.ceiling_db == 0.0 and is_equal_approx(restored.release, 0.2), "Limiter restores saved parameters")
	await _check_disconnect(effect, tree)


func _test_coverage(tree: SceneTree) -> void:
	var registered_count: int
	for effect_class: StringName in ClassDB.get_class_list():
		if ClassDB.is_parent_class(effect_class, &"FmodAudioEffect") and ClassDB.can_instantiate(effect_class):
			registered_count += 1
			_expect(effect_class == &"FmodAudioEffectRecord" or EXPECTED_PREVIEWS.has(String(effect_class)), "%s is covered or explicitly excluded" % effect_class)
	_expect(registered_count == EXPECTED_PREVIEWS.size() + 1, "Coverage catalog matches every registered concrete effect")
	for effect_class: String in EXPECTED_PREVIEWS:
		var effect: FmodAudioEffect = ClassDB.instantiate(effect_class) as FmodAudioEffect
		await _inspect(effect, EXPECTED_PREVIEWS[effect_class], tree)
	EditorInterface.edit_resource(FmodAudioEffectRecord.new())
	await _settle(tree)
	var custom_previews: int
	for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
		if node.get_class().begins_with("FmodAudioEffect") and node.get_class().ends_with("Preview"):
			custom_previews += 1
	_expect(custom_previews == 0, "Record retains its native inspector as requested")


func _test_limiter_audio() -> void:
	var system: FmodSystem = FmodSystem.create_system()
	system.set_output(FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	system.set_software_format(48000, FmodSystem.FMOD_SPEAKER_MODE_STEREO, 0)
	system.set_dsp_buffer_size(1024, 4)
	system.init(32, FmodSystem.FMOD_INIT_FLAG_STREAM_FROM_UPDATE)
	FmodServer.reset_main_system(system)
	var bus: FmodChannelGroup = system.get_master_channel_group()
	var initial_dsps: int = bus.get_num_dsps()
	var source: FmodDSP = system.create_dsp("Limiter preview fixture")
	source.set_input_channel_format(2, FmodSystem.FMOD_SPEAKER_MODE_STEREO)
	source.set_process_callback(_generate_constant)
	bus.add_dsp(-1, source)
	var limiter: FmodAudioEffectHardLimiter = FmodAudioEffectHardLimiter.new()
	limiter.apply_to(bus)
	var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	capture.buffer_length = 0.2
	capture.apply_to(bus)
	for parameters: Vector3 in [Vector3(0.1, 6.0, -12.0), Vector3(1.0, 6.0, -12.0), Vector3(0.05, 6.0, -12.0), Vector3(0.5, -12.0, -0.3),]:
		_fixture_level = parameters.x
		limiter.pre_gain_db = parameters.y
		limiter.ceiling_db = parameters.z
		capture.clear_buffer()
		_mix(system)
		var samples: PackedVector2Array = capture.get_buffer(capture.get_frames_available())
		var expected: float = minf(_fixture_level * pow(10.0, parameters.y / 20.0), pow(10.0, parameters.z / 20.0))
		var matched: bool = samples.size() >= 64
		for index: int in range(maxi(samples.size() - 64, 0), samples.size()):
			matched = matched and absf(samples[index].x - expected) < 0.0002 and absf(samples[index].y - expected) < 0.0002
		var measured: Vector2 = samples[-1] if not samples.is_empty() else Vector2.ZERO
		_expect(matched, "Limiter steady output after release history: input/pre/ceiling %s, expected %.6f, measured %s" % [parameters, expected, measured,])
	capture.remove_from_bus(bus)
	limiter.remove_from_bus(bus)
	bus.remove_dsp(source)
	source.release()
	_expect(bus.get_num_dsps() == initial_dsps, "Limiter fixture removes all DSPs")


func _test_spectrum(output_dir: String, tree: SceneTree) -> void:
	var system: FmodSystem = FmodServer.get_main_system()
	var bus: FmodChannelGroup = system.get_master_channel_group()
	var initial_dsps: int = bus.get_num_dsps()
	var effect: FmodAudioEffectSpectrumAnalyzer = FmodAudioEffectSpectrumAnalyzer.new()
	var preview: EditorProperty = await _inspect(effect, "FmodAudioEffectSpectrumAnalyzerPreview", tree)
	if preview == null:
		return
	_expect(bus.get_num_dsps() == initial_dsps and effect.get_bin_count() == 0, "Opening spectrum preview leaves audio routing unchanged")
	_expect(preview.mouse_filter == Control.MOUSE_FILTER_IGNORE and not preview.is_processing(), "Spectrum preview passes input and uses a timer")
	var timers: Array[Node] = preview.find_children("*", "Timer", true, false)
	_expect(timers.size() == 1, "Spectrum owns one refresh timer")
	var timer: Timer = timers[0] as Timer if timers.size() == 1 else null
	if timer != null:
		_expect(is_equal_approx(timer.wait_time, 0.1) and not timer.is_stopped(), "Visible spectrum refreshes at 10 Hz")
	var empty: Image = await _capture(preview, output_dir.path_join("spectrum_empty.png"), tree)
	_changes = 0
	effect.changed.connect(_changed)
	for iteration: int in range(2):
		effect.buffer_length = 4.0
		effect.fft_size = FmodAudioEffectSpectrumAnalyzer.FFT_SIZE_2048
		effect.immediate_mode = true
		_expect(_changes == 3, "FFT configuration notifies only on change, pass %d" % iteration)
	var left_oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	var right_oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	left_oscillator.set_parameter_float(1, 440.0)
	right_oscillator.set_parameter_float(1, 4000.0)
	var left_channel: FmodChannel = system.play_dsp(left_oscillator, bus)
	var right_channel: FmodChannel = system.play_dsp(right_oscillator, bus)
	left_channel.set_pan(-1.0)
	right_channel.set_pan(1.0)
	left_channel.set_volume_db(-6.0)
	right_channel.set_volume_db(-6.0)
	effect.apply_to(bus)
	_mix(system)
	await tree.create_timer(0.15).timeout
	var low: Vector2 = effect.get_magnitude_for_frequency_range(350.0, 550.0)
	var high: Vector2 = effect.get_magnitude_for_frequency_range(3500.0, 4500.0)
	_expect(low.x > low.y * 5.0 and high.y > high.x * 5.0 and low.x > 0.001 and high.y > 0.001, "Fixture supplies independent 440 Hz left and 4 kHz right spectra")
	var spectrum: Image = await _capture(preview, output_dir.path_join("spectrum_analyzer.png"), tree)
	if spectrum != null and empty != null:
		_expect(spectrum.get_data() != empty.get_data(), "Real FFT data draws stereo curves")
	preview.read_only = true
	await tree.create_timer(0.15).timeout
	_expect(effect.get_bin_count() > 0 and bus.get_num_dsps() == initial_dsps + 1, "Read-only spectrum continues displaying its existing FFT without adding DSPs")
	preview.read_only = false
	for size: int in range(5):
		effect.fft_size = size as FmodAudioEffectSpectrumAnalyzer.FFTSize
		_mix(system)
		_expect(effect.get_bin_count() == (256 << size), "FFT window %d exposes the requested bin count" % size)
	await _capture(preview, output_dir.path_join("spectrum_4096.png"), tree)
	if timer != null:
		preview.hide()
		await _settle(tree)
		_expect(timer.is_stopped(), "Hidden spectrum stops refreshing")
		preview.show()
		await _settle(tree)
		_expect(not timer.is_stopped(), "Visible spectrum resumes refreshing")
		var parent: Node = preview.get_parent()
		var index: int = preview.get_index()
		parent.remove_child(preview)
		parent.add_child(preview)
		parent.move_child(preview, index)
		await _settle(tree)
		_expect(not timer.is_stopped(), "Spectrum refresh resumes after inspector reparenting")
	var path: String = output_dir.path_join("spectrum_preview.tres")
	_expect(ResourceSaver.save(effect, path) == OK, "Spectrum resource saves")
	var restored: FmodAudioEffectSpectrumAnalyzer = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectSpectrumAnalyzer
	_expect(restored != null and restored.fft_size == FmodAudioEffectSpectrumAnalyzer.FFT_SIZE_4096 and restored.immediate_mode and restored.buffer_length == 4.0, "Spectrum restores saved configuration")
	effect.remove_from_bus(bus)
	left_channel.stop()
	right_channel.stop()
	left_oscillator.release()
	right_oscillator.release()
	await tree.create_timer(0.15).timeout
	_expect(effect.get_bin_count() == 0 and bus.get_num_dsps() == initial_dsps, "Detaching analyzer clears FFT data and restores routing")
	await _capture(preview, output_dir.path_join("spectrum_detached.png"), tree)
	await _check_disconnect(effect, tree)


func _exercise_drag(preview: EditorProperty, effect: FmodAudioEffect, property: StringName, start: Vector2, target: float, span: float, minimum: float, maximum: float, tree: SceneTree, shift: bool = false, ctrl: bool = false) -> void:
	var original: float = float(effect.get(property))
	var plot_height: float = preview.size.y - (42.0 if effect is FmodAudioEffectDistortion else 24.0) * EditorInterface.get_editor_scale()
	var position: Vector2 = start - Vector2(0.0, (target - original) / span * plot_height)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	_button(preview, start, true, shift, ctrl)
	_motion(preview, position, true)
	_expect(is_equal_approx(float(effect.get(property)), target), "%s drag updates live" % property)
	_button(preview, position, false)
	_expect(history.get_history_count() == 1, "%s drag creates one undo action" % property)
	history.undo()
	_expect(is_equal_approx(float(effect.get(property)), original), "%s supports undo" % property)
	history.redo()
	_expect(is_equal_approx(float(effect.get(property)), target), "%s supports redo" % property)
	_motion(preview, start, false)
	_expect(is_equal_approx(float(effect.get(property)), target), "%s ignores motion after release" % property)
	effect.set(property, original)
	await _settle(tree)
	history.clear_history()
	_button(preview, start, true, shift, ctrl)
	_button(preview, start, false)
	_expect(history.get_history_count() == 0, "%s unmoved click preserves undo history" % property)
	preview.read_only = true
	_button(preview, start, true, shift, ctrl)
	_motion(preview, position, true)
	_button(preview, position, false)
	_expect(is_equal_approx(float(effect.get(property)), original), "%s read-only preview rejects edits" % property)
	preview.read_only = false
	_button(preview, start, true, shift, ctrl)
	_motion(preview, start + Vector2(0.0, plot_height * 10.0), true)
	_expect(is_equal_approx(float(effect.get(property)), minimum), "%s clamps minimum" % property)
	_motion(preview, start - Vector2(0.0, plot_height * 10.0), true)
	_button(preview, start - Vector2(0.0, plot_height * 10.0), false)
	_expect(is_equal_approx(float(effect.get(property)), maximum) and history.get_history_count() == 1, "%s boundary drag clamps maximum in one action" % property)
	history.undo()
	_expect(is_equal_approx(float(effect.get(property)), original), "%s boundary undo restores value" % property)
	history.clear_history()


func _inspect(effect: FmodAudioEffect, preview_class: String, tree: SceneTree) -> EditorProperty:
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", preview_class, true, false)
	_expect(previews.size() == 1, "%s inserts one %s" % [effect.get_class(), preview_class,])
	if previews.size() != 1:
		return null
	var preview: EditorProperty = previews[0] as EditorProperty
	_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "%s receives 160 scaled pixels of height" % effect.get_class())
	_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "%s has no per-property reset button" % effect.get_class())
	_expect(effect.get_bus() == null, "%s opens without attaching DSP" % effect.get_class())
	return preview


func _check_disconnect(effect: FmodAudioEffect, tree: SceneTree) -> void:
	var count: int = effect.get_signal_connection_list(&"changed").size()
	EditorInterface.edit_resource(Resource.new())
	await _settle(tree)
	_expect(effect.get_signal_connection_list(&"changed").size() < count, "%s disconnects preview after resource switch" % effect.get_class())


func _distortion_center(preview: Control) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	return Vector2(8.0, 12.0) * scale + (preview.size - Vector2(64.0, 42.0) * scale) * 0.5


func _limiter_point(preview: Control, input: float, output: float) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var plot: Rect2 = Rect2(Vector2(8.0, 12.0) * scale, preview.size - Vector2(64.0, 24.0) * scale)
	return plot.position + Vector2((input + 80.0) / 80.0 * plot.size.x, (6.0 - output) / 86.0 * plot.size.y)


func _generate_constant(samples: PackedFloat32Array, _frames: int, _channels: int, _output_channels: int) -> Dictionary:
	for index: int in range(samples.size()):
		samples[index] = _fixture_level
	return {"output": samples,}


func _mix(system: FmodSystem) -> void:
	for block: int in range(64):
		system.update()


func _button(preview: Control, position: Vector2, pressed: bool, shift: bool = false, ctrl: bool = false) -> void:
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = pressed
	event.shift_pressed = shift
	event.ctrl_pressed = ctrl
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = preview.get_global_transform_with_canvas() * position
	event.global_position = event.position
	preview.get_viewport().push_input(event, true)


func _motion(preview: Control, position: Vector2, pressed: bool) -> void:
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = preview.get_global_transform_with_canvas() * position
	event.global_position = event.position
	preview.get_viewport().push_input(event, true)


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _capture(preview: Control, path: String, tree: SceneTree) -> Image:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return null
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = preview.get_viewport().get_texture().get_image()
	var area: Rect2i = Rect2i(preview.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if not area.has_area():
		return null
	var result: Image = image.get_region(area)
	_expect(result.save_png(path) == OK, "Rendered remaining effect preview captured")
	return result


func _changed() -> void:
	_changes += 1


func _expect(passed: bool, label: String) -> void:
	_checks.append({"name": label, "passed": passed,})
	if not passed:
		_failed = true
		print("REMAINING_EFFECTS_CHECK_FAILED ", label)
