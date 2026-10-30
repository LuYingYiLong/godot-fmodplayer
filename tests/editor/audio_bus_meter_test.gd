@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool


func run_startup(output_dir: String, tree: SceneTree) -> bool:
	if FmodServer.is_syncing_godot_buses():
		return true
	# 等待编辑器恢复已保存的总线布局，随后才开始第一次试听
	await _frames(tree, 4)
	var dock: EditorDock = EditorInterface.get_base_control().find_children("*", "FmodAudioBusesDock", true, false)[0] as EditorDock
	var meter: Control
	for row: Node in dock.find_children("*", "FmodAudioBusEditor", true, false):
		if row.get_meta(&"bus_id") == 0:
			meter = row.find_child("Meter", true, false) as Control
	var bar: TextureProgressBar = meter.get_child(0) as TextureProgressBar
	var samples: Array[Dictionary]
	samples.append(_startup_sample(dock, meter, bar, "startup"))
	var stream: FmodAudioStream = FmodAudioStream.new()
	stream.set_mode_flags(FmodAudioStream.MODE_SAMPLE | FmodAudioStream.MODE_LOOP)
	stream.set_audio_data(_tone())
	EditorInterface.edit_resource(stream)
	await tree.create_timer(0.2).timeout
	var panel: Control = EditorInterface.get_inspector().find_children("*", "FmodAudioPreviewProperty", true, false)[0] as Control
	panel.call(&"_play")
	await tree.create_timer(0.2).timeout
	var player: FmodAudioStreamPlayer = panel.find_children("*", "FmodAudioStreamPlayer", true, false)[0] as FmodAudioStreamPlayer
	_expect(player.is_playing(), "First inspector audition is playing through FMOD")
	samples.append(_startup_sample(dock, meter, bar, "first_play"))
	dock.make_visible()
	await tree.create_timer(0.4).timeout
	samples.append(_startup_sample(dock, meter, bar, "first_open"))
	var timer: Timer = _meter_timer(dock)
	_expect(not timer.is_stopped() and bar.value > 0.4 and bar.texture_over == null, "First inspector audition updates the existing Master meter without a layout edit")
	dock.hide()
	await tree.create_timer(0.1).timeout
	_expect(timer.is_stopped() and not meter.is_processing(), "Hidden Dock stops both presentation and metering")
	dock.make_visible()
	await tree.create_timer(0.3).timeout
	samples.append(_startup_sample(dock, meter, bar, "reopened"))
	_expect(not timer.is_stopped() and bar.value > 0.4 and bar.texture_over == null, "Reopening Dock resumes the same audition without another play command")
	# 原生 Dock 在恢复布局及浮动时会离开并重新进入场景树
	var parent: Node = dock.get_parent()
	var index: int = dock.get_index()
	parent.remove_child(dock)
	parent.add_child(dock)
	parent.move_child(dock, index)
	dock.make_visible()
	await tree.create_timer(0.4).timeout
	samples.append(_startup_sample(dock, meter, bar, "reentered"))
	_expect(meter.is_processing() and bar.value > 0.4 and bar.texture_over == null, "Dock reentry resumes metering during the same first audition")
	var subscribed: bool
	for connection: Dictionary in FmodServer.get_signal_connection_list(&"bus_layout_changed"):
		var callback: Callable = connection["callable"]
		if callback.get_object() == dock:
			subscribed = true
	_expect(subscribed, "Dock reentry reconnects runtime layout notifications")
	panel.call(&"_stop")
	EditorInterface.edit_resource(Resource.new())
	var report: FileAccess = FileAccess.open(output_dir.path_join("audio_bus_meter_startup.json"), FileAccess.WRITE)
	if report != null:
		report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks, "samples": samples,}, "\t"))
	return not _failed


