@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectPhaser = FmodAudioEffectPhaser.new()
	effect.range_max_hz = 6000.0
	effect.range_min_hz = 500.0
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectPhaserPreview", true, false)
	_expect(previews.size() == 1, "Phaser inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Phaser declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Phaser preview has no reset button")
		_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty() and effect.get_bus() == null, "Phaser uses cached static geometry without polling or attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "Phaser passes unhandled input")
		var native_fields: Array[StringName]
		for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
			var property_editor: EditorProperty = node as EditorProperty
			if property_editor != preview:
				native_fields.append(property_editor.get_edited_property())
		_expect(&"range_min_hz" in native_fields and &"range_max_hz" in native_fields, "Native range fields retain their property editors")
		var initial: Image = await _capture_if_requested(preview, output_dir.path_join("phaser.png"), tree)
		if initial != null:
			var inside: Vector2i = Vector2i(_position(preview, 2000.0, 0.85))
			var outside: Vector2i = Vector2i(_position(preview, 100.0, 0.85))
			_expect(initial.get_pixelv(inside) != initial.get_pixelv(outside), "Sweep range renders a translucent fill inside its frequency limits")
		_set_parameters(effect)
		_expect(_changes == 5, "All Phaser parameter changes notify preview")
		_set_parameters(effect)
		_expect(_changes == 5, "Unchanged Phaser parameters skip notification")
		var updated: Image = await _capture_if_requested(preview, output_dir.path_join("phaser_updated.png"), tree)
		if initial != null and updated != null:
			_expect(initial.get_data() != updated.get_data(), "Range and feedback update output response, phase and range fill")
		await _test_response_curves(preview, effect, output_dir, tree)
		await _test_drag(preview, effect, 0, tree)
		await _test_drag(preview, effect, 1, tree)
		await _test_collapsed_range(preview, effect, tree)
		effect.range_min_hz = 10.0
		effect.range_max_hz = 10000.0
		effect.feedback = 0.9
		await _capture_if_requested(preview, output_dir.path_join("phaser_full_range.png"), tree)
		var path: String = output_dir.path_join("phaser_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Phaser saves")
		var restored: FmodAudioEffectPhaser = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectPhaser
		_expect(restored != null and restored.range_min_hz == 10.0 and restored.range_max_hz == 10000.0 and is_equal_approx(restored.feedback, 0.9), "Saved Phaser restores range and feedback")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectDelay.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Phaser preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("phaser_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_response_curves(preview: EditorProperty, effect: FmodAudioEffectPhaser, output_dir: String, tree: SceneTree) -> void:
	effect.depth = 0.25
	var low: Image = await _capture_if_requested(preview, output_dir.path_join("phaser_low_depth.png"), tree)
	effect.depth = 2.0
	var high: Image = await _capture_if_requested(preview, output_dir.path_join("phaser_high_depth.png"), tree)
	if low != null and high != null:
		_expect(low.get_data() != high.get_data(), "Depth changes mixed output gain in the main curve")
	effect.feedback = 0.85
	var feedback: Image = await _capture_if_requested(preview, output_dir.path_join("phaser_high_feedback.png"), tree)
	if high != null and feedback != null:
		_expect(high.get_data() != feedback.get_data(), "Feedback changes the mixed output response")
	effect.rate_hz = 10.0
	var rate: Image = await _capture_if_requested(preview, output_dir.path_join("phaser_rate.png"), tree)
	if feedback != null and rate != null:
		_expect(feedback.get_data() == rate.get_data(), "Rate preserves the frozen sweep midpoint response")


func _set_parameters(effect: FmodAudioEffectPhaser) -> void:
	effect.range_min_hz = 1000.0
	effect.range_max_hz = 8000.0
	effect.feedback = 0.4
	effect.depth = 2.0
	effect.rate_hz = 2.0


func _test_drag(preview: EditorProperty, effect: FmodAudioEffectPhaser, boundary: int, tree: SceneTree) -> void:
	effect.range_min_hz = 500.0
	effect.range_max_hz = 6000.0
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var property: StringName = &"range_min_hz" if boundary == 0 else &"range_max_hz"
	var original: float = float(effect.get(property))
	var target: float = 1000.0 if boundary == 0 else 8000.0
	_mouse_button(preview, _position(preview, original), true)
	_mouse_motion(preview, _position(preview, target), true)
	_expect(is_equal_approx(float(effect.get(property)), target), "Range boundary %d drag updates parameter live" % boundary)
	_mouse_button(preview, _position(preview, target), false)
	_expect(history.get_history_count() == 1, "Range boundary %d drag creates one undo action" % boundary)
	history.undo()
	_expect(is_equal_approx(float(effect.get(property)), original), "Range boundary %d supports undo" % boundary)
	history.redo()
	_expect(is_equal_approx(float(effect.get(property)), target), "Range boundary %d supports redo" % boundary)
	_expect(is_equal_approx(effect.range_max_hz if boundary == 0 else effect.range_min_hz, 6000.0 if boundary == 0 else 500.0), "Range boundary %d preserves other boundary" % boundary)
	_mouse_motion(preview, _position(preview, original), false)
	_expect(is_equal_approx(float(effect.get(property)), target), "Range boundary %d ignores motion after release" % boundary)
	history.clear_history()
	_mouse_button(preview, _position(preview, target), true)
	_mouse_button(preview, _position(preview, target), false)
	_expect(history.get_history_count() == 0, "Range boundary %d unmoved click preserves undo history" % boundary)
	preview.read_only = true
	_mouse_button(preview, _position(preview, target), true)
	_mouse_motion(preview, _position(preview, original), true)
	_mouse_button(preview, _position(preview, original), false)
	_expect(is_equal_approx(float(effect.get(property)), target), "Read-only range boundary %d rejects drag" % boundary)
	preview.read_only = false
	_mouse_button(preview, _position(preview, target), true)
	_mouse_motion(preview, Vector2(-100.0, _position(preview, target).y), true)
	_expect(is_equal_approx(float(effect.get(property)), 10.0 if boundary == 0 else effect.range_min_hz), "Range boundary %d clamps left extent without crossing" % boundary)
	_mouse_motion(preview, Vector2(preview.size.x + 100.0, _position(preview, target).y), true)
	_mouse_button(preview, Vector2(preview.size.x + 100.0, _position(preview, target).y), false)
	_expect(is_equal_approx(float(effect.get(property)), effect.range_max_hz if boundary == 0 else 10000.0), "Range boundary %d clamps right extent without crossing" % boundary)
	history.clear_history()


func _test_collapsed_range(preview: EditorProperty, effect: FmodAudioEffectPhaser, tree: SceneTree) -> void:
	effect.range_min_hz = 500.0
	effect.range_max_hz = 500.0
	await _settle(tree)
	var position: Vector2 = _position(preview, 500.0) + Vector2(2.0, 0.0)
	var target: Vector2 = _position(preview, 2000.0) + Vector2(2.0, 0.0)
	_mouse_button(preview, position, true)
	_mouse_motion(preview, target, true)
	_mouse_button(preview, target, false)
	_expect(effect.range_min_hz == 500.0 and is_equal_approx(effect.range_max_hz, 2000.0), "Coincident boundaries can open toward higher frequencies")
	effect.range_max_hz = 500.0
	position = _position(preview, 500.0) - Vector2(2.0, 0.0)
	target = _position(preview, 100.0) - Vector2(2.0, 0.0)
	_mouse_button(preview, position, true)
	_mouse_motion(preview, target, true)
	_mouse_button(preview, target, false)
	_expect(effect.range_max_hz == 500.0 and is_equal_approx(effect.range_min_hz, 100.0), "Coincident boundaries can open toward lower frequencies")


func _position(preview: Control, frequency: float, height: float = 0.5) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var plot: Rect2 = Rect2(Vector2(8.0, 18.0) * scale, preview.size - Vector2(16.0, 48.0) * scale)
	return plot.position + Vector2(log(frequency / 10.0) / log(2000.0) * plot.size.x, height * plot.size.y)


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
	_expect(cropped.save_png(path) == OK, "Rendered Phaser preview captured")
	return cropped


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("PHASER_INSPECTOR_CHECK_FAILED ", label)
