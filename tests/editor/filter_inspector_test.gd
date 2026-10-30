@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectFilter = FmodAudioEffectFilter.new()
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectFilterPreview", true, false)
	_expect(previews.size() == 1, "Filter inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Filter declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Filter preview has no reset button")
		_expect(not preview.is_processing() and effect.get_bus() == null, "Filter preview redraws on demand without attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "Filter preview passes unhandled wheel events")
		effect.cutoff_hz = 3000.0
		_expect(_changes == 1, "Cutoff changes notify the preview")
		effect.cutoff_hz = 3000.0
		_expect(_changes == 1, "Unchanged cutoff skips notification")
		effect.db = FmodAudioEffectFilter.FILTER_12DB
		effect.gain = 2.0
		effect.resonance = 0.8
		_expect(_changes == 4, "Slope, gain and resonance changes notify the preview")
		effect.db = FmodAudioEffectFilter.FILTER_12DB
		effect.gain = 2.0
		effect.resonance = 0.8
		_expect(_changes == 4, "Unchanged filter parameters skip notifications")
		for slope: int in range(4):
			await _test_zero_line(preview, effect, slope, output_dir, tree)
			await _test_drag(preview, effect, slope, tree)
			await _capture_if_requested(preview, output_dir.path_join("filter_%d.png" % slope), tree)
		var path: String = output_dir.path_join("filter_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Filter saves")
		var restored: FmodAudioEffectFilter = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectFilter
		_expect(restored != null and restored.cutoff_hz == effect.cutoff_hz and restored.resonance == effect.resonance and restored.db == effect.db, "Saved Filter restores edited parameters")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectAmplify.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Filter preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("filter_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_zero_line(preview: EditorProperty, effect: FmodAudioEffectFilter, slope: int, output_dir: String, tree: SceneTree) -> void:
	effect.db = slope as FmodAudioEffectFilter.FilterDB
	var scale: float = EditorInterface.get_editor_scale()
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	for resonance: float in [0.0, 0.05, 0.5, 1.0,]:
		effect.cutoff_hz = 2000.0
		effect.resonance = resonance
		await _settle(tree)
		var original_resonance: float = effect.resonance
		var position: Vector2 = _position(preview, effect)
		var image: Image = await _capture_if_requested(preview, output_dir.path_join("filter_zero_%d_%s.png" % [slope, resonance,]), tree)
		if image != null:
			var above: Color = image.get_pixelv(Vector2i(position - Vector2(0.0, 2.0 * scale)))
			var background: Color = image.get_pixel(1, 1)
			_expect(above != background, "Slope %d resonance %s renders handle centered on zero line" % [slope, resonance,])
		history.clear_history()
		var offset: Vector2 = Vector2(3.0, -1.0) * scale
		_mouse_button(preview, position + offset, true)
		_mouse_button(preview, position + offset, false)
		_expect(effect.cutoff_hz == 2000.0 and effect.resonance == original_resonance and history.get_history_count() == 0, "Slope %d resonance %s offset click preserves parameters and undo history" % [slope, resonance,])
		var target: Vector2 = _position_for(preview, 4000.0) + offset
		_mouse_button(preview, position + offset, true)
		_mouse_motion(preview, target, true)
		_mouse_button(preview, target, false)
		_expect(is_equal_approx(effect.cutoff_hz, 4000.0) and effect.resonance == original_resonance, "Slope %d resonance %s horizontal drag preserves resonance" % [slope, resonance,])
		_expect(history.get_history_count() == 1, "Slope %d resonance %s horizontal drag is one undo action" % [slope, resonance,])
		history.undo()
		_expect(is_equal_approx(effect.cutoff_hz, 2000.0) and effect.resonance == original_resonance, "Slope %d resonance %s horizontal undo restores both parameters" % [slope, resonance,])
		history.clear_history()


func _test_drag(preview: EditorProperty, effect: FmodAudioEffectFilter, slope: int, tree: SceneTree) -> void:
	effect.db = slope as FmodAudioEffectFilter.FilterDB
	effect.cutoff_hz = 2000.0
	effect.resonance = 0.5
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var original: Vector2 = _position(preview, effect)
	var target: Vector2 = _position_for(preview, 4000.0, 0.3)
	_mouse_button(preview, original, true)
	_mouse_motion(preview, target, true)
	_expect(is_equal_approx(effect.cutoff_hz, 4000.0), "Slope %d drag updates cutoff live" % slope)
	_expect(is_equal_approx(effect.resonance, 0.5 if slope == 0 else 0.8), "Slope %d drag respects resonance usage" % slope)
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 1, "Slope %d drag creates one undo action" % slope)
	history.undo()
	_expect(is_equal_approx(effect.cutoff_hz, 2000.0) and is_equal_approx(effect.resonance, 0.5), "Slope %d undo restores both parameters" % slope)
	history.redo()
	_expect(is_equal_approx(effect.cutoff_hz, 4000.0) and is_equal_approx(effect.resonance, 0.5 if slope == 0 else 0.8), "Slope %d redo restores both parameters" % slope)
	_expect(effect.gain == 2.0, "Slope %d drag preserves gain" % slope)
	_mouse_motion(preview, original, false)
	_expect(is_equal_approx(effect.cutoff_hz, 4000.0), "Slope %d motion after release does not edit" % slope)
	preview.read_only = true
	_mouse_button(preview, _position(preview, effect), true)
	_mouse_motion(preview, original, true)
	_mouse_button(preview, original, false)
	_expect(is_equal_approx(effect.cutoff_hz, 4000.0), "Slope %d read-only preview rejects input" % slope)
	preview.read_only = false
	_mouse_button(preview, _position(preview, effect), true)
	_mouse_motion(preview, Vector2(-40.0, preview.size.y + 40.0), true)
	_expect(effect.cutoff_hz == 1.0 and is_equal_approx(effect.resonance, 0.5 if slope == 0 else 0.0), "Slope %d drag clamps lower boundaries" % slope)
	_mouse_motion(preview, Vector2(preview.size.x + 40.0, -40.0), true)
	_mouse_button(preview, Vector2(preview.size.x + 40.0, -40.0), false)
	_expect(effect.cutoff_hz == 20500.0 and is_equal_approx(effect.resonance, 0.5 if slope == 0 else 1.0), "Slope %d drag clamps upper boundaries" % slope)
	history.clear_history()
	effect.cutoff_hz = 2000.0
	effect.resonance = 0.9
	await _settle(tree)


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _position(preview: Control, effect: FmodAudioEffectFilter) -> Vector2:
	return _position_for(preview, effect.cutoff_hz)


func _position_for(preview: Control, cutoff: float, resonance_delta: float = 0.0) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var plot: Rect2 = Rect2(Vector2(8.0, 8.0) * scale, preview.size - Vector2(16.0, 38.0) * scale)
	return plot.position + Vector2(log(cutoff) / log(22000.0) * plot.size.x, (24.0 / 84.0 - resonance_delta) * plot.size.y)


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


func _capture_if_requested(preview: Control, path: String, tree: SceneTree) -> Image:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return null
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = preview.get_viewport().get_texture().get_image()
	var area: Rect2i = Rect2i(preview.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if area.has_area():
		var cropped: Image = image.get_region(area)
		_expect(cropped.save_png(path) == OK, "Rendered Filter preview captured")
		return cropped
	return null


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("FILTER_INSPECTOR_CHECK_FAILED ", label)
