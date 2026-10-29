@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool


func run(output_dir: String, tree: SceneTree) -> bool:
	for class_name_value: StringName in [&"FmodAudioEffectEQ6", &"FmodAudioEffectEQ10", &"FmodAudioEffectEQ21",]:
		await _test_effect(class_name_value, output_dir, tree)
	var report: FileAccess = FileAccess.open(output_dir.path_join("eq_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_effect(native_class: StringName, output_dir: String, tree: SceneTree) -> void:
	var effect: FmodAudioEffectEQ = ClassDB.instantiate(native_class) as FmodAudioEffectEQ
	var count: int = effect.get_band_count()
	var band: int = count / 2
	var original_frequency: float = effect.get_band_frequency(band)
	effect.set_band_gain_db(band, 6.0)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectEQPreview", true, false)
	_expect(previews.size() == 1, "%s inserts one native preview" % native_class)
	if previews.size() != 1:
		return
	var preview: EditorProperty = previews[0] as EditorProperty
	var scale: float = EditorInterface.get_editor_scale()
	_expect(preview.size.y >= 160.0 * scale, "%s allocates preview height" % native_class)
	_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "%s preview has no reset button" % native_class)
	_expect(not preview.is_processing(), "%s redraws on demand" % native_class)
	_expect(effect.get_bus() == null, "%s opening preview does not attach audio DSP" % native_class)
	await _capture_if_requested(preview, output_dir.path_join(String(native_class) + ".png"), tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var target_frequency: float = original_frequency * 1.5
	var target_gain: float = 12.0
	var original: Vector2 = _position(preview, original_frequency, 6.0)
	var target: Vector2 = _position(preview, target_frequency, target_gain)
	_mouse_button(preview, original, true)
	_mouse_motion(preview, target, true)
	_expect(is_equal_approx(effect.get_band_frequency(band), target_frequency), "%s horizontal drag updates frequency live" % native_class)
	_expect(is_equal_approx(effect.get_band_gain_db(band), target_gain), "%s vertical drag updates gain live" % native_class)
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 1, "%s drag creates one undo action" % native_class)
	history.undo()
	_expect(is_equal_approx(effect.get_band_frequency(band), original_frequency) and is_equal_approx(effect.get_band_gain_db(band), 6.0), "%s one undo restores both parameters" % native_class)
	history.redo()
	_expect(is_equal_approx(effect.get_band_frequency(band), target_frequency) and is_equal_approx(effect.get_band_gain_db(band), target_gain), "%s redo restores both parameters" % native_class)
	_expect(is_equal_approx(float(effect.get("band_%d_frequency" % band)), target_frequency) and is_equal_approx(float(effect.get("band_%d_gain" % band)), target_gain), "%s native properties match dragged values" % native_class)
	_expect(effect.get_band_gain_db(0) == 0.0, "%s other bands remain unchanged" % native_class)
	_mouse_motion(preview, original, false)
	_expect(is_equal_approx(effect.get_band_frequency(band), target_frequency), "%s motion after release does not edit" % native_class)
	preview.read_only = true
	_mouse_button(preview, target, true)
	_mouse_motion(preview, original, true)
	_mouse_button(preview, original, false)
	_expect(is_equal_approx(effect.get_band_frequency(band), target_frequency), "%s read-only preview rejects input" % native_class)
	preview.read_only = false
	_mouse_button(preview, target, true)
	_mouse_motion(preview, Vector2(-40.0, preview.size.y + 40.0), true)
	_expect(effect.get_band_frequency(band) == 10.0 and effect.get_band_gain_db(band) == -60.0, "%s drag clamps lower boundaries" % native_class)
	_mouse_motion(preview, Vector2(preview.size.x + 40.0, -40.0), true)
	_mouse_button(preview, Vector2(preview.size.x + 40.0, -40.0), false)
	_expect(effect.get_band_frequency(band) == 22000.0 and effect.get_band_gain_db(band) == 24.0, "%s drag clamps upper boundaries" % native_class)
	history.clear_history()
	effect.set_band_frequency(band, original_frequency)
	effect.set_band_filter_type(band, FmodAudioEffectEQ.FILTER_LOWPASS_12DB)
	await _settle(tree)
	_mouse_button(preview, _position(preview, original_frequency, 0.0), true)
	_mouse_motion(preview, target, true)
	_mouse_button(preview, target, false)
	_expect(is_equal_approx(effect.get_band_frequency(band), target_frequency) and effect.get_band_gain_db(band) == 24.0, "%s pass filter drags frequency without changing unused gain" % native_class)
	await _capture_if_requested(preview, output_dir.path_join(String(native_class) + "_edited.png"), tree)
	history.clear_history()
	var connections: int = effect.get_signal_connection_list(&"changed").size()
	effect.preset = FmodAudioEffectEQ.PRESET_10_BANDS if count != 10 else FmodAudioEffectEQ.PRESET_6_BANDS
	await _settle(tree)
	var expected_count: int = 10 if count != 10 else 6
	_expect(effect.get_band_count() == expected_count, "%s preset updates band count" % native_class)
	_expect(EditorInterface.get_inspector().find_children("*", "FmodAudioEffectEQPreview", true, false).size() == 1, "%s preset rebuilds a single preview" % native_class)
	var path: String = output_dir.path_join(String(native_class) + ".tres")
	_expect(ResourceSaver.save(effect, path) == OK, "%s edited resource saves" % native_class)
	var restored: FmodAudioEffectEQ = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectEQ
	_expect(restored != null and restored.get_band_count() == expected_count, "%s saved resource restores its preset" % native_class)
	EditorInterface.edit_resource(FmodAudioEffectAmplify.new())
	await _settle(tree)
	_expect(effect.get_signal_connection_list(&"changed").size() < connections, "%s resource switch disconnects preview" % native_class)


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _position(preview: Control, frequency: float, gain: float) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var plot: Rect2 = Rect2(Vector2(8.0, 8.0) * scale, preview.size - Vector2(16.0, 38.0) * scale)
	return plot.position + Vector2(log(frequency / 10.0) / log(2200.0) * plot.size.x, (24.0 - gain) / 84.0 * plot.size.y)


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
	image.save_png(path.get_basename() + "_full.png")
	var area: Rect2i = Rect2i(preview.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if area.has_area():
		_expect(image.get_region(area).save_png(path) == OK, "Rendered EQ preview captured")


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("EQ_INSPECTOR_CHECK_FAILED ", label)
