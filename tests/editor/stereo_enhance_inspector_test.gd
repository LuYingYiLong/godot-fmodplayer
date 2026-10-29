@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectStereoEnhance = FmodAudioEffectStereoEnhance.new()
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectStereoEnhancePreview", true, false)
	_expect(previews.size() == 1, "StereoEnhance inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "StereoEnhance declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "StereoEnhance preview has no reset button")
		_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty() and effect.get_bus() == null, "StereoEnhance caches geometry without polling or attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "StereoEnhance passes unhandled scroll events")
		var native_fields: Array[StringName]
		for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
			var property_editor: EditorProperty = node as EditorProperty
			if property_editor != preview:
				native_fields.append(property_editor.get_edited_property())
		_expect(&"pan_pullout" in native_fields and &"time_pullout_ms" in native_fields and &"surround" in native_fields, "Native stereo parameters retain their property editors")
		var initial: Image = await _capture_if_requested(preview, output_dir.path_join("stereo_enhance.png"), tree)
		_set_parameters(effect)
		_expect(_changes == 3, "All stereo parameters notify preview")
		_set_parameters(effect)
		_expect(_changes == 3, "Unchanged stereo parameters skip notification")
		var updated: Image = await _capture_if_requested(preview, output_dir.path_join("stereo_enhance_updated.png"), tree)
		if initial != null and updated != null:
			_expect(initial.get_data() != updated.get_data(), "Stereo parameters update radar sector and delay arc")
		effect.set_time_pullout(0.0)
		effect.surround = 0.0
		var plain: Image = await _capture_if_requested(preview, output_dir.path_join("stereo_enhance_plain.png"), tree)
		effect.set_time_pullout(25.0)
		var delayed: Image = await _capture_if_requested(preview, output_dir.path_join("stereo_enhance_delay.png"), tree)
		if plain != null and delayed != null:
			_expect(plain.get_data() != delayed.get_data(), "Time Pullout adds the inner delay arc")
		effect.surround = 1.0
		var surround: Image = await _capture_if_requested(preview, output_dir.path_join("stereo_enhance_surround.png"), tree)
		if delayed != null and surround != null:
			_expect(delayed.get_data() != surround.get_data(), "Surround changes sector fill strength")
		await _test_drag(preview, effect, tree)
		effect.pan_pullout = -1.0
		_expect(effect.pan_pullout == 0.0, "Stereo width clamps to zero")
		await _capture_if_requested(preview, output_dir.path_join("stereo_enhance_zero.png"), tree)
		effect.pan_pullout = 8.0
		effect.time_pullout_ms = 80.0
		effect.surround = 2.0
		_expect(effect.pan_pullout == 4.0 and effect.time_pullout_ms == 50.0 and effect.surround == 1.0, "Stereo parameters clamp to upper limits")
		await _capture_if_requested(preview, output_dir.path_join("stereo_enhance_max.png"), tree)
		var path: String = output_dir.path_join("stereo_enhance_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited StereoEnhance saves")
		var restored: FmodAudioEffectStereoEnhance = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectStereoEnhance
		_expect(restored != null and restored.pan_pullout == 4.0 and restored.time_pullout_ms == 50.0 and restored.surround == 1.0, "Saved StereoEnhance restores its parameters")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectDelay.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects StereoEnhance preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("stereo_enhance_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _set_parameters(effect: FmodAudioEffectStereoEnhance) -> void:
	effect.pan_pullout = 1.5
	effect.set_time_pullout(12.5)
	effect.surround = 0.5


func _test_drag(preview: EditorProperty, effect: FmodAudioEffectStereoEnhance, tree: SceneTree) -> void:
	effect.pan_pullout = 1.0
	effect.set_time_pullout(12.5)
	effect.surround = 0.5
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	_mouse_button(preview, _position(preview, 1.0), true)
	_mouse_motion(preview, _position(preview, 2.0), true)
	_expect(is_equal_approx(effect.pan_pullout, 2.0), "Radar drag updates stereo width before release")
	_mouse_button(preview, _position(preview, 2.0), false)
	_expect(history.get_history_count() == 1, "Radar drag creates one undo action")
	history.undo()
	_expect(is_equal_approx(effect.pan_pullout, 1.0), "Radar width supports undo")
	history.redo()
	_expect(is_equal_approx(effect.pan_pullout, 2.0), "Radar width supports redo")
	_mouse_motion(preview, _position(preview, 0.5), false)
	_expect(is_equal_approx(effect.pan_pullout, 2.0), "Motion after release preserves width")
	history.clear_history()
	_mouse_button(preview, _position(preview, 2.0, -1.0), true)
	_mouse_button(preview, _position(preview, 2.0, -1.0), false)
	_expect(is_equal_approx(effect.pan_pullout, 2.0) and history.get_history_count() == 0, "Unmoved click on symmetric left edge preserves width and undo history")
	_mouse_button(preview, _position(preview, 3.0, -1.0), true)
	_mouse_button(preview, _position(preview, 3.0, -1.0), false)
	_expect(is_equal_approx(effect.pan_pullout, 3.0), "Left side radar click sets stereo width symmetrically")
	_expect(effect.time_pullout_ms == 12.5 and effect.surround == 0.5, "Radar width edits preserve delay and surround")
	history.clear_history()
	_mouse_button(preview, _position(preview, 3.0), true)
	_mouse_motion(preview, Vector2(preview.size.x + 100.0, preview.size.y + 40.0), true)
	_expect(effect.pan_pullout == 4.0, "Drag below the radar clamps to maximum width")
	_mouse_motion(preview, Vector2(preview.size.x * 0.5, -100.0), true)
	_mouse_button(preview, Vector2(preview.size.x * 0.5, -100.0), false)
	_expect(effect.pan_pullout == 0.0, "Vertical drag to radar centerline closes sector")
	_expect(history.get_history_count() == 1, "Boundary drag remains one undo action")
	history.undo()
	_expect(is_equal_approx(effect.pan_pullout, 3.0), "Boundary drag undo restores original width")
	history.clear_history()
	var center: Vector2 = Vector2(preview.size.x * 0.5, preview.size.y - 12.0 * EditorInterface.get_editor_scale())
	_mouse_button(preview, center, true)
	_mouse_motion(preview, _position(preview, 0.5), true)
	_mouse_button(preview, _position(preview, 0.5), false)
	_expect(is_equal_approx(effect.pan_pullout, 3.0) and history.get_history_count() == 0, "Click at radar origin avoids ambiguous angular edits")
	preview.read_only = true
	_mouse_button(preview, _position(preview, 0.5), true)
	_mouse_motion(preview, _position(preview, 1.0), true)
	_mouse_button(preview, _position(preview, 1.0), false)
	_expect(is_equal_approx(effect.pan_pullout, 3.0), "Read-only StereoEnhance rejects mouse edits")
	preview.read_only = false


func _position(preview: Control, width: float, side: float = 1.0) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var center: Vector2 = Vector2(preview.size.x * 0.5, preview.size.y - 12.0 * scale)
	var radius: float = minf(preview.size.x * 0.5 - 14.0 * scale, preview.size.y - 24.0 * scale) * 0.85
	var angle: float = -PI * 0.5 + width * PI * 0.125 * side
	return center + Vector2(cos(angle), sin(angle)) * radius


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
	_expect(cropped.save_png(path) == OK, "Rendered StereoEnhance preview captured")
	return cropped


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("STEREO_ENHANCE_INSPECTOR_CHECK_FAILED ", label)
