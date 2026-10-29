@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectPitchShift = FmodAudioEffectPitchShift.new()
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectPitchShiftPreview", true, false)
	_expect(previews.size() == 1, "PitchShift inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "PitchShift declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "PitchShift preview has no reset button")
		_expect(not preview.is_processing() and effect.get_bus() == null, "PitchShift redraws on demand without attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS and not preview.mouse_force_pass_scroll_events, "PitchShift consumes dial scrolling and passes other input")
		var native_field_found: bool
		for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
			var property_editor: EditorProperty = node as EditorProperty
			if property_editor != preview and property_editor.get_edited_property() == &"pitch_scale":
				native_field_found = true
		_expect(native_field_found, "Native pitch field retains its property editor")
		effect.pitch_scale = 2.0
		_expect(_changes == 1, "Pitch change notifies preview")
		var positive: Image = await _capture_if_requested(preview, output_dir.path_join("pitch_shift.png"), tree)
		effect.pitch_scale = 2.0
		_expect(_changes == 1, "Unchanged pitch skips notification")
		effect.pitch_scale = 0.5
		var negative: Image = await _capture_if_requested(preview, output_dir.path_join("pitch_shift_negative.png"), tree)
		if positive != null and negative != null:
			_expect(positive.get_data() != negative.get_data(), "Positive and negative pitch update dial fill and readout")
		await _test_drag(preview, effect, tree)
		await _test_scroll(preview, effect, tree)
		for value: float in [0.01, 16.0,]:
			effect.pitch_scale = value
			await _capture_if_requested(preview, output_dir.path_join("pitch_shift_limit_%s.png" % value), tree)
			var semitones: float = 12.0 * log(value) / log(2.0)
			var dial_range: float = maxf(24.0, ceilf(absf(semitones) / 12.0 - 0.00001) * 12.0)
			_mouse_button(preview, _position(preview, semitones, dial_range), true)
			_mouse_button(preview, _position(preview, semitones, dial_range), false)
			_expect(is_equal_approx(effect.pitch_scale, value), "Expanded dial preserves pitch boundary %s on unmoved click" % value)
		effect.pitch_scale = 1.0
		await _capture_if_requested(preview, output_dir.path_join("pitch_shift_zero.png"), tree)
		preview.read_only = true
		_mouse_button(preview, _position(preview, 12.0), true)
		_mouse_motion(preview, _position(preview, -12.0), true)
		_mouse_button(preview, _position(preview, -12.0), false)
		_scroll(preview, MOUSE_BUTTON_WHEEL_UP)
		_expect(effect.pitch_scale == 1.0, "Read-only PitchShift rejects drag and scroll edits")
		preview.read_only = false
		effect.pitch_scale = 0.5
		var path: String = output_dir.path_join("pitch_shift_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited PitchShift saves")
		var restored: FmodAudioEffectPitchShift = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectPitchShift
		_expect(restored != null and restored.pitch_scale == 0.5, "Saved PitchShift restores pitch ratio")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectDelay.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects PitchShift preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("pitch_shift_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_drag(preview: EditorProperty, effect: FmodAudioEffectPitchShift, tree: SceneTree) -> void:
	effect.pitch_scale = 1.0
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	_mouse_button(preview, _position(preview, 0.0), true)
	_mouse_motion(preview, _position(preview, 12.0), true)
	_expect(is_equal_approx(effect.pitch_scale, 2.0), "Dial drag maps +12 semitones to pitch ratio 2 before release")
	_mouse_button(preview, _position(preview, 12.0), false)
	_expect(history.get_history_count() == 1, "Pitch drag creates one undo action")
	history.undo()
	_expect(is_equal_approx(effect.pitch_scale, 1.0), "Pitch drag supports undo")
	history.redo()
	_expect(is_equal_approx(effect.pitch_scale, 2.0), "Pitch drag supports redo")
	_mouse_motion(preview, _position(preview, -12.0), false)
	_expect(is_equal_approx(effect.pitch_scale, 2.0), "Motion after release does not change pitch")
	history.clear_history()
	_mouse_button(preview, _position(preview, 12.0), true)
	_mouse_button(preview, _position(preview, 12.0), false)
	_expect(is_equal_approx(effect.pitch_scale, 2.0) and history.get_history_count() == 0, "Unmoved marker click preserves pitch and undo history")
	_mouse_button(preview, _position(preview, -12.0), true)
	_mouse_button(preview, _position(preview, -12.0), false)
	_expect(is_equal_approx(effect.pitch_scale, 0.5), "Dial click maps -12 semitones to pitch ratio 0.5")
	history.clear_history()
	_mouse_button(preview, _position(preview, -12.0), true)
	_mouse_motion(preview, Vector2(-100.0, preview.size.y + 40.0), true)
	_expect(is_equal_approx(effect.pitch_scale, 0.25), "Drag below left endpoint clamps to displayed minimum")
	_mouse_motion(preview, Vector2(preview.size.x + 100.0, preview.size.y + 40.0), true)
	_mouse_button(preview, Vector2(preview.size.x + 100.0, preview.size.y + 40.0), false)
	_expect(is_equal_approx(effect.pitch_scale, 4.0), "Drag below right endpoint clamps to displayed maximum")
	_expect(history.get_history_count() == 1, "Endpoint drag remains one undo action")
	history.undo()
	_expect(is_equal_approx(effect.pitch_scale, 0.5), "Undo restores pitch before endpoint drag")
	history.clear_history()


func _test_scroll(preview: EditorProperty, effect: FmodAudioEffectPitchShift, tree: SceneTree) -> void:
	effect.pitch_scale = 1.0
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	_scroll(preview, MOUSE_BUTTON_WHEEL_UP)
	_expect(is_equal_approx(effect.pitch_scale, pow(2.0, 1.0 / 12.0)), "Scroll raises pitch by one semitone")
	_expect(history.get_history_count() == 1, "Pitch scroll creates one undo action")
	history.undo()
	_expect(is_equal_approx(effect.pitch_scale, 1.0), "Pitch scroll supports undo")
	history.redo()
	_expect(is_equal_approx(effect.pitch_scale, pow(2.0, 1.0 / 12.0)), "Pitch scroll supports redo")
	_scroll(preview, MOUSE_BUTTON_WHEEL_DOWN, true)
	_expect(is_equal_approx(effect.pitch_scale, pow(2.0, 0.9 / 12.0)), "Shift scroll lowers pitch by 0.1 semitone")
	effect.pitch_scale = 4.0
	_scroll(preview, MOUSE_BUTTON_WHEEL_UP)
	_expect(is_equal_approx(effect.pitch_scale, pow(2.0, 25.0 / 12.0)), "Scroll crosses two octaves and expands dial range")
	for value: float in [0.01, 16.0,]:
		effect.pitch_scale = value
		history.clear_history()
		var before: float = effect.pitch_scale
		_scroll(preview, MOUSE_BUTTON_WHEEL_DOWN if value < 1.0 else MOUSE_BUTTON_WHEEL_UP)
		_expect(effect.pitch_scale == before and history.get_history_count() == 0, "Scroll at API limit %s preserves pitch and undo history" % value)
	history.clear_history()


func _position(preview: Control, semitones: float, dial_range: float = 24.0) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var center: Vector2 = Vector2(preview.size.x * 0.5, preview.size.y - 12.0 * scale)
	var radius: float = minf(preview.size.x * 0.5 - 14.0 * scale, preview.size.y - 24.0 * scale) - 4.0 * scale
	var angle: float = (semitones / dial_range - 1.0) * PI * 0.5
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


func _scroll(preview: Control, button: MouseButton, fine: bool = false) -> void:
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = button
	event.pressed = true
	event.shift_pressed = fine
	event.factor = 1.0
	event.position = preview.get_global_transform_with_canvas() * Vector2(preview.size.x * 0.5, preview.size.y * 0.6)
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
	_expect(cropped.save_png(path) == OK, "Rendered PitchShift preview captured")
	return cropped


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("PITCH_SHIFT_INSPECTOR_CHECK_FAILED ", label)
