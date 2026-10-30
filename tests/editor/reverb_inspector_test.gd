@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectReverb = FmodAudioEffectReverb.new()
	effect.room_size = 0.85
	effect.spread = 0.9
	effect.damping = 0.35
	effect.wet = 0.8
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectReverbPreview", true, false)
	_expect(previews.size() == 1, "Reverb inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Reverb declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Reverb preview has no reset button")
		_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty(), "Reverb caches geometry and redraws without animation polling")
		_expect(effect.get_bus() == null, "Opening Reverb preview does not attach DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "Reverb preview passes unhandled input")
		var initial: Image = await _capture_if_requested(preview, output_dir.path_join("reverb.png"), tree)
		_set_parameters(effect)
		_expect(_changes == 8, "Reverb parameter changes notify the preview")
		_set_parameters(effect)
		_expect(_changes == 8, "Unchanged Reverb parameters skip notification")
		var updated: Image = await _capture_if_requested(preview, output_dir.path_join("reverb_updated.png"), tree)
		if initial != null and updated != null:
			_expect(initial.get_data() != updated.get_data(), "Reverb parameters update projected ring geometry")
		await _test_spread_drag(preview, effect, output_dir, tree)
		effect.room_size = 1.0
		effect.spread = 1.0
		effect.predelay_msec = 500.0
		effect.predelay_feedback = 0.98
		effect.damping = 0.0
		effect.wet = 1.0
		effect.dry = 1.0
		await _capture_if_requested(preview, output_dir.path_join("reverb_max.png"), tree)
		effect.room_size = 0.0
		effect.spread = 0.0
		effect.predelay_msec = 0.0
		effect.predelay_feedback = 0.0
		effect.wet = 0.0
		effect.dry = 0.0
		var zero: Image = await _capture_if_requested(preview, output_dir.path_join("reverb_zero.png"), tree)
		if initial != null and zero != null:
			_expect(initial.get_data() != zero.get_data(), "Zero Wet and Dry clear reflection rings and source marker")
		preview.read_only = true
		effect.room_size = 0.75
		await _settle(tree)
		_expect(is_equal_approx(effect.room_size, 0.75), "Read-only preview continues observing resource changes")
		var path: String = output_dir.path_join("reverb_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Reverb saves")
		var restored: FmodAudioEffectReverb = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectReverb
		_expect(restored != null and restored.room_size == 0.75 and restored.spread == 0.0 and restored.wet == 0.0 and restored.dry == 0.0, "Saved Reverb restores its parameters")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectDelay.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Reverb preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("reverb_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _set_parameters(effect: FmodAudioEffectReverb) -> void:
	effect.damping = 0.8
	effect.dry = 0.4
	effect.hipass = 0.3
	effect.predelay_feedback = 0.6
	effect.predelay_msec = 320.0
	effect.room_size = 0.3
	effect.spread = 0.4
	effect.wet = 0.3


func _test_spread_drag(preview: EditorProperty, effect: FmodAudioEffectReverb, output_dir: String, tree: SceneTree) -> void:
	effect.room_size = 0.8
	effect.damping = 0.35
	effect.predelay_feedback = 0.4
	effect.predelay_msec = 150.0
	effect.wet = 0.8
	effect.dry = 1.0
	for spread: float in [0.0, 0.5, 1.0,]:
		effect.spread = spread
		await _capture_if_requested(preview, output_dir.path_join("reverb_spread_%d.png" % roundi(spread * 100.0)), tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	var scale: float = EditorInterface.get_editor_scale()
	var separation: float = minf(42.0 * scale, (preview.size.x - 24.0 * scale) * 0.14)
	for side: float in [-1.0, 1.0,]:
		effect.spread = 0.4
		await _settle(tree)
		history.clear_history()
		var start: Vector2 = _handle_position(preview, effect, side) + Vector2(3.0 * scale * side, 0.0)
		_mouse_motion(preview, start, false)
		_expect(preview.mouse_default_cursor_shape == Control.CURSOR_HSIZE, "Spread handle uses horizontal resize cursor, side %d" % side)
		if side > 0.0:
			await _capture_if_requested(preview, output_dir.path_join("reverb_spread_hover.png"), tree)
		_mouse_button(preview, start, true)
		_expect(is_equal_approx(effect.spread, 0.4), "Off-center handle press preserves Spread, side %d" % side)
		var end: Vector2 = start + Vector2(side * separation * 0.3, 0.0)
		_mouse_motion(preview, end, true)
		_expect(is_equal_approx(effect.spread, 0.7), "Either handle increases Spread when dragged outward, side %d" % side)
		_mouse_button(preview, end, false)
		_expect(history.get_history_count() == 1, "Spread drag creates one undo action, side %d" % side)
		history.undo()
		_expect(is_equal_approx(effect.spread, 0.4), "Spread supports undo, side %d" % side)
		history.redo()
		_expect(is_equal_approx(effect.spread, 0.7), "Spread supports redo, side %d" % side)
		_mouse_motion(preview, start, false)
		_expect(is_equal_approx(effect.spread, 0.7), "Motion after release preserves Spread, side %d" % side)
		history.clear_history()
		start = _handle_position(preview, effect, side)
		_mouse_button(preview, start, true)
		_mouse_button(preview, start, false)
		_expect(is_equal_approx(effect.spread, 0.7) and history.get_history_count() == 0, "Unmoved click preserves Spread and history, side %d" % side)
		_mouse_button(preview, start, true)
		end = start + Vector2(0.0, 24.0 * scale)
		_mouse_motion(preview, end, true)
		_mouse_button(preview, end, false)
		_expect(is_equal_approx(effect.spread, 0.7) and history.get_history_count() == 0, "Vertical motion preserves Spread, side %d" % side)
		_mouse_button(preview, start, true)
		_mouse_motion(preview, start + Vector2(side * 1000.0, 0.0), true)
		_expect(effect.spread == 1.0, "Outward drag clamps Spread to one, side %d" % side)
		end = start - Vector2(side * 1000.0, 0.0)
		_mouse_motion(preview, end, true)
		_mouse_button(preview, end, false)
		_expect(effect.spread == 0.0 and history.get_history_count() == 1, "Inward drag clamps Spread to zero in one undo action, side %d" % side)
		history.undo()
		_expect(is_equal_approx(effect.spread, 0.7), "Boundary drag undo restores Spread, side %d" % side)
		effect.spread = 0.0
		await _settle(tree)
		history.clear_history()
		start = _handle_position(preview, effect, side)
		_mouse_button(preview, start, true)
		end = start + Vector2(side * separation * 0.5, 0.0)
		_mouse_motion(preview, end, true)
		_mouse_button(preview, end, false)
		_expect(is_equal_approx(effect.spread, 0.5), "Both handles remain usable when reflection rings overlap, side %d" % side)
		preview.read_only = true
		start = _handle_position(preview, effect, side)
		_mouse_button(preview, start, true)
		_mouse_motion(preview, start + Vector2(side * separation, 0.0), true)
		_mouse_button(preview, start + Vector2(side * separation, 0.0), false)
		_expect(is_equal_approx(effect.spread, 0.5) and preview.mouse_default_cursor_shape == Control.CURSOR_ARROW, "Read-only preview rejects Spread edits, side %d" % side)
		preview.read_only = false
	history.clear_history()
	_mouse_button(preview, Vector2.ONE, true)
	_mouse_motion(preview, _handle_position(preview, effect, 1.0), true)
	_mouse_button(preview, _handle_position(preview, effect, 1.0), false)
	_expect(is_equal_approx(effect.spread, 0.5) and history.get_history_count() == 0, "Dragging from empty space does not edit Spread")
	_mouse_motion(preview, Vector2.ONE, false)
	_expect(preview.mouse_default_cursor_shape == Control.CURSOR_ARROW, "Leaving Spread handles restores arrow cursor")
	_expect(
		is_equal_approx(effect.room_size, 0.8)
		and is_equal_approx(effect.damping, 0.35)
		and is_equal_approx(effect.predelay_feedback, 0.4)
		and effect.predelay_msec == 150.0
		and is_equal_approx(effect.wet, 0.8)
		and effect.dry == 1.0
		and is_equal_approx(effect.hipass, 0.3),
		"Spread edits preserve other Reverb parameters",
	)


func _handle_position(preview: Control, effect: FmodAudioEffectReverb, side: float) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var available: Vector2 = preview.size - Vector2(24.0, 40.0) * scale
	var center: Vector2 = Vector2(12.0, 28.0) * scale + available * 0.5
	var height: float = available.y * (0.62 + 0.30 * effect.room_size + 0.08 * effect.predelay_feedback)
	var radius: float = minf(available.x * 0.2, height * 0.4)
	var separation: float = minf(42.0 * scale, available.x * 0.14)
	return center + Vector2(side * (radius + separation * effect.spread), -height * 0.17)


func _mouse_button(preview: Control, position: Vector2, pressed: bool) -> void:
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = pressed
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
	_expect(cropped.save_png(path) == OK, "Rendered Reverb preview captured")
	return cropped


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("REVERB_INSPECTOR_CHECK_FAILED ", label)
