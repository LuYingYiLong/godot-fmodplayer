@tool
extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool
var _changes: int


func run(output_dir: String, tree: SceneTree) -> bool:
	var effect: FmodAudioEffectChorus = FmodAudioEffectChorus.new()
	effect.set_voice_depth_ms(0, 12.0)
	effect.set_voice_depth_ms(1, 6.0)
	effect.set_voice_pan(0, -0.6)
	effect.set_voice_pan(1, 0.7)
	effect.changed.connect(_on_effect_changed)
	EditorInterface.edit_resource(effect)
	await _settle(tree)
	var preview: EditorProperty = _find_preview()
	_expect(preview != null, "Chorus inserts one native preview")
	if preview != null:
		_expect(is_equal_approx(preview.custom_minimum_size.y, 160.0 * EditorInterface.get_editor_scale()) and preview.size.y >= preview.custom_minimum_size.y, "Chorus declares and receives 160 scaled pixels of height")
		_expect(preview.get_edited_object() == effect and preview.get_edited_property().is_empty(), "Chorus preview has no reset button")
		_expect(not preview.is_processing() and preview.find_children("*", "Timer", true, false).is_empty(), "Chorus redraws on demand without animation polling")
		_expect(effect.get_bus() == null, "Opening Chorus preview does not attach DSP")
		_expect(preview.mouse_filter == Control.MOUSE_FILTER_PASS, "Chorus passes unhandled input")
		var initial: Image = await _capture_if_requested(preview, output_dir.path_join("chorus.png"), tree)
		_set_voice_parameters(effect)
		_expect(_changes == 8, "Chorus parameter changes notify the preview")
		_set_voice_parameters(effect)
		_expect(_changes == 8, "Unchanged Chorus parameters skip notification")
		await _settle(tree)
		var updated: Image = await _capture_if_requested(preview, output_dir.path_join("chorus_updated.png"), tree)
		if initial != null and updated != null:
			_expect(initial.get_data() != updated.get_data(), "Chorus parameters update rendered curves")
		effect.voice_count = 4
		await _settle(tree)
		preview = _find_preview()
		_expect(preview != null and effect.voice_count == 4, "Four voices rebuild a single Chorus preview")
		if preview != null:
			effect.set_voice_depth_ms(2, 16.0)
			effect.set_voice_rate_hz(2, 1.6)
			effect.set_voice_pan(2, 0.15)
			effect.set_voice_depth_ms(3, 8.0)
			effect.set_voice_rate_hz(3, 0.6)
			effect.set_voice_pan(3, 0.9)
			var four_voices: Image = await _capture_if_requested(preview, output_dir.path_join("chorus_four.png"), tree)
			preview.add_theme_color_override(&"voice_1_color", Color(1.0, 0.4, 0.1))
			var custom: Image = await _capture_if_requested(preview, output_dir.path_join("chorus_custom.png"), tree)
			if four_voices != null and custom != null:
				_expect(four_voices.get_data() != custom.get_data(), "Custom voice color updates rendered preview")
			effect.set_voice_delay_ms(0, 50.0)
			effect.set_voice_rate_hz(0, 20.0)
			effect.set_voice_depth_ms(0, 20.0)
			effect.set_voice_pan(0, -1.0)
			await _capture_if_requested(preview, output_dir.path_join("chorus_limits.png"), tree)
		effect.voice_count = 1
		await _settle(tree)
		preview = _find_preview()
		_expect(preview != null and effect.voice_count == 1, "One voice rebuilds a single Chorus preview")
		var path: String = output_dir.path_join("chorus_preview.tres")
		_expect(ResourceSaver.save(effect, path) == OK, "Edited Chorus saves")
		var restored: FmodAudioEffectChorus = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as FmodAudioEffectChorus
		_expect(restored != null and restored.voice_count == 1 and restored.get_voice_rate_hz(0) == 20.0 and restored.get_voice_depth_ms(0) == 20.0 and restored.get_voice_pan(0) == -1.0, "Saved Chorus restores voice parameters")
		var connections: int = effect.get_signal_connection_list(&"changed").size()
		EditorInterface.edit_resource(FmodAudioEffectDelay.new())
		await _settle(tree)
		_expect(effect.get_signal_connection_list(&"changed").size() < connections, "Resource switch disconnects Chorus preview")
	var report: FileAccess = FileAccess.open(output_dir.path_join("chorus_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _set_voice_parameters(effect: FmodAudioEffectChorus) -> void:
	effect.wet = 0.7
	effect.dry = 0.8
	effect.set_voice_delay_ms(0, 18.0)
	effect.set_voice_rate_hz(0, 2.0)
	effect.set_voice_depth_ms(0, 8.0)
	effect.set_voice_level_db(0, -3.0)
	effect.set_voice_cutoff_hz(0, 4000.0)
	effect.set_voice_pan(0, -0.75)


func _find_preview() -> EditorProperty:
	var previews: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioEffectChorusPreview", true, false)
	return previews[0] as EditorProperty if previews.size() == 1 else null


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
	_expect(cropped.save_png(path) == OK, "Rendered Chorus preview captured")
	return cropped


func _on_effect_changed() -> void:
	_changes += 1


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("CHORUS_INSPECTOR_CHECK_FAILED ", label)
