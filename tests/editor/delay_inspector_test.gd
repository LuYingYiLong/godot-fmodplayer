@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectDelay = FmodAudioEffectDelay.new()
	_reset_taps(effect)
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectDelayPreview", true, false)
	_expect(previews.size() == 1, "Delay inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Delay declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Delay preview has no reset button")
		_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty() and effect.get_bus() == null, "Delay caches geometry without polling or attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "Delay passes unhandled input")
		var native_fields: Array[StringName]
		for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
			var property_editor: EditorProperty = node as EditorProperty
			if property_editor != preview:
				native_fields.append(property_editor.get_edited_property())
		_expect(&"tap1_delay_ms" in native_fields and &"tap2_pan" in native_fields and &"feedback_lowpass" in native_fields, "Native delay fields retain their property editors")
		var initial: Image = await _capture_if_requested(preview, output_dir.path_join("delay.png"), tree)
		_set_parameters(effect)
		_expect(_changes == 13, "All thirteen delay parameters notify preview")
		_set_parameters(effect)
		_expect(_changes == 13, "Unchanged delay parameters skip notification")
		await _capture_if_requested(preview, output_dir.path_join("delay_updated.png"), tree)
		await _test_drag(preview, effect, 0, tree)
		await _test_drag(preview, effect, 1, tree)
		await _test_overlap(preview, effect, tree)
		_reset_taps(effect)
		effect.feedback_active = true
		var feedback: Image = await _capture_if_requested(preview, output_dir.path_join("delay_feedback.png"), tree)
		if initial != null and feedback != null:
			_expect(initial.get_data() != feedback.get_data(), "Feedback adds fading repeat rings and expands the time range")
		effect.feedback_delay_ms = 0.0
		effect.feedback_level_db = 0.0
		await _capture_if_requested(preview, output_dir.path_join("delay_zero_feedback.png"), tree)
		effect.tap1_delay_ms = 3000.0
		effect.tap2_delay_ms = 3000.0
		effect.feedback_delay_ms = 3000.0
		await _capture_if_requested(preview, output_dir.path_join("delay_max.png"), tree)
		effect.feedback_active = false
		effect.tap1_active = false
		effect.tap2_active = false
		effect.dry = 0.0
		var empty: Image = await _capture_if_requested(preview, output_dir.path_join("delay_empty.png"), tree)
		if initial != null and empty != null:
			_expect(initial.get_data() != empty.get_data(), "Disabled taps and zero dry leave only the reference rings")
		_mouse_button(preview, _position(preview, 3000.0, -0.6), true)
		_mouse_motion(preview, _position(preview, 250.0, 0.4), true)
		_mouse_button(preview, _position(preview, 250.0, 0.4), false)
		_expect(effect.tap1_delay_ms == 3000.0 and effect.tap2_delay_ms == 3000.0, "Disabled taps cannot be dragged")
		effect.tap1_active = true
		effect.tap1_delay_ms = 500.0
		effect.tap1_pan = 0.0
		effect.tap1_level_db = -60.0
		await _capture_if_requested(preview, output_dir.path_join("delay_muted.png"), tree)
		_mouse_button(preview, _position(preview, 500.0, 0.0), true)
		_mouse_motion(preview, _position(preview, 250.0, -0.5), true)
		_mouse_button(preview, _position(preview, 250.0, -0.5), false)
		_expect(is_equal_approx(effect.tap1_delay_ms, 250.0) and is_equal_approx(effect.tap1_pan, -0.5), "Muted tap retains an editable node")
		var path: String = output_dir.path_join("delay_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Delay saves")
		var restored: FmodAudioEffectDelay = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectDelay
		_expect(restored != null and is_equal_approx(restored.tap1_delay_ms, 250.0) and is_equal_approx(restored.tap1_pan, -0.5) and restored.tap1_level_db == -60.0 and not restored.tap2_active, "Saved Delay restores tap parameters and enable flags")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectStereoEnhance.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Delay preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("delay_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _set_parameters(effect: FmodAudioEffectDelay) -> void:
	effect.dry = 0.75
	effect.tap1_active = false
	effect.tap1_delay_ms = 350.0
	effect.tap1_level_db = -3.0
	effect.tap1_pan = 0.35
	effect.tap2_active = false
	effect.tap2_delay_ms = 750.0
	effect.tap2_level_db = -9.0
	effect.tap2_pan = -0.65
	effect.feedback_active = true
	effect.feedback_delay_ms = 400.0
	effect.feedback_level_db = -9.0
	effect.feedback_lowpass = 8000.0


func _reset_taps(effect: FmodAudioEffectDelay) -> void:
	effect.dry = 1.0
	effect.feedback_active = false
	effect.feedback_delay_ms = 340.0
	effect.feedback_level_db = -6.0
	effect.tap1_active = true
	effect.tap2_active = true
	effect.tap1_delay_ms = 250.0
	effect.tap2_delay_ms = 500.0
	effect.tap1_pan = -0.6
	effect.tap2_pan = 0.65
	effect.tap1_level_db = -6.0
	effect.tap2_level_db = -12.0


func _test_drag(preview: EditorProperty, effect: FmodAudioEffectDelay, tap: int, tree: SceneTree) -> void:
	_reset_taps(effect)
	await _settle(tree)
	var prefix: String = "tap1_" if tap == 0 else "tap2_"
	var original_time: float = float(effect.get(prefix + "delay_ms"))
	var original_pan: float = float(effect.get(prefix + "pan"))
	var target_time: float = 750.0 if tap == 0 else 800.0
	var target_pan: float = 0.4 if tap == 0 else -0.3
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var original: Vector2 = _position(preview, original_time, original_pan)
	var target: Vector2 = _position(preview, target_time, target_pan)
	_mouse_button(preview, original, true)
	_mouse_motion(preview, target, true)
	_expect(is_equal_approx(float(effect.get(prefix + "delay_ms")), target_time) and is_equal_approx(float(effect.get(prefix + "pan")), target_pan), "Tap %d drag updates time and pan live" % tap)
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 1, "Tap %d drag creates one undo action for both parameters" % tap)
	history.undo()
	_expect(is_equal_approx(float(effect.get(prefix + "delay_ms")), original_time) and is_equal_approx(float(effect.get(prefix + "pan")), original_pan), "Tap %d undo restores time and pan" % tap)
	history.redo()
	_expect(is_equal_approx(float(effect.get(prefix + "delay_ms")), target_time) and is_equal_approx(float(effect.get(prefix + "pan")), target_pan), "Tap %d redo restores time and pan" % tap)
	_mouse_motion(preview, original, false)
	_expect(is_equal_approx(float(effect.get(prefix + "delay_ms")), target_time), "Tap %d motion after release preserves time" % tap)
	_expect(effect.tap1_level_db == -6.0 and effect.tap2_level_db == -12.0, "Tap %d drag preserves levels" % tap)
	history.clear_history()
	_mouse_button(preview, target, true)
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 0, "Tap %d unmoved click preserves undo history" % tap)
	preview.read_only = true
	_mouse_button(preview, target, true)
	_mouse_motion(preview, original, true)
	_mouse_button(preview, original, false)
	_expect(is_equal_approx(float(effect.get(prefix + "delay_ms")), target_time), "Read-only Tap %d rejects edits" % tap)
	preview.read_only = false
	_mouse_button(preview, target, true)
	_mouse_motion(preview, _position(preview, 4000.0, -1.0), true)
	_expect(float(effect.get(prefix + "delay_ms")) == 3000.0 and float(effect.get(prefix + "pan")) == -1.0, "Tap %d clamps maximum delay and left pan" % tap)
	_mouse_motion(preview, Vector2(preview.size.x * 0.5, preview.size.y - 12.0 * EditorInterface.get_editor_scale()), true)
	_mouse_button(preview, Vector2(preview.size.x * 0.5, preview.size.y - 12.0 * EditorInterface.get_editor_scale()), false)
	_expect(float(effect.get(prefix + "delay_ms")) == 0.0, "Tap %d clamps minimum delay at origin" % tap)
	_expect(history.get_history_count() == 1, "Tap %d boundary drag remains one undo action" % tap)
	history.undo()
	_expect(is_equal_approx(float(effect.get(prefix + "delay_ms")), target_time) and is_equal_approx(float(effect.get(prefix + "pan")), target_pan), "Tap %d boundary undo restores both parameters" % tap)
	history.clear_history()


