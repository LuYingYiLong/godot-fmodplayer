@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectCompressor = FmodAudioEffectCompressor.new()
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectCompressorPreview", true, false)
	_expect(previews.size() == 1, "Compressor inserts one native preview")
	if previews.size() == 1:
		var preview: EditorProperty = previews[0] as EditorProperty
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Compressor declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Compressor preview has no reset button")
		_expect(not preview.is_processing() and effect.get_bus() == null, "Compressor redraws on demand without attaching DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "Compressor passes unhandled wheel events")
		effect.threshold_db = -24.0
		effect.ratio = 8.0
		effect.gain_db = 6.0
		effect.mix = 0.5
		_expect(_changes == 4, "Threshold, ratio, gain and mix notify the preview")
		effect.threshold_db = -24.0
		effect.ratio = 8.0
		effect.gain_db = 6.0
		effect.mix = 0.5
		_expect(_changes == 4, "Unchanged compressor parameters skip notification")
		effect.gain_db = 0.0
		effect.mix = 1.0
		effect.ratio = 4.0
		await _settle(tree)
		await _capture_if_requested(preview, output_dir.path_join("compressor.png"), tree)
		await _test_threshold(preview, effect, tree)
		for mix_value: float in [1.0, 0.5, 0.0,]:
			await _test_ratio(preview, effect, mix_value, tree)
		effect.threshold_db = 0.0
		effect.ratio = 4.0
		effect.mix = 1.0
		await _settle(tree)
		var shared: Vector2 = _ratio_position(preview, effect)
		var ratio_target: Vector2 = shared + Vector2(0.0, (preview.size.y - 24.0 * EditorInterface.get_editor_scale()) * log(2.0) / log(48.0))
		_mouse_button(preview, shared, true)
		_mouse_motion(preview, ratio_target, true)
		_mouse_button(preview, ratio_target, false)
		_expect(absf(effect.ratio - 8.0) < 0.01, "Ratio remains editable at default 0 dB threshold")
		_mouse_button(preview, _threshold_position(preview, effect), true)
		var threshold_target: Vector2 = _threshold_position(preview, effect) - Vector2((preview.size.x - 64.0 * EditorInterface.get_editor_scale()) * 0.25, 0.0)
		_mouse_motion(preview, threshold_target, true)
		_mouse_button(preview, threshold_target, false)
		_expect(is_equal_approx(effect.threshold_db, -20.0) and absf(effect.ratio - 8.0) < 0.01, "Merged handles still allow horizontal threshold dragging")
		preview.read_only = true
		var original_threshold: float = effect.threshold_db
		var original_ratio: float = effect.ratio
		_mouse_button(preview, _threshold_position(preview, effect), true)
		_mouse_motion(preview, _point(preview, -40.0, 0.0), true)
		_mouse_button(preview, _point(preview, -40.0, 0.0), false)
		_mouse_button(preview, _ratio_position(preview, effect), true)
		_mouse_motion(preview, Vector2(preview.size.x - 8.0, -40.0), true)
		_mouse_button(preview, Vector2(preview.size.x - 8.0, -40.0), false)
		_expect(effect.threshold_db == original_threshold and effect.ratio == original_ratio, "Read-only Compressor rejects both handle edits")
		preview.read_only = false
		effect.threshold_db = -24.0
		effect.ratio = 4.0
		effect.gain_db = 6.0
		effect.mix = 0.5
		await _settle(tree)
		await _capture_if_requested(preview, output_dir.path_join("compressor_mixed.png"), tree)
		var path: String = output_dir.path_join("compressor_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Compressor saves")
		var restored: FmodAudioEffectCompressor = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectCompressor
		_expect(restored != null and restored.threshold_db == -24.0 and restored.ratio == 4.0 and restored.gain_db == 6.0 and restored.mix == 0.5, "Saved Compressor restores edited curve parameters")
		var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
		manager.get_history_undo_redo(manager.get_object_history_id(effect)).clear_history()
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectAmplify.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Compressor preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("compressor_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_threshold(preview: EditorProperty, effect: FmodAudioEffectCompressor, tree: SceneTree) -> void:
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var target: Vector2 = _point(preview, -36.0, 10.0)
	_mouse_button(preview, _threshold_position(preview, effect), true)
	_mouse_motion(preview, target, true)
	_expect(is_equal_approx(effect.threshold_db, -36.0) and effect.ratio == 4.0, "Threshold handle edits threshold live and preserves ratio")
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 1, "Threshold drag creates one undo action")
	history.undo()
	_expect(is_equal_approx(effect.threshold_db, -24.0), "Threshold drag supports undo")
	history.redo()
	_expect(is_equal_approx(effect.threshold_db, -36.0), "Threshold drag supports redo")
	_mouse_motion(preview, _point(preview, -50.0, 0.0), false)
	_expect(is_equal_approx(effect.threshold_db, -36.0), "Motion after release does not edit threshold")
	_mouse_button(preview, _threshold_position(preview, effect), true)
	_mouse_motion(preview, Vector2(-40.0, 0.0), true)
	_expect(effect.threshold_db == -60.0, "Threshold clamps at -60 dB")
	_mouse_motion(preview, Vector2(preview.size.x + 40.0, 0.0), true)
	_mouse_button(preview, Vector2(preview.size.x + 40.0, 0.0), false)
	_expect(effect.threshold_db == 0.0, "Threshold clamps at 0 dB")
	history.clear_history()
	_mouse_button(preview, _threshold_position(preview, effect), true)
	_mouse_button(preview, _threshold_position(preview, effect), false)
	_expect(history.get_history_count() == 0, "Unmoved threshold click does not create an undo action")
	effect.threshold_db = -24.0
	await _settle(tree)


func _test_ratio(preview: EditorProperty, effect: FmodAudioEffectCompressor, mix_value: float, tree: SceneTree) -> void:
	effect.ratio = 4.0
	effect.mix = mix_value
	effect.gain_db = 6.0 if mix_value == 0.5 else 0.0
	await _settle(tree)
	var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
	var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(effect))
	history.clear_history()
	var original: Vector2 = _ratio_position(preview, effect)
	var target: Vector2 = _ratio_position(preview, effect, 8.0)
	if mix_value == 0.0:
		var plot_height: float = preview.size.y - 24.0 * EditorInterface.get_editor_scale()
		target = original + Vector2(0.0, plot_height * log(2.0) / log(48.0))
	_mouse_button(preview, original, true)
	_mouse_motion(preview, target, true)
	_expect(absf(effect.ratio - 8.0) < 0.01, "Mix %.1f ratio handle edits ratio live" % mix_value)
	_mouse_button(preview, target, false)
	_expect(history.get_history_count() == 1, "Mix %.1f ratio drag creates one undo action" % mix_value)
	history.undo()
	_expect(absf(effect.ratio - 4.0) < 0.01, "Mix %.1f ratio drag supports undo" % mix_value)
	history.redo()
	_expect(absf(effect.ratio - 8.0) < 0.01, "Mix %.1f ratio drag supports redo" % mix_value)
	_expect(effect.threshold_db == -24.0 and effect.mix == mix_value and effect.gain_db == (6.0 if mix_value == 0.5 else 0.0), "Mix %.1f ratio drag preserves other parameters" % mix_value)
	_mouse_button(preview, _ratio_position(preview, effect), true)
	_mouse_motion(preview, Vector2(original.x, original.y - preview.size.y * 2.0), true)
	_expect(effect.ratio == 1.0, "Mix %.1f ratio clamps at 1:1" % mix_value)
	_mouse_motion(preview, Vector2(original.x, original.y + preview.size.y * 2.0), true)
	_mouse_button(preview, Vector2(original.x, original.y + preview.size.y * 2.0), false)
	_expect(effect.ratio == 48.0, "Mix %.1f ratio clamps at 48:1" % mix_value)
	history.clear_history()
	var position: Vector2 = _ratio_position(preview, effect)
	_mouse_button(preview, position, true)
	_mouse_button(preview, position, false)
	_expect(effect.ratio == 48.0 and history.get_history_count() == 0, "Mix %.1f unmoved ratio click preserves value and history" % mix_value)
	history.clear_history()


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _output(input_db: float, effect: FmodAudioEffectCompressor, ratio: float = -1.0) -> float:
	var value: float = effect.ratio if ratio < 0.0 else ratio
	var compressed: float = input_db if input_db <= effect.threshold_db else effect.threshold_db + (input_db - effect.threshold_db) / value
	var amplitude: float = (1.0 - effect.mix) * pow(10.0, input_db / 20.0) + effect.mix * pow(10.0, (compressed + effect.gain_db) / 20.0)
	return 20.0 * log(amplitude) / log(10.0)


func _point(preview: Control, input_db: float, output_db: float) -> Vector2:
	var scale: float = EditorInterface.get_editor_scale()
	var plot: Rect2 = Rect2(Vector2(8.0, 12.0) * scale, preview.size - Vector2(64.0, 24.0) * scale)
	return plot.position + Vector2((input_db + 80.0) / 80.0 * plot.size.x, (20.0 - output_db) / 100.0 * plot.size.y)


func _threshold_position(preview: Control, effect: FmodAudioEffectCompressor) -> Vector2:
	return _point(preview, effect.threshold_db, _output(effect.threshold_db, effect))


func _ratio_position(preview: Control, effect: FmodAudioEffectCompressor, ratio: float = -1.0) -> Vector2:
	return _point(preview, 0.0, _output(0.0, effect, ratio))


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
		_expect(image.get_region(area).save_png(path) == OK, "Rendered Compressor preview captured")


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("COMPRESSOR_INSPECTOR_CHECK_FAILED ", label)
