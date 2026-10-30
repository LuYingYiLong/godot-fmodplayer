extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool


func run(output_dir: String) -> bool:
	var layout: FmodAudioBusLayout = FmodAudioBusLayout.new()
	var parent_id: int = layout.add_bus(&"RoutingParent")
	var child_id: int = layout.add_bus(&"RoutingChild", parent_id)
	var peer_id: int = layout.add_bus(&"RoutingPeer")
	_expect(layout.validate() == OK and layout.get_bus_ids() == PackedInt64Array([0, parent_id, child_id, peer_id,]), "Layout starts with Master and stable IDs")
	_expect(layout.set_bus_send(parent_id, child_id) == ERR_INVALID_PARAMETER, "Routing cycles are rejected")
	_expect(layout.set_bus_send(child_id, child_id) == ERR_INVALID_PARAMETER, "Self sends are rejected")
	_expect(layout.remove_bus(0) == ERR_INVALID_PARAMETER and layout.rename_bus(0, &"Other") == ERR_INVALID_PARAMETER, "Master remains fixed")
	_expect(layout.add_bus(&"RoutingParent") == -1 and layout.add_bus(&"A,B") == -1, "Names stay unique and usable by the bus enum")
	_expect(layout.set_bus_send(child_id, peer_id) == OK, "Sending to a bus on the right is supported")
	layout.move_bus(peer_id, 1)
	_expect(layout.get_bus_send(child_id) == peer_id, "Reordering preserves routing IDs")
	layout.set_bus_send(child_id, parent_id)
	var reverb: FmodAudioEffectReverb = FmodAudioEffectReverb.new()
	reverb.room_size = 0.85
	reverb.wet = 0.8
	reverb.dry = 0.0
	var reverb_slot: int = layout.add_bus_effect(parent_id, reverb)
	var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	var capture_slot: int = layout.add_bus_effect(parent_id, capture)
	var peer_slot: int = layout.add_bus_effect(peer_id, reverb)
	var saved: String = output_dir.path_join("fmod_bus_layout.tres")
	_expect(ResourceSaver.save(layout, saved) == OK, "Independent layout serializes through ResourceSaver")
	var loaded: FmodAudioBusLayout = ResourceLoader.load(saved, "FmodAudioBusLayout", ResourceLoader.CACHE_MODE_IGNORE_DEEP) as FmodAudioBusLayout
	_expect(loaded != null and loaded.validate() == OK and loaded.get_bus_ids() == layout.get_bus_ids(), "Reload restores IDs and validates routes")
	_expect(loaded.get_bus_send(child_id) == parent_id and loaded.get_bus_effect_ids(parent_id) == layout.get_bus_effect_ids(parent_id), "Reload restores routing and slot IDs")
	_expect(loaded.get_bus_effect(parent_id, reverb_slot) == loaded.get_bus_effect(peer_id, peer_slot), "Shared configuration references survive serialization")
	var text: String = FileAccess.get_file_as_string(saved)
	_expect(not text.contains("FmodChannelGroup") and not text.contains("FmodDSP"), "Serialized data contains no running FMOD handles")
	var state: Array = layout.get_state()
	layout.remove_bus(parent_id)
	_expect(layout.get_bus_send(child_id) == 0, "Deleting a bus reconnects children to its former target")
	_expect(layout.set_state(state) == OK and layout.get_bus_send(child_id) == parent_id, "Snapshot restores a deleted bus and its routes")
	var invalid: Array = layout.get_state()
	invalid[0]["name"] = &"Broken Master"
	_expect(layout.set_state(invalid) == ERR_INVALID_DATA and layout.get_bus_name(0) == &"Master", "Invalid snapshots leave the layout intact")
	var active: FmodAudioBusLayout = FmodServer.get_audio_bus_layout()
	var original: Array = active.get_state()
	active.set_state(layout.get_state())
	FmodServer.flush_bus_updates()
	var bus: FmodAudioBus = FmodServer.get_audio_bus(&"RoutingParent")
	var child: FmodAudioBus = FmodServer.get_audio_bus(&"RoutingChild")
	_expect(bus != null and child.get_bus().get_parent_group().get_name() == bus.get_bus().get_name(), "Runtime connects the requested parent")
	var first: FmodAudioEffectReverb = FmodServer.get_bus_effect_instance(parent_id, reverb_slot) as FmodAudioEffectReverb
	var second: FmodAudioEffectReverb = FmodServer.get_bus_effect_instance(peer_id, peer_slot) as FmodAudioEffectReverb
	_expect(first != null and second != null and first != second and first != reverb, "Each slot owns a separate running effect")
	_expect(reverb.get_bus() == null, "Configuration resources stay detached from audio buses")
	active.set_bus_solo(child_id, true)
	FmodServer.flush_bus_updates()
	_expect(not bus.get_bus().get_mute() and not child.get_bus().get_mute(), "Solo keeps the send path to Master audible")
	_expect(FmodServer.get_audio_bus(&"RoutingPeer").get_bus().get_mute(), "Solo mutes unrelated buses")
	active.set_bus_solo(child_id, false)
	FmodServer.flush_bus_updates()
	var effect_dsp: FmodDSP = bus.get_bus().get_dsp(1)
	var started: int = Time.get_ticks_usec()
	for index: int in range(100):
		reverb.room_size = 0.5 + float(index % 20) * 0.01
		FmodServer.flush_bus_updates()
	var update_usec: int = Time.get_ticks_usec() - started
	_expect(FmodServer.get_bus_effect_instance(parent_id, reverb_slot) == first and bus.get_bus().get_dsp_index(effect_dsp) >= 0, "Parameter edits preserve the live effect and DSP")
	_expect(is_equal_approx(first.room_size, reverb.room_size) and is_equal_approx(second.room_size, reverb.room_size), "Shared configuration edits update both running instances")
	active.set_bus_effect_enabled(parent_id, reverb_slot, false)
	FmodServer.flush_bus_updates()
	_expect(FmodServer.get_bus_effect_instance(parent_id, reverb_slot) == first and effect_dsp.get_bypass(), "Disabling a slot bypasses its DSP without releasing it")
	active.set_bus_effect_enabled(parent_id, reverb_slot, true)
	FmodServer.flush_bus_updates()
	var running_capture: FmodAudioEffectCapture = FmodServer.get_bus_effect_instance(parent_id, capture_slot) as FmodAudioEffectCapture
	var player: FmodAudioStreamPlayer = FmodAudioStreamPlayer.new()
	var stream: FmodAudioStream = FmodAudioStream.new()
	stream.set_audio_data(_impulse())
	player.stream = stream
	player.bus = &"RoutingParent"
	player.play()
	var system: FmodSystem = FmodServer.get_main_system()
	for index: int in range(12):
		system.update()
	running_capture.clear_buffer()
	reverb.wet = 0.75
	FmodServer.flush_bus_updates()
	for index: int in range(4):
		system.update()
	var tail: PackedVector2Array = running_capture.get_buffer(running_capture.get_frames_available())
	var energy: float
	for sample: Vector2 in tail:
		energy += sample.length_squared()
	_expect(energy > 0.00000001, "Reverb tail survives configuration edits")
	active.move_bus_effect(parent_id, reverb_slot, peer_id, 0)
	FmodServer.flush_bus_updates()
	_expect(FmodServer.get_bus_effect_instance(peer_id, reverb_slot) == first and first.get_bus() == FmodServer.get_audio_bus(&"RoutingPeer").get_bus(), "Moving a slot reconnects its existing runtime instance")
	active.remove_bus(parent_id)
	FmodServer.flush_bus_updates()
	_expect(player.is_playing(), "Deleting an occupied bus keeps its playing channels alive")
	player.stop()
	player.free()
	active.set_state(original)
	FmodServer.flush_bus_updates()
	var report: FileAccess = FileAccess.open(output_dir.path_join("audio_bus_regression.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks, "parameter_updates_usec": update_usec, "tail_energy": energy,}, "\t"))
	return not _failed


func _impulse() -> PackedByteArray:
	var data: PackedByteArray
	data.resize(44 + 48000 * 4)
	data.encode_u32(0, 0x46464952)
	data.encode_u32(4, data.size() - 8)
	data.encode_u32(8, 0x45564157)
	data.encode_u32(12, 0x20746d66)
	data.encode_u32(16, 16)
	data.encode_u16(20, 1)
	data.encode_u16(22, 2)
	data.encode_u32(24, 48000)
	data.encode_u32(28, 192000)
	data.encode_u16(32, 4)
	data.encode_u16(34, 16)
	data.encode_u32(36, 0x61746164)
	data.encode_u32(40, data.size() - 44)
	data.encode_s16(44, 24000)
	data.encode_s16(46, 24000)
	return data


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("BUS_CHECK_FAILED ", label)