func run(dock: EditorDock, output_dir: String, tree: SceneTree) -> bool:
	var layout: FmodAudioBusLayout = FmodServer.get_audio_bus_layout()
	var before: Array = layout.get_state()
	var id: int = layout.add_bus(&"Meter Playback Check")
	FmodServer.flush_bus_updates()
	await _frames(tree, 6)
	var meter: Control
	for row: Node in dock.find_children("*", "FmodAudioBusEditor", true, false):
		if row.get_meta(&"bus_id") == id:
			meter = row.find_child("Meter", true, false) as Control
	_expect(meter != null, "Playback bus owns a meter")
	if meter == null:
		layout.set_state(before)
		return false
	var bar: TextureProgressBar = meter.get_child(0) as TextureProgressBar
	var marker: Panel = bar.get_node("Peak") as Panel
	var hold: Timer = marker.get_node("PeakHold") as Timer
	var timer: Timer
	for node: Node in dock.get_children():
		if node is Timer and is_equal_approx((node as Timer).wait_time, 1.0 / 30.0):
			timer = node as Timer
	_expect(timer != null, "Dock refreshes presentation at 30 Hz")
	if timer == null:
		layout.set_state(before)
		return false
	var stream: FmodAudioStream = FmodAudioStream.new()
	stream.set_mode_flags(FmodAudioStream.MODE_SAMPLE | FmodAudioStream.MODE_LOOP)
	stream.set_audio_data(_tone())
	var player: FmodAudioStreamPlayer = FmodAudioStreamPlayer.new()
	player.stream = stream
	player.bus = &"Meter Playback Check"
	tree.root.add_child(player)
	player.play()
	await tree.create_timer(0.15).timeout
	_expect(bar.value > 0.4, "Editor playback produces a stable visible level")
	_expect(bar.texture_over == null, "Active playback uses the active gradient")
	var settings: EditorSettings = EditorInterface.get_editor_settings()
	var original: Color = settings.get_setting("editors/audio_buses/active_max_db_color")
	var custom: Color = Color(0.2, 0.8, 0.3, 1.0)
	settings.set_setting("editors/audio_buses/active_max_db_color", custom)
	settings.emit_signal(&"settings_changed")
	var texture: GradientTexture2D = bar.texture_progress as GradientTexture2D
	_expect(texture.gradient.colors[2] == custom, "Editing the native audio-bus palette immediately updates FMOD gradients")
	settings.set_setting("editors/audio_buses/active_max_db_color", original)
	settings.emit_signal(&"settings_changed")
	var first_value: float = bar.value
	player.volume_db = -24.0
	# 连续修改配置会刷新 header，不能一直推迟电平刷新
	var drag_started: int = Time.get_ticks_usec()
	var index: int
	var refresh_count: Array[int] = [0,]
	var counted: Callable = func() -> void: refresh_count[0] += 1
	timer.timeout.connect(counted)
	while index < 12 or Time.get_ticks_usec() - drag_started < 550000:
		layout.set_bus_volume_db(id, -float(index % 2))
		index += 1
		await tree.process_frame
	var during_drag_refreshes: int = refresh_count[0]
	await timer.timeout
	timer.timeout.disconnect(counted)
	var after_drag_value: float = bar.value
	_expect(during_drag_refreshes > 0 and bar.value < first_value - 0.02, "Continuous header refreshes do not starve the level timer")
	layout.set_bus_volume_db(id, 0.0)
	await tree.create_timer(0.2).timeout
	dock.call(&"save_layout")
	# 暂停界面刷新，并留出后台混音线程处理音量命令的时间
	timer.stop()
	player.volume_db = 0.0
	await tree.create_timer(0.06).timeout
	player.volume_db = -80.0
	await tree.create_timer(0.12).timeout
	_expect(timer.is_stopped(), "Transient check keeps UI refresh deferred")
	timer.emit_signal(&"timeout")
	_expect(bar.value > 0.4, "A short peak survives between UI refreshes")
	timer.start()
	player.volume_db = 0.0
	await tree.create_timer(0.15).timeout
	var head: FmodDSP = FmodServer.get_audio_bus_by_id(id).get_bus().get_dsp(0)
	head.set_metering_enabled(false, false)
	await _frames(tree, 4)
	_expect(head.get_metering_enabled()["output_enabled"], "Meter recovers when output metering is disabled on an unchanged DSP")
	var slot: int = layout.add_bus_effect(id, FmodAudioEffectCapture.new())
	FmodServer.flush_bus_updates()
	await tree.create_timer(0.12).timeout
	_expect(bar.value > 0.4, "Replacing the head DSP keeps playback metering alive")
	layout.set_bus_effect_enabled(id, slot, false)
	FmodServer.flush_bus_updates()
	await tree.create_timer(0.12).timeout
	_expect(bar.value > 0.4, "Bypassed Capture still meters the bus output")
	layout.remove_bus_effect(id, slot)
	FmodServer.flush_bus_updates()
	await tree.create_timer(0.12).timeout
	var peak_y: float = marker.position.y
	player.stop()
	await tree.create_timer(0.7).timeout
	_expect(absf(marker.position.y - peak_y) < 1.0, "Peak indicator holds its position before 1.5 seconds")
	var started: int = Time.get_ticks_usec()
	while marker.position.y <= peak_y + 1.0 and Time.get_ticks_usec() - started < 2200000:
		await tree.process_frame
	# 在显示刷新边界测量，避免把上一次刷新之前的时间算入距离
	await timer.timeout
	var y: float = marker.position.y
	started = Time.get_ticks_usec()
	while Time.get_ticks_usec() - started < 200000:
		await timer.timeout
	var elapsed: float = float(Time.get_ticks_usec() - started) / 1000000.0
	var pixels_per_second: float = (marker.position.y - y) / elapsed
	_expect(absf(pixels_per_second - 60.0) < 15.0, "Peak indicator falls at the native 60 pixels per second")
	_expect(is_equal_approx(hold.wait_time, 1.5), "Peak hold interval matches native Audio")
	player.free()
	layout.set_state(before)
	FmodServer.flush_bus_updates()
	await _frames(tree, 6)
	var report: FileAccess = FileAccess.open(output_dir.path_join("audio_bus_meter_playback.json"), FileAccess.WRITE)
	if report != null:
		report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks, "peak_fall_pixels_per_second": pixels_per_second, "volume_before_drag": first_value, "volume_after_drag": after_drag_value, "drag_frames": index, "refreshes_during_drag": during_drag_refreshes, "output_type": FmodServer.get_main_system().get_output(),}, "\t"))
	return not _failed