func _test_overlap(preview: EditorProperty, effect: FmodAudioEffectDelay, tree: SceneTree) -> void:
	_reset_taps(effect)
	await _settle(tree)
	_mouse_button(preview, _position(preview, 250.0, -0.6), true)
	_mouse_button(preview, _position(preview, 250.0, -0.6), false)
	effect.tap1_delay_ms = 500.0
	effect.tap1_pan = 0.0
	effect.tap2_pan = 0.0
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	_mouse_button(preview, _position(preview, 500.0, 0.0), true, true)
	_mouse_motion(preview, _position(preview, 750.0, -0.5), true)
	_mouse_button(preview, _position(preview, 750.0, -0.5), false)
	_expect(effect.tap1_delay_ms == 500.0 and effect.tap1_pan == 0.0 and is_equal_approx(effect.tap2_delay_ms, 750.0) and is_equal_approx(effect.tap2_pan, -0.5), "Alt-click selects the other overlapping tap")
	_expect(history.get_history_count() == 1, "Overlapping tap drag remains one undo action")
	history.undo()
	_expect(effect.tap1_delay_ms == 500.0 and effect.tap2_delay_ms == 500.0 and effect.tap1_pan == 0.0 and effect.tap2_pan == 0.0, "Overlap undo restores identical tap positions")
	history.clear_history()


func _position(preview: Control, time: float, pan: float, time_range: float = 1000.0) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var center: Vector2 = Vector2(preview.size.x * 0.5, preview.size.y - 12.0 * scale)
	var outer: float = minf(preview.size.x * 0.5 - 14.0 * scale, preview.size.y - 24.0 * scale)
	var radius: float = 12.0 * scale + time / time_range * (outer - 12.0 * scale)
	var angle: float = (pan - 1.0) * PI * 0.5
	return center + Vector2(cos(angle), sin(angle)) * radius


func _mouse_button(preview: Control, position: Vector2, pressed: bool, alternate: bool = false) -> void:
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = pressed
	event.alt_pressed = alternate
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = preview.get_global_transform_with_canvas() * position
	event.global_position = event.position
	preview.get_viewport().push_input(event, true)


func _mouse_motion(preview: Control, position: Vector2, pressed: bool) -> void:
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = preview.get_global_transform_with_canvas() * position
	event.global_position = event.position
	preview.get_viewport().push_input(event, true)


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _capture_if_requested(preview: Control, path: String, tree: SceneTree) -> Image:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return null
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = preview.get_viewport().get_texture().get_image()
	var area: Rect2i = Rect2i(preview.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if not area.has_area():
		return null
	var cropped: Image = image.get_region(area)
	_expect(cropped.save_png(path) == OK, "Rendered Delay preview captured")
	return cropped


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("DELAY_INSPECTOR_CHECK_FAILED ", label)
