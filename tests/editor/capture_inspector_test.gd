@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _source_frame: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	effect.buffer_length = 10.0
	var bus: FmodChannelGroup = FmodServer.get_main_system().get_master_channel_group()
	var original_dsps: int = bus.get_num_dsps()
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectCapturePreview", true, false)
	_expect(previews.size() == 1, "Capture inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Capture declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Capture preview has no reset button")
		_expect(not preview.is_processing() and preview.mouse_filter == Control.MOUSE_FILTER_IGNORE, "Capture preview uses a timer and passes mouse input")
		_expect(effect.get_bus() == null and bus.get_num_dsps() == original_dsps, "Opening Capture preview does not attach DSP")
		bus = null
		var timers: Array[Node] = preview.find_children("*", "Timer", true, false)
		_expect(timers.size() == 1, "Capture preview owns one refresh timer")
		if timers.size() == 1:
			var timer: Timer = timers[0] as Timer
			_expect(is_equal_approx(timer.wait_time, 0.1) and not timer.is_stopped(), "Visible Capture preview refreshes at 10 Hz")
			await _capture_if_requested(preview, output_dir.path_join("capture_empty.png"), tree)
			_fill_capture(effect)
			var available: int = effect.get_frames_available()
			var pushed: int = effect.get_pushed_frames()
			var discarded: int = effect.get_discarded_frames()
			_expect(available == 480000, "Fixture fills ten seconds of stereo capture")
			await tree.create_timer(0.25).timeout
			_expect(effect.get_frames_available() == available and effect.get_pushed_frames() == pushed and effect.get_discarded_frames() == discarded, "Repeated preview snapshots preserve queued frames and counters")
			await _capture_if_requested(preview, output_dir.path_join("capture.png"), tree)
			var samples: PackedVector2Array = effect.get_buffer(8)
			var ordered: bool = samples.size() == 8
			for index: int in range(samples.size()):
				var left: float = 0.5 if index % 2 == 0 else -0.5
				ordered = ordered and samples[index] == Vector2(left, -left * 0.5)
			_expect(ordered, "Business reader receives original stereo frames after preview")
			preview.read_only = true
			await tree.create_timer(0.15).timeout
			_expect(effect.get_frames_available() == available - 8, "Read-only Capture preview remains non-consuming")
			preview.read_only = false
			preview.hide()
			await _settle(tree)
			_expect(timer.is_stopped(), "Hidden Capture preview stops refreshing")
			effect.clear_buffer()
			preview.show()
			await _settle(tree)
			_expect(not timer.is_stopped() and effect.get_frames_available() == 0, "Visible Capture preview resumes with cleared state")
			effect.buffer_length = 2.0
			await _settle(tree)
			_expect(effect.get_frames_available() == 0 and effect.buffer_length == 2.0, "Changing duration clears queued data and updates preview")
			await _capture_if_requested(preview, output_dir.path_join("capture_resized.png"), tree)
		var path: String = output_dir.path_join("capture_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Capture resource saves")
		var restored: FmodAudioEffectCapture = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectCapture
		_expect(restored != null and restored.buffer_length == effect.buffer_length, "Capture resource restores buffer duration")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectAmplify.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Capture preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("capture_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _fill_capture(effect: FmodAudioEffectCapture) -> void:
	# 编辑器测试位于隔离项目，最后使用离线系统生成确定性的真实捕获数据
	var system: FmodSystem = FmodSystem.create_system()
	system.set_output(FmodSystem.FMOD_OUTPUT_TYPE_NOSOUND_NRT)
	system.set_software_format(48000, FmodSystem.FMOD_SPEAKER_MODE_STEREO, 0)
	system.set_dsp_buffer_size(1024, 4)
	system.init(32, FmodSystem.FMOD_INIT_FLAG_STREAM_FROM_UPDATE)
	FmodServer.reset_main_system(system)
	var bus: FmodChannelGroup = system.get_master_channel_group()
	var initial_dsps: int = bus.get_num_dsps()
	var source: FmodDSP = system.create_dsp("Capture preview fixture")
	source.set_input_channel_format(2, FmodSystem.FMOD_SPEAKER_MODE_STEREO)
	source.set_process_callback(_generate_samples)
	bus.add_dsp(-1, source)
	effect.apply_to(bus)
	for block: int in range(469):
		system.update()
	_expect(effect.get_pushed_frames() == 480256 and effect.get_discarded_frames() == 256, "Fixture exercises full-buffer overflow")
	# 卸载后保留未读帧，后续等待编辑器刷新时不会继续向缓冲写入
	effect.remove_from_bus(bus)
	bus.remove_dsp(source)
	source.release()
	_expect(bus.get_num_dsps() == initial_dsps, "Capture fixture leaves no attached DSP")


func _generate_samples(samples: PackedFloat32Array, frames: int, channels: int, _output_channels: int) -> Dictionary:
	for frame: int in range(frames):
		var left: float = 0.5 if (_source_frame + frame) % 2 == 0 else -0.5
		for channel: int in range(channels):
			samples[frame * channels + channel] = left if channel == 0 else -left * 0.5
	_source_frame += frames
	return {"output": samples,}


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _capture_if_requested(preview: Control, path: String, tree: SceneTree) -> void:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = preview.get_viewport().get_texture().get_image()
	var area: Rect2i = Rect2i(preview.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if area.has_area():
		_expect(image.get_region(area).save_png(path) == OK, "Rendered Capture preview captured")


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("CAPTURE_INSPECTOR_CHECK_FAILED ", label)
