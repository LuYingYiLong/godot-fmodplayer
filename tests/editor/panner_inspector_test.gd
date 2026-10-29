@tool
extends RefCounted

var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectPanner = FmodAudioEffectPanner.new()
	var checks: Array[Dictionary]
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	for frame: int in range(4):
		await tree.process_frame
	var previews: Array[Node] = EditorInterface.get_inspector().find_children(
		"*", "FmodAudioEffectPannerPreview", true, false
	)
	checks.append({"name": "Native plugin inserts one Panner preview", "passed": previews.size() == 1,})
	if previews.size() == 1:
		var preview: Control = previews[0] as Control
		var minimum_height: float = 160.0 * EditorInterface.get_editor_scale()
		checks.append({
			"name": "Preview declares its scaled minimum height",
			"passed": preview.custom_minimum_size.y >= minimum_height,
			"actual": preview.custom_minimum_size.y,
			"expected": minimum_height,
		})
		checks.append({
			"name": "Container receives the preview minimum height",
			"passed": preview.get_combined_minimum_size().y >= minimum_height,
			"actual": preview.get_combined_minimum_size().y,
			"expected": minimum_height,
		})
		checks.append({
			"name": "Inspector allocates the full preview height",
			"passed": preview.size.y >= minimum_height,
			"actual": preview.size.y,
			"expected": minimum_height,
		})
		checks.append({"name": "Preview uses event-driven redraws", "passed": not preview.is_processing(),})
		checks.append({
			"name": "Preview accepts mouse input and passes scroll events",
			"passed": preview.mouse_filter == Control.MOUSE_FILTER_PASS and preview.mouse_force_pass_scroll_events,
		})
		for color: StringName in [
			&"dark_color_2", &"accent_color", &"font_color", &"separator_color", &"box_selection_fill_color",
		]:
			checks.append({"name": "Editor theme provides " + color, "passed": preview.has_theme_color(color, &"Editor"),})
		var original_connections: int = effect.get_signal_connection_list(&"changed").size()
		effect.pan = -0.5
		checks.append({"name": "Pan property emits changed", "passed": _changes == 1,})
		effect.set_pan(-0.5)
		checks.append({"name": "Unchanged pan skips notification", "passed": _changes == 1,})
		effect.set_pan(2.0)
		checks.append({"name": "Method clamps pan and emits changed", "passed": effect.pan == 1.0 and _changes == 2,})
		for frame: int in range(4):
			await tree.process_frame
		checks.append_array(await _test_mouse_editing(preview as EditorProperty, effect, tree))
		EditorInterface.edit_resource(FmodAudioEffectAmplify.new())
		for frame: int in range(4):
			await tree.process_frame
		checks.append({
			"name": "Changing inspected resource disconnects preview",
			"passed": effect.get_signal_connection_list(&"changed").size() < original_connections,
		})
		checks.append({
			"name": "Other effects keep their standard inspector",
			"passed": EditorInterface.get_inspector().find_children("*", "FmodAudioEffectPannerPreview", true, false).is_empty(),
		})
	var passed: bool = true
	for check: Dictionary in checks:
		passed = passed and bool(check["passed"])
		print("FMOD_PANNER_INSPECTOR_TEST ", check["name"], "=", check["passed"])
	var report: FileAccess = FileAccess.open(output_dir.path_join("panner_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": passed, "checks": checks,}, "\t"))
	return passed


func _test_mouse_editing(preview: EditorProperty, effect: FmodAudioEffectPanner, tree: SceneTree) -> Array[Dictionary]:
	var checks: Array[Dictionary]
	checks.append({
		"name": "Preview keeps the edited object without a per-property reset button",
		"passed": preview.get_edited_object() == effect and preview.get_edited_property().is_empty(),
	})
	var native_pan_editor_found: bool
	for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
		var property_editor: EditorProperty = node as EditorProperty
		if property_editor != preview and property_editor.get_edited_property() == &"pan":
			native_pan_editor_found = true
	checks.append({"name": "Native Pan editor retains its property and reset support", "passed": native_pan_editor_found,})
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	var before: int = history.get_history_count()
	_mouse_button(preview, preview.size.x * 0.5, true)
	_mouse_button(preview, preview.size.x * 0.5, false)
	checks.append({"name": "Click at center sets pan to zero", "passed": is_zero_approx(effect.pan),})
	checks.append({"name": "Click creates one undo action", "passed": history.get_history_count() == before + 1,})
	history.undo()
	checks.append({"name": "Click supports undo", "passed": effect.pan == 1.0,})
	history.redo()
	checks.append({"name": "Click supports redo", "passed": is_zero_approx(effect.pan),})
	# 独立检查整次拖动，避免与 Godot 的短时间连续属性编辑合并
	history.clear_history()
	var inset: float = 14.0 * EditorInterface.get_editor_scale()
	var quarter: float = inset + (preview.size.x - inset * 2.0) * 0.25
	_mouse_button(preview, preview.size.x * 0.5, true)
	_mouse_motion(preview, quarter, true)
	checks.append({"name": "Drag updates pan before mouse release", "passed": is_equal_approx(effect.pan, -0.5),})
	_mouse_motion(preview, -40.0, true)
	checks.append({"name": "Drag outside left edge clamps to -1", "passed": effect.pan == -1.0,})
	_mouse_motion(preview, preview.size.x + 40.0, true)
	checks.append({"name": "Drag outside right edge clamps to 1", "passed": effect.pan == 1.0,})
	_mouse_button(preview, preview.size.x + 40.0, false)
	checks.append({"name": "Whole drag creates one undo action", "passed": history.get_history_count() == 1,})
	history.undo()
	checks.append({"name": "One undo restores the value before dragging", "passed": is_zero_approx(effect.pan),})
	history.redo()
	checks.append({"name": "Redo restores the drag result", "passed": effect.pan == 1.0,})
	_mouse_motion(preview, quarter, false)
	checks.append({"name": "Mouse motion after release does not edit pan", "passed": effect.pan == 1.0,})
	preview.read_only = true
	_mouse_button(preview, quarter, true)
	_mouse_button(preview, quarter, false)
	checks.append({"name": "Read-only inspector prevents mouse edits", "passed": effect.pan == 1.0,})
	preview.read_only = false
	for frame: int in range(4):
		await tree.process_frame
	history.clear_history()
	return checks


func _mouse_button(preview: Control, x: float, pressed: bool) -> void:
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = pressed
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = preview.get_global_transform_with_canvas() * Vector2(x, preview.size.y * 0.5)
	event.global_position = event.position
	preview.get_viewport().push_input(event, true)


func _mouse_motion(preview: Control, x: float, pressed: bool) -> void:
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = preview.get_global_transform_with_canvas() * Vector2(x, preview.size.y * 0.5)
	event.global_position = event.position
	preview.get_viewport().push_input(event, true)


func _on_effect_changed() -> void:
	_changes += 1
