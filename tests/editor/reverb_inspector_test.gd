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