func _startup_sample(dock: EditorDock, meter: Control, bar: TextureProgressBar, phase: String) -> Dictionary:
	var timer: Timer = _meter_timer(dock)
	var dsp: FmodDSP = FmodServer.get_audio_bus_by_id(0).get_bus().get_dsp(0)
	return {"phase": phase, "visible": dock.is_visible_in_tree(), "processing": meter.is_processing(), "timer_running": not timer.is_stopped(), "level": bar.value, "active": bar.texture_over == null, "idle": dsp.get_idle(), "flags": dsp.get_metering_enabled(),}


func _meter_timer(dock: EditorDock) -> Timer:
	for node: Node in dock.get_children():
		if node is Timer and is_equal_approx((node as Timer).wait_time, 1.0 / 30.0):
			return node as Timer
	return null


func _tone() -> PackedByteArray:
	var frames: int = 48000
	var data: PackedByteArray
	data.resize(44 + frames * 4)
	data.encode_u32(0, 0x46464952)
	data.encode_u32(4, data.size() - 8)
	data.encode_u32(8, 0x45564157)
	data.encode_u32(12, 0x20746d66)
	data.encode_u32(16, 16)
	data.encode_u16(20, 1)
	data.encode_u16(22, 2)
	data.encode_u32(24, 48000)
	data.encode_u32(28, 192000)
	data.encode_u16(32, 4)
	data.encode_u16(34, 16)
	data.encode_u32(36, 0x61746164)
	data.encode_u32(40, frames * 4)
	for index: int in range(frames):
		var value: int = int(sin(TAU * 440.0 * float(index) / 48000.0) * 16000.0)
		data.encode_s16(44 + index * 4, value)
		data.encode_s16(46 + index * 4, value)
	return data


func _frames(tree: SceneTree, count: int) -> void:
	for index: int in range(count):
		await tree.process_frame


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("BUS_METER_CHECK_FAILED ", label)
