@tool
extends RefCounted

const MP3_UID: String = "uid://c6kky7072rqpo"

var _checks: Array[Dictionary]
var _failed: bool
var _input_targets: Array[Control]


func run(output_dir: String, tree: SceneTree) -> bool:
	var native_settings: Dictionary = await _native_reference(output_dir, tree)
	var stream: FmodAudioStream = FmodAudioStream.new()
	stream.resource_name = "preview.wav"
	stream.set_audio_data(_wav(8000, 16))
	var shared: RefCounted
	for node_class: String in ["FmodAudioStreamPlayer", "FmodAudioStreamPlayer2D", "FmodAudioStreamPlayer3D",]:
		var player: Node = ClassDB.instantiate(node_class) as Node
		player.set(&"stream", stream)
		EditorInterface.inspect_object(player, "", true)
		await _settle(tree)
		var editor: EditorProperty = _stream_editor()
		_expect(editor != null, "%s has a stream property editor" % node_class)
		if editor == null:
			EditorInterface.edit_resource(Resource.new())
			await _settle(tree)
			player.free()
			continue
		var view: Control = editor.find_children("*", "FmodAudioWaveformView", true, false)[0] as Control
		var waveform: RefCounted = view.call(&"get_waveform") as RefCounted
		_expect(await _wait_ready(waveform, tree), "%s completes background waveform" % node_class)
		_expect(editor.get_edited_object() == player and editor.get_edited_property() == &"stream", "%s retains stream property and reset support" % node_class)
		_expect(not stream.is_preloaded(), "%s preview does not create a playback sound" % node_class)
		if shared == null:
			shared = waveform
		else:
			_expect(waveform == shared, "%s reuses the same waveform object" % node_class)
		await _capture(editor, output_dir.path_join(node_class + "_stream.png"), tree)
		var picker: EditorResourcePicker = editor.find_children("*", "EditorResourcePicker", true, false)[0] as EditorResourcePicker
		_expect(picker.base_type == "FmodAudioStream" and picker.get_edited_resource() == stream, "%s uses the native resource picker" % node_class)
		var assign_button: Button = view.get_parent() as Button
		_expect(assign_button != null and assign_button.get_parent() == picker, "%s draws waveform inside the native assignment button" % node_class)
		await _settle(tree)
		var actual_settings: Dictionary = _picker_settings(editor, picker, assign_button, view)
		for setting: String in native_settings:
			if actual_settings[setting] != native_settings[setting]:
				print("PICKER_SETTING_DIAGNOSTIC ", setting, " actual=", actual_settings[setting], " native=", native_settings[setting])
			_expect(actual_settings[setting] == native_settings[setting], "%s matches native picker %s" % [node_class, setting,])
		_input_targets.clear()
		assign_button.gui_input.connect(_input_received.bind(assign_button))
		_motion(view, view.size * Vector2(0.5, 0.25))
		_expect(assign_button in _input_targets, "%s waveform motion reaches native assignment button" % node_class)
		var manager: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
		var history: UndoRedo = manager.get_history_undo_redo(manager.get_object_history_id(player))
		history.clear_history()
		picker.emit_signal(&"resource_changed", null)
		_expect(player.get(&"stream") == null and history.get_history_count() == 1, "%s clearing stream creates one undo action" % node_class)
		history.undo()
		await _settle(tree)
		_expect(player.get(&"stream") == stream, "%s undo restores stream" % node_class)
		history.redo()
		await _settle(tree)
		_expect(player.get(&"stream") == null and view.call(&"get_waveform") == null, "%s empty stream keeps native empty picker" % node_class)
		history.undo()
		await _settle(tree)
		editor.read_only = true
		await _drop_resource(view, FmodAudioStream.new(), tree)
		_expect(player.get(&"stream") == stream and not picker.editable, "%s read-only stream rejects assignment" % node_class)
		editor.read_only = false
		var folded_height: float = editor.size.y
		var folded_minimum: Vector2 = editor.get_combined_minimum_size()
		_click(view, view.size * Vector2(0.5, 0.25))
		await _settle(tree)
		var panels: Array[Node] = editor.find_children("*", "FmodAudioPreviewProperty", true, false)
		_expect(panels.size() == 1, "%s expands the existing audition panel" % node_class)
		if panels.size() == 1:
			var panel_view: Node = panels[0].find_children("*", "FmodAudioWaveformView", true, false)[0]
			_expect(panel_view.call(&"get_waveform") == shared, "%s audition panel shares the thumbnail cache" % node_class)
		_click(view, view.size * Vector2(0.5, 0.25))
		await _settle(tree)
		_expect(editor.find_children("*", "EditorInspector", true, false).is_empty(), "%s folding destroys the bottom inspector" % node_class)
		_expect(is_equal_approx(editor.size.y, folded_height) and editor.get_combined_minimum_size().is_equal_approx(folded_minimum), "%s folding restores the original row height" % node_class)
		await _capture(editor, output_dir.path_join(node_class + "_stream_folded.png"), tree, 48.0)
		_click(view, view.size * Vector2(0.5, 0.25))
		await _settle(tree)
		panels = editor.find_children("*", "FmodAudioPreviewProperty", true, false)
		_expect(panels.size() == 1, "%s reopening recreates one audition panel" % node_class)
		picker.emit_signal(&"resource_changed", null)
		await _settle(tree)
		_expect(editor.find_children("*", "EditorInspector", true, false).is_empty(), "%s clearing an expanded stream removes the bottom inspector" % node_class)
		history.undo()
		await _settle(tree)
		_expect(player.get(&"stream") == stream and editor.find_children("*", "FmodAudioPreviewProperty", true, false).size() == 1, "%s undo restores the expanded stream inspector" % node_class)
		_click(view, view.size * Vector2(0.5, 0.25), MOUSE_BUTTON_RIGHT)
		await _settle(tree)
		var menu_opened: bool
		for node: Node in picker.find_children("*", "PopupMenu", true, false):
			var menu: PopupMenu = node as PopupMenu
			menu_opened = menu_opened or menu.visible
			menu.hide()
		_expect(menu_opened, "%s waveform right click opens the native resource menu" % node_class)
		await _settle(tree)
		var empty_stream: FmodAudioStream = FmodAudioStream.new()
		await _drop_resource(view, empty_stream, tree)
		await _settle(tree)
		if player.get(&"stream") != empty_stream:
			print("PICKER_DROP_DIAGNOSTIC actual=", player.get(&"stream"), " expected=", empty_stream, " success=", view.get_viewport().gui_is_drag_successful(), " ctrl=", Input.is_key_pressed(KEY_CTRL))
		_expect(player.get(&"stream") == empty_stream and view.visible, "%s dropping onto waveform assigns an empty resource" % node_class)
		empty_stream.set_audio_data(_wav(8000, 16))
		var filled: RefCounted = view.call(&"get_waveform") as RefCounted
		_expect(view.visible and await _wait_ready(filled, tree), "%s filling an empty resource reveals waveform" % node_class)
		history.clear_history()
		EditorInterface.edit_resource(Resource.new())
		await _settle(tree)
		player.free()
	EditorInterface.edit_resource(stream)
	await _settle(tree)
	var panels: Array[Node] = EditorInterface.get_inspector().find_children("*", "FmodAudioPreviewProperty", true, false)
	_expect(panels.size() == 1, "Resource inspector retains audition panel")
	if panels.size() == 1:
		var panel: Control = panels[0] as Control
		var view: Node = panel.find_children("*", "FmodAudioWaveformView", true, false)[0]
		_expect(view.call(&"get_waveform") == shared, "Standalone audition panel reuses shared waveform")
		await _capture(panel, output_dir.path_join("stream_audition.png"), tree)
		panel.call(&"_play")
		await _settle(tree)
		var player: Node = panel.find_children("*", "FmodAudioStreamPlayer", true, false)[0]
		_expect(player.call(&"is_playing"), "Audition still plays through FMOD")
		panel.call(&"_stop")
		_expect(not player.call(&"is_playing") and view.call(&"get_waveform") == shared, "Stopping audition preserves shared waveform")
	EditorInterface.edit_resource(Resource.new())
	await _settle(tree)
	await _test_property_name_styles(tree)
	await _test_decode(output_dir, tree)
	var report: FileAccess = FileAccess.open(output_dir.path_join("stream_waveform_inspector.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_property_name_styles(tree: SceneTree) -> void:
	var menu: PopupMenu
	var raw_index: int = -1
	for node: Node in EditorInterface.get_base_control().find_children("*", "MenuButton", true, false):
		var candidate: PopupMenu = (node as MenuButton).get_popup()
		for index: int in range(candidate.item_count):
			if candidate.is_item_radio_checkable(index) and candidate.get_item_text(index).contains("z_index"):
				menu = candidate
				raw_index = index
				break
		if menu != null:
			break
	_expect(menu != null, "Found Godot's property name style menu")
	if menu == null:
		return
	menu.emit_signal(&"about_to_popup")
	var original_id: int = menu.get_item_id(raw_index + 1)
	for offset: int in range(3):
		if menu.is_item_checked(raw_index + offset):
			original_id = menu.get_item_id(raw_index + offset)
	var expected_labels: Array[String] = ["default_frequency", "Default Frequency", "默认频率",]
	var stream: FmodAudioStream = FmodAudioStream.new()
	for style: int in range(expected_labels.size()):
		menu.emit_signal(&"id_pressed", menu.get_item_id(raw_index + style))
		await _settle(tree)
		for node_class: String in ["FmodAudioStreamPlayer", "FmodAudioStreamPlayer2D", "FmodAudioStreamPlayer3D",]:
			var player: Node = ClassDB.instantiate(node_class) as Node
			player.set(&"stream", stream)
			EditorInterface.inspect_object(player, "", true)
			await _settle(tree)
			var editor: EditorProperty = _stream_editor()
			_expect(editor != null and editor.is_class("EditorPropertyResource"), "%s retains Godot's native resource property editor" % node_class)
			if editor != null:
				var picker: EditorResourcePicker = editor.find_children("*", "EditorResourcePicker", true, false)[0] as EditorResourcePicker
				picker.emit_signal(&"resource_selected", stream, false)
				await _settle(tree)
				var label: String
				for child: Node in editor.find_children("*", "EditorProperty", true, false):
					var property: EditorProperty = child as EditorProperty
					if property.get_edited_property() == &"default_frequency":
						label = property.label
						break
				_expect(label == expected_labels[style], "%s child property follows Godot style %d: %s" % [node_class, style, label,])
			EditorInterface.edit_resource(Resource.new())
			await _settle(tree)
			player.free()
	menu.emit_signal(&"id_pressed", original_id)
	await _settle(tree)


func _test_decode(output_dir: String, tree: SceneTree) -> void:
	var host: Control = FmodAudioPreviewProperty.new()
	tree.root.add_child(host)
	host.hide()
	var view: Control = host.find_children("*", "FmodAudioWaveformView", true, false)[0] as Control
	var stream: FmodAudioStream = FmodAudioStream.new()
	for bits: int in [8, 16, 24, 32, -32,]:
		stream.set_audio_data(_wav(8000, bits))
		view.call(&"set_stream", stream)
		var waveform: RefCounted = view.call(&"get_waveform") as RefCounted
		_expect(await _wait_ready(waveform, tree), "PCM %d decode completes" % bits)
		var envelope: PackedVector2Array = waveform.call(&"get_envelope")
		_expect(not envelope.is_empty() and envelope[0].x < -0.49 and envelope[0].y > 0.49, "PCM %d preserves opposing stereo peaks" % bits)
		_expect(is_equal_approx(waveform.call(&"get_length"), 1.0), "PCM %d preserves duration" % bits)
	var before: RefCounted = view.call(&"get_waveform") as RefCounted
	stream.set_mode_flags(FmodAudioStream.MODE_SAMPLE | FmodAudioStream.MODE_LOOP)
	_expect(view.call(&"get_waveform") == before, "Playback mode changes preserve waveform cache")
	stream.set_audio_data(_wav(16000, 16))
	var replaced: RefCounted = view.call(&"get_waveform") as RefCounted
	_expect(replaced != before and await _wait_ready(replaced, tree), "Replacing audio data invalidates waveform cache")
	_expect(is_equal_approx(replaced.call(&"get_length"), 2.0), "Replacement waveform uses new duration")
	stream.suggested_sound_type = FmodSound.FMOD_SOUND_TYPE_RAW
	stream.default_frequency = 8000
	stream.num_channels = 2
	stream.format = FmodSound.FMOD_SOUND_FORMAT_PCM16
	stream.set_audio_data(_wav(8000, 16).slice(44))
	var raw: RefCounted = view.call(&"get_waveform") as RefCounted
	_expect(await _wait_ready(raw, tree) and is_equal_approx(raw.call(&"get_length"), 1.0), "Raw PCM preview honors stream creation settings")
	stream = FmodAudioStream.new()
	stream.set_audio_data(_wav(8400000, 16))
	var request_started: int = Time.get_ticks_msec()
	view.call(&"set_stream", stream)
	_expect(Time.get_ticks_msec() - request_started < 100, "Long audio request returns without blocking on decoding")
	var long_waveform: RefCounted = view.call(&"get_waveform") as RefCounted
	_expect(await _wait_ready(long_waveform, tree), "Long audio finishes bounded block decoding")
	var long_envelope: PackedVector2Array = long_waveform.call(&"get_envelope")
	_expect(not long_envelope.is_empty() and long_envelope[-1].y > 0.49 and is_equal_approx(long_waveform.call(&"get_length"), 1050.0), "Waveform includes audio beyond the former 30 MiB cutoff")
	stream = FmodAudioStream.new()
	stream.set_audio_data(PackedByteArray([1, 2, 3, 4,]))
	view.call(&"set_stream", stream)
	var invalid: RefCounted = view.call(&"get_waveform") as RefCounted
	await _wait_ready(invalid, tree, false)
	_expect(invalid.call(&"is_ready") and not String(invalid.call(&"get_error")).is_empty() and PackedVector2Array(invalid.call(&"get_envelope")).is_empty(), "Malformed audio completes with a cached error")
	stream.clear()
	var empty: RefCounted = view.call(&"get_waveform") as RefCounted
	_expect(empty != invalid and empty.call(&"is_ready"), "Clearing audio invalidates cached error")
	var source_path: String = ResourceUID.get_id_path(ResourceUID.text_to_id(MP3_UID))
	_expect(not source_path.is_empty(), "MP3 fixture resolves by UID")
	if not source_path.is_empty():
		stream = load(MP3_UID) as FmodAudioStream
		_expect(stream != null, "Imported FMOD MP3 remains a usable stream resource")
		view.call(&"set_stream", stream)
		var mp3: RefCounted = view.call(&"get_waveform") as RefCounted
		_expect(await _wait_ready(mp3, tree) and mp3.call(&"get_length") > 1.0, "Compressed MP3 generates a complete waveform")
		var player: Node = FmodAudioStreamPlayer3D.new()
		player.set(&"stream", stream)
		EditorInterface.inspect_object(player, "", true)
		await _settle(tree)
		var editor: Control = _stream_editor()
		await _capture(editor, output_dir.path_join("stream_mp3.png"), tree)
		EditorInterface.edit_resource(Resource.new())
		# 导入器缓存会持有此测试资源，退出前先释放它的运行时 Sound
		stream.clear()
		await _settle(tree)
		player.free()
	host.free()
	await _settle(tree)


func _wav(frames: int, bits: int) -> PackedByteArray:
	var bytes_per_sample: int = absi(bits) >> 3
	var data: PackedByteArray
	data.resize(44 + frames * 2 * bytes_per_sample)
	data.encode_u32(0, 0x46464952)
	data.encode_u32(4, data.size() - 8)
	data.encode_u32(8, 0x45564157)
	data.encode_u32(12, 0x20746d66)
	data.encode_u32(16, 16)
	data.encode_u16(20, 3 if bits < 0 else 1)
	data.encode_u16(22, 2)
	data.encode_u32(24, 8000)
	data.encode_u32(28, 8000 * 2 * bytes_per_sample)
	data.encode_u16(32, 2 * bytes_per_sample)
	data.encode_u16(34, absi(bits))
	data.encode_u32(36, 0x61746164)
	data.encode_u32(40, data.size() - 44)
	for frame: int in range(maxi(0, frames - 8000), frames):
		for channel: int in range(2):
			var offset: int = 44 + (frame * 2 + channel) * bytes_per_sample
			var sign_value: int = 1 if channel == 0 else -1
			match bits:
				8:
					data.encode_u8(offset, 128 + sign_value * 64)
				16:
					data.encode_s16(offset, sign_value * 16384)
				24:
					var value: int = sign_value * 4194304
					data.encode_u8(offset, value & 255)
					data.encode_u8(offset + 1, (value >> 8) & 255)
					data.encode_u8(offset + 2, (value >> 16) & 255)
				32:
					data.encode_s32(offset, sign_value * 1073741824)
				-32:
					data.encode_float(offset, float(sign_value) * 0.5)
	return data


func _wait_ready(waveform: RefCounted, tree: SceneTree, expect_success: bool = true) -> bool:
	if waveform == null:
		return false
	var deadline: int = Time.get_ticks_msec() + 15000
	while not waveform.call(&"is_ready") and Time.get_ticks_msec() < deadline:
		await tree.create_timer(0.01).timeout
	return waveform.call(&"is_ready") and (not expect_success or String(waveform.call(&"get_error")).is_empty())


func _native_reference(output_dir: String, tree: SceneTree) -> Dictionary:
	var stream: AudioStreamWAV = AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = 8000
	stream.stereo = true
	stream.data = _wav(8000, 16).slice(44)
	stream.resource_name = "preview.wav"
	var player: AudioStreamPlayer3D = AudioStreamPlayer3D.new()
	player.stream = stream
	EditorInterface.inspect_object(player, "", true)
	await _settle(tree)
	var editor: EditorProperty = _stream_editor()
	var picker: EditorResourcePicker = editor.find_children("*", "EditorResourcePicker", true, false)[0] as EditorResourcePicker
	var button: Button
	for child: Node in picker.get_children():
		if child is Button and (child as Button).size_flags_horizontal & Control.SIZE_EXPAND:
			button = child as Button
	var preview: Control = button.get_child(0) as Control
	await _capture(editor, output_dir.path_join("native_audio_stream_picker.png"), tree)
	var settings: Dictionary = _picker_settings(editor, picker, button, preview)
	EditorInterface.edit_resource(Resource.new())
	await _settle(tree)
	player.free()
	return settings


func _stream_editor() -> EditorProperty:
	for node: Node in EditorInterface.get_inspector().find_children("*", "EditorProperty", true, false):
		var editor: EditorProperty = node as EditorProperty
		if editor.get_edited_property() == &"stream":
			return editor
	return null


func _picker_settings(editor: EditorProperty, picker: EditorResourcePicker, button: Button, preview: Control) -> Dictionary:
	var controls: Array[Dictionary]
	for node: Node in picker.get_children():
		if node is Button:
			var child: Button = node as Button
			controls.append({
				"mouse_filter": child.mouse_filter,
				"pass_scroll": child.mouse_force_pass_scroll_events,
				"button_mask": child.button_mask,
				"action_mode": child.action_mode,
				"focus_mode": child.focus_mode,
				"theme": child.theme_type_variation,
				"horizontal": child.size_flags_horizontal,
				"vertical": child.size_flags_vertical,
			})
	return {
		"property_mouse_filter": editor.mouse_filter,
		"picker_mouse_filter": picker.mouse_filter,
		"picker_pass_scroll": picker.mouse_force_pass_scroll_events,
		"buttons": controls,
		"preview_mouse_filter": preview.mouse_filter,
		"preview_pass_scroll": preview.mouse_force_pass_scroll_events,
		"preview_anchors": Vector4(preview.anchor_left, preview.anchor_top, preview.anchor_right, preview.anchor_bottom),
		"preview_offsets": Vector4(preview.offset_left, preview.offset_top, preview.offset_right, preview.offset_bottom),
		"button_height": button.size.y,
	}


func _motion(control: Control, position: Vector2, pressed: bool = false) -> void:
	control.get_viewport().notification(Node.NOTIFICATION_VP_MOUSE_ENTER)
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.position = control.get_global_transform_with_canvas() * position
	event.global_position = event.position
	control.get_viewport().push_input(event, true)


func _click(control: Control, position: Vector2, button: MouseButton = MOUSE_BUTTON_LEFT) -> void:
	_motion(control, position)
	# 离屏窗口没有操作系统鼠标进入事件，补上原生按钮的悬停状态
	control.get_parent().notification(Control.NOTIFICATION_MOUSE_ENTER)
	for pressed: bool in [true, false,]:
		var event: InputEventMouseButton = InputEventMouseButton.new()
		event.button_index = button
		event.pressed = pressed
		event.button_mask = (MOUSE_BUTTON_MASK_LEFT if button == MOUSE_BUTTON_LEFT else MOUSE_BUTTON_MASK_RIGHT) if pressed else 0
		event.position = control.get_global_transform_with_canvas() * position
		event.global_position = event.position
		control.get_viewport().push_input(event, true)
	control.get_parent().notification(Control.NOTIFICATION_MOUSE_EXIT)


func _drop_resource(control: Control, resource: Resource, tree: SceneTree) -> void:
	control.force_drag({"type": "resource", "resource": resource,}, Control.new())
	_motion(control, control.size * Vector2(0.5, 0.25), true)
	# 使用 force_drag 的点击投放路径，避免依赖离屏窗口的系统鼠标位置
	for pressed: bool in [true, false,]:
		if not pressed:
			control.get_parent().notification(Control.NOTIFICATION_MOUSE_EXIT)
		var event: InputEventMouseButton = InputEventMouseButton.new()
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
		event.position = control.get_global_transform_with_canvas() * (control.size * Vector2(0.5, 0.25))
		event.global_position = event.position
		control.get_viewport().push_input(event, true)
	await _settle(tree)


func _input_received(_event: InputEvent, control: Control) -> void:
	_input_targets.append(control)


func _settle(tree: SceneTree) -> void:
	for frame: int in range(4):
		await tree.process_frame


func _capture(control: Control, path: String, tree: SceneTree, extra_bottom: float = 0.0) -> void:
	if "--eq-capture" not in OS.get_cmdline_user_args():
		return
	await _settle(tree)
	RenderingServer.force_draw()
	var image: Image = control.get_viewport().get_texture().get_image()
	var capture_rect: Rect2 = control.get_global_rect()
	capture_rect.size.y += extra_bottom * EditorInterface.get_editor_scale()
	var area: Rect2i = Rect2i(capture_rect).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	_expect(area.has_area() and image.get_region(area).save_png(path) == OK, "Captured stream waveform preview")


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("STREAM_WAVEFORM_CHECK_FAILED ", label)
