@tool
extends RefCounted

const METER_TEST: Script = preload("uid://de8my4rx6sw2q")

var _checks: Array[Dictionary]
var _failed: bool


func run_startup(output_dir: String, tree: SceneTree) -> bool:
	return await METER_TEST.new().run_startup(output_dir, tree)


func run(output_dir: String, tree: SceneTree) -> bool:
	var candidates: Array[Node] = EditorInterface.get_base_control().find_children("*", "FmodAudioBusesDock", true, false)
	_expect(candidates.size() == 1, "One native FMOD Dock is registered")
	if candidates.size() != 1:
		return false
	var dock: EditorDock = candidates[0] as EditorDock
	# 无场景的隔离项目尚未保存底栏高度，展开后测试完整面板
	dock.custom_minimum_size.y = 440.0 * EditorInterface.get_editor_scale()
	_expect(dock.default_slot == EditorDock.DOCK_SLOT_BOTTOM, "Default Dock slot matches Godot Audio")
	_expect(dock.available_layouts == (EditorDock.DOCK_LAYOUT_HORIZONTAL | EditorDock.DOCK_LAYOUT_FLOATING), "Native framework owns docking and floating")
	dock.make_visible()
	await _settle(tree)
	var body: MarginContainer = dock.find_child("Body", true, false) as MarginContainer
	var bus_panel: ScrollContainer = body.get_node("BusPanel") as ScrollContainer
	var hint: Label = body.get_node("SyncHint") as Label
	_expect(body.theme_type_variation == &"NoBorderBottomPanel", "Body keeps the native bottom-panel MarginContainer")
	_expect(bus_panel.get_theme_stylebox(&"panel") == dock.get_theme_stylebox(&"panel", &"Tree"), "Bus panel uses the native Tree background and inner margins")
	_expect(not (_button(dock, "FMOD") as MenuButton).flat, "Header FMOD menu uses the regular button style")
	await _check_native_panel(dock, body, bus_panel, output_dir, tree)
	var domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.editor")
	_expect(domain.translate(&"FMOD Audio") == &"FMOD 音频", "Panel translations are registered in godot.editor")
	var properties: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.properties")
	_expect(properties.translate(&"Sync Godot Audio Buses") == &"同步 Godot 音频总线", "Project setting translations use godot.properties")
	var layout: FmodAudioBusLayout = dock.call(&"get_layout") as FmodAudioBusLayout
	var rows: Array[Node] = dock.find_children("*", "FmodAudioBusEditor", true, false)
	_expect(rows.size() == layout.get_bus_count(), "Dock shows the active layout")
	var master: Control = _row(dock, 0)
	_expect(master.get_theme_stylebox(&"panel") == master.get_theme_stylebox(&"master", &"EditorAudioBus"), "Master uses the native EditorAudioBus theme")
	var add: Button = _button(dock, "Add Bus")
	var slider: VSlider = master.find_child("Volume", true, false) as VSlider
	_expect(slider.min_value == 0.0 and slider.max_value == 1.0 and is_equal_approx(slider.step, 0.0001), "Volume slider matches the native normalized range")
	if FmodServer.is_syncing_godot_buses():
		_expect(not bus_panel.visible and hint.visible, "Synchronization shows only the hint in the body")
		_expect(hint.horizontal_alignment == HORIZONTAL_ALIGNMENT_CENTER and hint.vertical_alignment == VERTICAL_ALIGNMENT_CENTER, "Synchronization hint is centered in both directions")
		_expect(hint.get_global_rect().get_center().distance_to(body.get_global_rect().get_center()) <= 1.0, "Synchronization hint occupies the body center")
		_expect(add.disabled and not slider.editable, "Godot synchronization makes the mirrored controls read-only")
		var converted: FmodAudioBusLayout = FmodServer.create_layout_from_godot()
		_expect(converted.validate() == OK, "Conversion creates a standalone valid configuration")
		await _check_sync(layout, tree)
		await _capture(dock, output_dir.path_join("fmod_bus_dock_sync.png"), tree)
	else:
		_expect(bus_panel.visible and not hint.visible, "Independent mode shows the bus panel")
		_expect(not add.disabled and slider.editable, "Independent layouts are editable")
		_expect(await METER_TEST.new().run(dock, output_dir, tree), "Real playback, transient peaks, colors and meter fall behavior pass")
		await _settle(tree)
		var before: Array = layout.get_state()
		var undo: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
		var history: UndoRedo = undo.get_history_undo_redo(undo.get_object_history_id(layout))
		history.clear_history()
		add.emit_signal(&"pressed")
		await _settle(tree)
		_expect(layout.get_bus_count() == before.size() + 1 and history.get_history_count() == 1, "Add Bus is one undo action")
		var id: int = layout.get_bus_ids()[-1]
		var row: Control = _row(dock, id)
		slider = row.find_child("Volume", true, false) as VSlider
		(row.get_parent().get_parent() as ScrollContainer).ensure_control_visible(row)
		await _settle(tree)
		await _capture(dock, output_dir.path_join("fmod_bus_dock_before_drag.png"), tree)
		history.clear_history()
		var initial_volume: float = layout.get_bus_volume_db(id)
		_mouse_motion(slider, slider.size * Vector2(0.5, 0.35))
		_mouse_button(slider, slider.size * Vector2(0.5, 0.35), true)
		_mouse_motion(slider, slider.size * Vector2(0.5, 0.60))
		_mouse_button(slider, slider.size * Vector2(0.5, 0.60), false)
		await _settle(tree)
		_expect(not is_equal_approx(layout.get_bus_volume_db(id), initial_volume), "Dragging the native slider updates volume")
		_expect(history.get_history_count() == 1, "A complete volume drag creates one undo action")
		history.undo()
		await _settle(tree)
		_expect(is_equal_approx(layout.get_bus_volume_db(id), initial_volume), "Undo restores the volume before the drag")
		history.redo()
		await _settle(tree)
		var menus: Array[Node] = row.find_children("*", "PopupMenu", true, false)
		var effect_menu: PopupMenu
		for node: Node in menus:
			var candidate: PopupMenu = node as PopupMenu
			if candidate.item_count > 0 and str(candidate.get_item_metadata(0)).begins_with("FmodAudioEffect"):
				effect_menu = candidate
		_expect(effect_menu != null, "Native popup lists FMOD effect classes")
		if effect_menu != null:
			for index: int in range(effect_menu.item_count):
				if effect_menu.get_item_metadata(index) == "FmodAudioEffectAmplify":
					effect_menu.emit_signal(&"index_pressed", index)
					break
		await _settle(tree)
		var slots: PackedInt64Array = layout.get_bus_effect_ids(id)
		_expect(slots.size() == 1, "Add Effect inserts an enabled slot")
		var effect_tree: Tree = row.find_child("Effects", true, false) as Tree
		var cell: Rect2 = effect_tree.get_item_area_rect(effect_tree.get_root().get_first_child())
		_mouse_button(effect_tree, cell.position + Vector2(70.0 * EditorInterface.get_editor_scale(), cell.size.y * 0.5), true)
		_mouse_button(effect_tree, cell.position + Vector2(70.0 * EditorInterface.get_editor_scale(), cell.size.y * 0.5), false)
		await _settle(tree)
		_expect(layout.is_bus_effect_enabled(id, slots[0]), "Clicking an effect label selects it without toggling its checkbox")
		_expect(EditorInterface.get_inspector().get_edited_object() == layout.get_bus_effect(id, slots[0]), "Effect selection reuses the standard inspector")
		FmodServer.flush_bus_updates()
		var instance: FmodAudioEffect = FmodServer.get_bus_effect_instance(id, slots[0])
		history.clear_history()
		var destination: Tree = _row(dock, 0).find_child("Effects", true, false) as Tree
		effect_tree.force_drag({"type": "fmod_bus_effect", "layout": layout.get_instance_id(), "bus": id, "slot": slots[0],}, Control.new())
		var drop_position: Vector2 = Vector2(50.0, 12.0) * EditorInterface.get_editor_scale()
		_mouse_motion(destination, drop_position)
		_mouse_button(destination, drop_position, true)
		_mouse_button(destination, drop_position, false)
		await _settle(tree)
		FmodServer.flush_bus_updates()
		_expect(layout.get_bus_effect_ids(id).is_empty() and layout.get_bus_effect_ids(0).has(slots[0]), "Native drag and drop moves effects across buses")
		_expect(FmodServer.get_bus_effect_instance(0, slots[0]) == instance, "Dragging retains the running DSP instance")
		history.undo()
		await _settle(tree)
		FmodServer.flush_bus_updates()
		_expect(layout.get_bus_effect_ids(id).has(slots[0]) and FmodServer.get_bus_effect_instance(id, slots[0]) == instance, "Undo moves the same effect instance back")
		_expect(dock.call(&"save_layout") == OK, "First save writes the configured default layout")
		var default_reference: String = ProjectSettings.get_setting("audio/fmod/default_bus_layout")
		_expect(default_reference.begins_with("uid://"), "Saving the default updates its project reference to a UID")
		var saved: String = "res://bus_editor_saved.tres"
		_expect(dock.call(&"save_layout", saved) == OK and not dock.call(&"has_unsaved_changes"), "Explicit save clears the dirty state")
		_expect(ProjectSettings.get_setting("audio/fmod/default_bus_layout") == default_reference, "Save As keeps the project default separate")
		var extra: MenuButton = _button(dock, "FMOD") as MenuButton
		extra.get_popup().emit_signal(&"id_pressed", 5)
		var saved_reference: String = ProjectSettings.get_setting("audio/fmod/default_bus_layout")
		_expect(saved_reference.begins_with("uid://") and saved_reference != default_reference, "Set as Default explicitly selects the saved layout UID")
		var moved: String = "res://bus_editor_moved.tres"
		_expect(DirAccess.rename_absolute(saved, moved) == OK, "Layout file can move independently of its UID reference")
		EditorInterface.get_resource_filesystem().scan()
		while EditorInterface.get_resource_filesystem().is_scanning():
			await tree.process_frame
		_expect(dock.call(&"open_layout", saved_reference) == OK, "Opening the default UID resolves the moved file")
		layout = dock.call(&"get_layout") as FmodAudioBusLayout
		var old_names: PackedStringArray = FmodServer.get_bus_names()
		var invalid: FmodAudioBusLayout = FmodAudioBusLayout.new()
		invalid.set(&"layout_version", 999)
		_expect(FmodServer.set_audio_bus_layout(invalid) == ERR_INVALID_DATA and FmodServer.get_bus_names() == old_names, "Rejected layout replacement preserves the running graph")
		await _capture(dock, output_dir.path_join("fmod_bus_dock.png"), tree)
		layout.set_state(before)
		FmodServer.flush_bus_updates()
		history.clear_history()
		await _settle(tree)
		dock.call(&"save_layout")
	EditorInterface.edit_resource(Resource.new())
	await _settle(tree)
	for class_name_value: StringName in [&"FmodAudioStreamPlayer", &"FmodAudioStreamPlayer2D", &"FmodAudioStreamPlayer3D",]:
		var player: Node = ClassDB.instantiate(class_name_value) as Node
		player.set(&"bus", &"Missing FMOD Bus")
		_expect(player.get(&"bus") == &"Master", "%s falls back to Master" % class_name_value)
		for property: Dictionary in player.get_property_list():
			if property["name"] == &"bus":
				_expect(property["hint_string"] == ",".join(FmodServer.get_bus_names()), "%s lists the current FMOD layout" % class_name_value)
		player.free()
	dock.close()
	await _settle(tree)
	var head: FmodDSP = FmodServer.get_master_channel_group().get_dsp(0)
	var previous_metering: Dictionary = head.get_metering_enabled()
	dock.make_visible()
	TranslationServer.set_locale("en")
	await _settle(tree)
	_expect(domain.translate(&"FMOD Audio") == &"FMOD Audio", "English editor text remains available")
	await _capture(dock, output_dir.path_join("fmod_bus_dock_en.png"), tree)
	TranslationServer.set_locale("zh_Hans")
	await tree.create_timer(0.1).timeout
	if FmodServer.is_syncing_godot_buses():
		_expect(head.get_metering_enabled() == previous_metering, "Hidden synchronized buses leave metering flags unchanged")
	else:
		_expect(head.get_metering_enabled()["output_enabled"], "Visible Dock enables FMOD output metering")
	dock.close()
	await _settle(tree)
	_expect(head.get_metering_enabled() == previous_metering, "Hiding the Dock restores prior metering flags")
	for node: Node in dock.find_children("*", "Timer", true, false):
		var timer: Timer = node as Timer
		if is_equal_approx(timer.wait_time, 1.0 / 30.0):
			_expect(timer.is_stopped(), "Hidden Dock stops its 30 Hz polling timer")
	var report: FileAccess = FileAccess.open(output_dir.path_join("audio_bus_dock.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _check_native_panel(dock: EditorDock, body: MarginContainer, bus_panel: ScrollContainer, output_dir: String, tree: SceneTree) -> void:
	var native_panels: Array[Node] = EditorInterface.get_base_control().find_children("*", "EditorAudioBuses", true, false)
	_expect(native_panels.size() == 1, "Native Audio Dock is available for comparison")
	if native_panels.size() != 1:
		return
	var native_dock: EditorDock = native_panels[0] as EditorDock
	var native_body: MarginContainer
	for node: Node in native_dock.find_children("*", "MarginContainer", true, false):
		var candidate: MarginContainer = node as MarginContainer
		if candidate.theme_type_variation == &"NoBorderBottomPanel":
			native_body = candidate
			break
	_expect(native_body != null, "Native Audio body uses the same margin variation")
	if native_body == null:
		return
	var native_scroll: ScrollContainer = native_body.get_child(0) as ScrollContainer
	for constant: StringName in [&"margin_left", &"margin_top", &"margin_right", &"margin_bottom",]:
		_expect(body.get_theme_constant(constant) == native_body.get_theme_constant(constant), "Body %s matches native Audio" % constant)
	_expect(bus_panel.get_theme_stylebox(&"panel") == native_scroll.get_theme_stylebox(&"panel"), "FMOD and native Audio share the scroll panel StyleBox")
	_expect(bus_panel.scroll_hint_mode == native_scroll.scroll_hint_mode and bus_panel.mouse_filter == native_scroll.mouse_filter, "Scroll hints and mouse filtering match native Audio")
	for text: String in ["Add Bus", "Load", "Save As", "Load Default", "Create",]:
		var actual: Button = _button(dock, text)
		var expected: Button = _button(native_dock, text)
		var domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.editor")
		_expect(expected != null and actual.flat == expected.flat and actual.mouse_filter == expected.mouse_filter and domain.translate(actual.tooltip_text) == domain.translate(expected.tooltip_text), "Header %s matches the native button and tooltip" % text)
	var previous_minimum: Vector2 = native_dock.custom_minimum_size
	native_dock.custom_minimum_size.y = dock.custom_minimum_size.y
	native_dock.make_visible()
	await _settle(tree)
	var native_scales: Array[Node] = native_dock.find_children("*", "EditorAudioMeterNotches", true, false)
	var fmod_scales: Array[Node] = dock.find_children("*", "FmodAudioBusMeterNotches", true, false)
	_expect(not native_scales.is_empty() and not fmod_scales.is_empty(), "Both docks expose their dedicated meter scale controls")
	var native_scale: Control = native_scales[0] as Control if not native_scales.is_empty() else null
	var fmod_scale: Control = fmod_scales[0] as Control if not fmod_scales.is_empty() else null
	var native_bars: Array[Node] = native_dock.find_children("*", "TextureProgressBar", true, false)
	var fmod_bars: Array[Node] = dock.find_children("*", "TextureProgressBar", true, false)
	_expect(not native_bars.is_empty() and not fmod_bars.is_empty(), "Both meters use native TextureProgressBar rendering")
	var native_bar: TextureProgressBar = native_bars[0] as TextureProgressBar if not native_bars.is_empty() else null
	var fmod_bar: TextureProgressBar = fmod_bars[0] as TextureProgressBar if not fmod_bars.is_empty() else null
	if native_bar != null and fmod_bar != null:
		for key: StringName in [&"texture_under", &"texture_progress", &"texture_over",]:
			var actual: GradientTexture2D = fmod_bar.get(key) as GradientTexture2D
			var expected: GradientTexture2D = native_bar.get(key) as GradientTexture2D
			_expect(actual != null and expected != null and actual.gradient.colors == expected.gradient.colors and actual.gradient.offsets == expected.gradient.offsets, "Meter %s uses the exact native gradient stops and colors" % key)
		_expect(fmod_bar.tint_under == native_bar.tint_under and fmod_bar.tint_over == native_bar.tint_over, "Meter uses both native audio-bus tint settings")
	var native_bar_image: Image = await _capture_meter_scale(native_bar, output_dir.path_join("native_meter_inactive.png"), tree)
	if native_scale != null and fmod_scale != null:
		_expect(fmod_scale.get_minimum_size().is_equal_approx(native_scale.get_minimum_size()), "Meter scale minimum width and height match native font metrics")
		_expect(fmod_scale.get_theme_font(&"font", &"Label") == native_scale.get_theme_font(&"font", &"Label") and fmod_scale.get_theme_font_size(&"font_size", &"Label") == native_scale.get_theme_font_size(&"font_size", &"Label"), "Meter scale uses the native Label font and font size")
		_expect(fmod_scale.mouse_filter == native_scale.mouse_filter, "Meter scale preserves native mouse filtering")
	var native_image: Image = await _capture_meter_scale(native_scale, output_dir.path_join("native_meter_scale.png"), tree)
	await _capture(native_dock, output_dir.path_join("native_audio_dock.png"), tree)
	native_dock.custom_minimum_size = previous_minimum
	dock.make_visible()
	await _settle(tree)
	if native_scale != null and fmod_scale != null and bus_panel.visible:
		bus_panel.scroll_vertical = 0
		await _settle(tree)
		_expect(fmod_scale.size.is_equal_approx(native_scale.size), "Meter scale layout height and width match the native Audio bus")
		var fmod_image: Image = await _capture_meter_scale(fmod_scale, output_dir.path_join("fmod_meter_scale.png"), tree)
		var fmod_bar_image: Image = await _capture_meter_scale(fmod_bar, output_dir.path_join("fmod_meter_inactive.png"), tree)
		if native_bar_image != null and fmod_bar_image != null:
			_expect(native_bar_image.get_size() == fmod_bar_image.get_size() and native_bar_image.get_data() == fmod_bar_image.get_data(), "Inactive FMOD meter gradient matches native Audio pixels")
		if native_image != null and fmod_image != null:
			_expect(fmod_image.get_size() == native_image.get_size() and fmod_image.get_data() == native_image.get_data(), "Rendered signs, dB suffixes, tick lengths, spacing, and baselines match native Audio pixels")
		await _capture(dock, output_dir.path_join("fmod_audio_dock_aligned.png"), tree)


func _capture_meter_scale(control: Control, path: String, tree: SceneTree) -> Image:
	if control == null or "--eq-capture" not in OS.get_cmdline_user_args():
		return null
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = control.get_viewport().get_texture().get_image()
	var rect: Rect2i = Rect2i(control.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	if not rect.has_area():
		return null
	var result: Image = image.get_region(rect)
	_expect(result.save_png(path) == OK, "Meter scale screenshot captured")
	return result


func _check_sync(layout: FmodAudioBusLayout, tree: SceneTree) -> void:
	var original: AudioBusLayout = AudioServer.generate_bus_layout()
	AudioServer.add_bus()
	var index: int = AudioServer.bus_count - 1
	AudioServer.set_bus_name(index, &"FMOD Sync Check")
	var effect: AudioEffectDelay = AudioEffectDelay.new()
	AudioServer.add_bus_effect(index, effect)
	await _settle(tree)
	FmodServer.generate_bus_layout()
	var id: int = layout.find_bus_id(&"FMOD Sync Check")
	_expect(id > 0 and layout.get_bus_effect_ids(id).size() == 1, "Native buses and supported effects appear in the mirror")
	if id > 0 and not layout.get_bus_effect_ids(id).is_empty():
		var slot: int = layout.get_bus_effect_ids(id)[0]
		var instance: FmodAudioEffectDelay = FmodServer.get_bus_effect_instance(id, slot) as FmodAudioEffectDelay
		AudioServer.set_bus_name(index, &"FMOD Sync Renamed")
		AudioServer.set_bus_volume_db(index, -8.0)
		effect.set_tap1_delay_ms(220.0)
		AudioServer.set_bus_effect_enabled(index, 0, false)
		await _settle(tree)
		FmodServer.generate_bus_layout()
		_expect(layout.find_bus_id(&"FMOD Sync Renamed") == id, "Native bus rename preserves its stable FMOD ID")
		_expect(is_equal_approx(layout.get_bus_volume_db(id), -8.0) and not layout.is_bus_effect_enabled(id, slot), "Native mixing state and enabled flags synchronize")
		_expect(FmodServer.get_bus_effect_instance(id, slot) == instance and is_equal_approx(instance.get_tap1_delay_ms(), 220.0), "Native parameter edits preserve the running effect instance")
	AudioServer.set_bus_layout(original)
	AudioServer.emit_signal(&"bus_layout_changed")
	FmodServer.generate_bus_layout()
	await _settle(tree)


func _row(dock: Control, id: int) -> Control:
	for node: Node in dock.find_children("*", "FmodAudioBusEditor", true, false):
		if int(node.get_meta(&"bus_id")) == id:
			return node as Control
	return null


func _button(dock: Control, text: String) -> Button:
	var translated: String = TranslationServer.get_or_add_domain(&"godot.editor").translate(text)
	for node: Node in dock.find_children("*", "Button", true, false):
		if (node as Button).text in [text, translated,]:
			return node as Button
	return null


func _mouse_button(control: Control, position: Vector2, pressed: bool) -> void:
	control.get_viewport().notification(Node.NOTIFICATION_VP_MOUSE_ENTER)
	control.notification(Control.NOTIFICATION_MOUSE_ENTER)
	var event: InputEventMouseButton = InputEventMouseButton.new()
	event.button_index = MOUSE_BUTTON_LEFT
	event.pressed = pressed
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = control.get_global_transform_with_canvas() * position
	event.global_position = event.position
	control.get_viewport().push_input(event, true)


func _mouse_motion(control: Control, position: Vector2) -> void:
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.button_mask = MOUSE_BUTTON_MASK_LEFT
	event.position = control.get_global_transform_with_canvas() * position
	event.global_position = event.position
	control.get_viewport().push_input(event, true)


func _settle(tree: SceneTree) -> void:
	for index: int in range(5):
		await tree.process_frame


func _capture(control: Control, path: String, tree: SceneTree) -> void:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = control.get_viewport().get_texture().get_image()
	var rect: Rect2i = Rect2i(control.get_global_rect()).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	_expect(rect.has_area() and image.get_region(rect).save_png(path) == OK, "Dock screenshot captured")


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("BUS_DOCK_CHECK_FAILED ", label)
