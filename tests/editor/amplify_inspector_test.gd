@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectAmplify = FmodAudioEffectAmplify.new()
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectAmplifyPreview", true, false)
	_expect(previews.size() == 1, "Amplify inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Amplify declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Amplify preview has no reset button")
		_expect(not preview.is_processing() and effect.get_bus() == null, "Amplify redraws on demand without attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS and preview.mouse_force_pass_scroll_events, "Amplify passes unhandled scroll events")
		var native_field_found: bool
		for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
			var property_editor: EditorProperty = node as EditorProperty
			if property_editor != preview and property_editor.get_edited_property() == &"volume_db":
				native_field_found = true
		_expect(native_field_found, "Native gain field retains its property editor")
		effect.volume_db = 6.0
		_expect(_changes == 1, "Gain changes notify the preview")
		await _capture_if_requested(preview, output_dir.path_join("amplify.png"), tree)
		effect.volume_db = 6.0
		_expect(_changes == 1, "Unchanged gain skips notification")
		effect.set_volume_linear(2.0)
		_expect(_changes == 2 and is_equal_approx(effect.get_volume_linear(), 2.0), "Linear gain updates dB and notifies the preview")
		effect.set_volume_linear(2.0)
		_expect(_changes == 2, "Unchanged linear gain skips notification")
		await _test_drag(preview, effect, tree)
		for gain: float in [-80.0, 0.0, 24.0,]:
			effect.volume_db = gain
			await _settle(tree)
			var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
			var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
			history.clear_history()
			var position: Vector2 = _position(preview, gain)
			_mouse_button(preview, position, true)
			_mouse_button(preview, position, false)
			_expect(effect.volume_db == gain and history.get_history_count() == 0, "Unmoved click preserves %.1f dB and undo history" % gain)
		effect.volume_db = 0.0
		await _capture_if_requested(preview, output_dir.path_join("amplify_unity.png"), tree)
		preview.read_only = true
		_mouse_button(preview, _position(preview, 0.0), true)
		_mouse_motion(preview, _position(preview, 12.0), true)
		_mouse_button(preview, _position(preview, 12.0), false)
		_expect(effect.volume_db == 0.0, "Read-only Amplify rejects mouse edits")
		preview.read_only = false
		effect.set_volume_linear(0.0)
		_expect(effect.get_volume_linear() == 0.0 and effect.volume_db == -60.0, "Zero linear gain keeps existing silence conversion")
		await _capture_if_requested(preview, output_dir.path_join("amplify_silence.png"), tree)
		effect.volume_db = -12.0
		await _capture_if_requested(preview, output_dir.path_join("amplify_attenuation.png"), tree)
		var path: String = output_dir.path_join("amplify_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Amplify saves")
		var restored: FmodAudioEffectAmplify = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectAmplify
		_expect(restored != null and restored.volume_db == -12.0, "Saved Amplify restores gain")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectDelay.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Amplify preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("amplify_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_drag(preview: EditorProperty, effect: FmodAudioEffectAmplify, tree: SceneTree) -> void:
	effect.volume_db = 0.0
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var target: Vector2 = _position(preview, 12.0)
	_mouse_button(preview, _position(preview, 0.0), true)
	_mouse_motion(preview, target, true)
	_expect(is_equal_approx(effect.volume_db, 12.0), "Amplify drag updates gain before release")
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 1, "Amplify drag creates one undo action")
	history.undo()
	_expect(is_zero_approx(effect.volume_db), "Amplify drag supports undo")
	history.redo()
	_expect(is_equal_approx(effect.volume_db, 12.0), "Amplify drag supports redo")
	_mouse_motion(preview, _position(preview, -24.0), false)
	_expect(is_equal_approx(effect.volume_db, 12.0), "Motion after release does not change gain")
	history.clear_history()
	_mouse_button(preview, _position(preview, effect.volume_db), true)
	_mouse_motion(preview, _position(preview, effect.volume_db) - Vector2(40.0, 0.0), true)
	_mouse_button(preview, _position(preview, effect.volume_db) - Vector2(40.0, 0.0), false)
	_expect(is_equal_approx(effect.volume_db, 12.0) and history.get_history_count() == 0, "Horizontal drag preserves gain and undo history")
	_mouse_button(preview, _position(preview, effect.volume_db), true)
	_mouse_motion(preview, Vector2(target.x, preview.size.y * 2.0), true)
	_expect(effect.volume_db == -80.0, "Amplify drag clamps at -80 dB")
	_mouse_motion(preview, Vector2(target.x, -preview.size.y), true)
	_mouse_button(preview, Vector2(target.x, -preview.size.y), false)
	_expect(effect.volume_db == 24.0, "Amplify drag clamps at +24 dB")
	_expect(history.get_history_count() == 1, "Boundary drag remains one undo action")
	history.undo()
	_expect(is_equal_approx(effect.volume_db, 12.0), "Undo restores gain before boundary drag")
	history.clear_history()


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _position(preview: Control, gain: float) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var left: float = maxf((preview.size.x - 88.0 * scale) * 0.5, 8.0 * scale)
	var width: float = minf(32.0 * scale, maxf(preview.size.x - 64.0 * scale, 0.0))
	var plot: Rect2 = Rect2(Vector2(left, 12.0 * scale), Vector2(width, preview.size.y - 24.0 * scale))
	return plot.position + Vector2(plot.size.x * 0.5, (24.0 - gain) / 104.0 * plot.size.y)


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


func _capture_if_requested(preview: Control, path: String, tree: SceneTree) -> void:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = preview.get_viewport().get_texture().get_image()
	var area: Rect2i = Rect2i(preview.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if area.has_area():
		_expect(image.get_region(area).save_png(path) == OK, "Rendered Amplify preview captured")


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("AMPLIFY_INSPECTOR_CHECK_FAILED ", label)
