extends RefCounted

const SAMPLE_RATE: int = 48000

var _system: FmodSystem
var _bus: FmodChannelGroup
var _player: FmodAudioStreamPlayer3D
var _channel: FmodChannel
var _capture: FmodAudioEffectCapture
var _checks: Array[Dictionary]
var _measurements: Dictionary
var _failed: bool


func run(output_dir: String) -> bool:
	_system = FmodServer.get_main_system()
	_system.set_3d_listener_attributes(
		0, Vector3.ZERO, Vector3.ZERO, Vector3(0.0, 0.0, 1.0), Vector3.UP
	)
	var layout: FmodAudioBusLayout = FmodServer.get_audio_bus_layout()
	layout.create_audio_bus("Spatial regression")
	var audio_bus: FmodAudioBus = layout.get_audio_bus("Spatial regression")
	_bus = audio_bus.get_bus()
	_capture = FmodAudioEffectCapture.new()
	_capture.apply_to(_bus)
	_player = FmodAudioStreamPlayer3D.new()
	_player.bus = &"Spatial regression"
	var tree: SceneTree = Engine.get_main_loop() as SceneTree
	tree.root.add_child(_player)
	var stream: FmodAudioStream = _create_stream(output_dir)
	if stream == null:
		_cleanup(layout, audio_bus)
		return false
	_player.stream = stream
	_player.global_position = Vector3(0.0, 0.0, 0.5)
	_player.attenuation_model = FmodAudioStreamPlayer3D.ATTENUATION_DISABLED
	_player.attenuation_filter_db = -12.0
	_player.attenuation_filter_cutoff_hz = 2400.0
	_player.play()
	_system.update()
	_expect(_bus.get_num_channels() == 1, "Fixture creates one native channel")
	if _bus.get_num_channels() != 1:
		_cleanup(layout, audio_bus)
		return false
	_channel = _bus.get_channel(0)
	_check_filter(-12.0, 2400.0, "Settings assigned before playback")
	for level: float in [0.0, -6.0, -24.0, -60.0, -80.0,]:
		_player.attenuation_filter_db = level
		_check_filter(level, 2400.0, "Live dB update")
	_player.attenuation_filter_cutoff_hz = 4000.0
	_check_filter(-80.0, 4000.0, "Cutoff update preserves linear level")
	_player.attenuation_filter_db = 10.0
	_player.attenuation_filter_cutoff_hz = 1.0
	_check_filter(0.0, 10.0, "Upper dB and lower frequency clamps")
	_player.attenuation_filter_db = -100.0
	_player.attenuation_filter_cutoff_hz = 30000.0
	_check_filter(-80.0, 22050.0, "Lower dB and upper frequency clamps")
	_check_angles(output_dir)
	_player.emission_angle_enabled = false
	_player.attenuation_filter_db = 0.0
	_player.attenuation_filter_cutoff_hz = 4000.0
	_measurements["disabled_near"] = _measure(0.5)
	_measurements["disabled_far"] = _measure(1000.0)
	var near: float = float(_measurements["disabled_near"])
	var far: float = float(_measurements["disabled_far"])
	_expect(near > 0.01 and far > 0.01, "0 dB filter produces audible output")
	_expect(_same_level(near, far), "Disabled attenuation keeps near/far RMS equal")
	_expect((_channel.get_mode() & FmodSystem.FMOD_MODE_3D) != 0, "Disabled attenuation keeps 3D mode")
	_expect(_channel.get_3d_level() == 1.0, "Disabled attenuation keeps full 3D panning")
	_expect((_channel.get_mode() & FmodSystem.FMOD_MODE_3D_CUSTOMROLLOFF) != 0, "Disabled attenuation uses custom rolloff")
	var curve: PackedVector3Array = _channel.get_3d_custom_rolloff()
	_expect(curve.size() == 2 and curve[0].y == 1.0 and curve[1].y == 1.0, "Flat curve has unity-gain endpoints")
	_player.unit_size = 2.0
	_player.max_distance = 20.0
	_measurements["disabled_resized_far"] = _measure(2000.0)
	_expect(_same_level(near, float(_measurements["disabled_resized_far"])), "Disabled mode ignores unit size and max distance")
	_player.unit_size = 1.0
	_player.max_distance = 10.0
	_player.attenuation_model = FmodAudioStreamPlayer3D.ATTENUATION_LOGARITHMIC
	_measurements["linear_near"] = _measure(0.5)
	_measurements["linear_far"] = _measure(20.0)
	_expect(float(_measurements["linear_near"]) > 0.01, "Linear mode retains near audio")
	_expect(float(_measurements["linear_far"]) < near * 0.01, "Switching back restores linear attenuation")
	_expect((_channel.get_mode() & FmodSystem.FMOD_MODE_3D_CUSTOMROLLOFF) == 0, "Linear mode clears custom rolloff flag")
	_player.attenuation_model = FmodAudioStreamPlayer3D.ATTENUATION_DISABLED
	_measurements["disabled_again_far"] = _measure(1000.0)
	_expect(_same_level(near, float(_measurements["disabled_again_far"])), "Disabling attenuation again restores far audio")
	_check_independent_player(layout, stream)
	_player.attenuation_filter_cutoff_hz = 2000.0
	_player.attenuation_filter_db = 0.0
	_measurements["filter_bypass"] = _measure(0.5)
	_player.attenuation_filter_db = -24.0
	_measurements["filter_active"] = _measure(0.5)
	_expect(
		float(_measurements["filter_active"]) < float(_measurements["filter_bypass"]) * 0.8,
		"Negative dB filter changes rendered audio with CHANNEL_DISTANCEFILTER enabled"
	)
	_player.attenuation_filter_db = 0.0
	_measurements["filter_reset"] = _measure(0.5)
	_expect(_same_level(near, float(_measurements["filter_reset"])), "0 dB restores unfiltered output")
	for native_level: float in [0.0, 0.01, 0.1, 0.5, 1.0,]:
		_channel.set_3d_distance_filter(true, native_level, 2000.0)
		_measurements["native_level_%s" % native_level] = _measure(0.5)
	_expect(_same_level(near, float(_measurements["native_level_0.0"])),
		"FMOD 2.04 native amount zero bypasses distance filtering"
	)
	_expect(float(_measurements["native_level_1.0"]) < near * 0.8,
		"FMOD 2.04 native amount one filters the rendered signal"
	)
	_cleanup(layout, audio_bus)
	stream.clear()
	var report: FileAccess = FileAccess.open(
		output_dir.path_join("spatial_audio_regression.json"), FileAccess.WRITE
	)
	if report == null:
		return false
	report.store_string(JSON.stringify({
		"passed": not _failed,
		"checks": _checks,
		"rms": _measurements,
		"distance_filter_init_enabled": true,
	}, "\t"))
	return not _failed


func _create_stream(output_dir: String) -> FmodAudioStream:
	var pcm: PackedByteArray
	pcm.resize(SAMPLE_RATE * 2)
	for frame: int in range(SAMPLE_RATE):
		var time: float = float(frame) / float(SAMPLE_RATE)
		var sample: float = 0.25 * (sin(TAU * 750.0 * time) + sin(TAU * 6000.0 * time))
		pcm.encode_s16(frame * 2, int(sample * 32767.0))
	var wav: AudioStreamWAV = AudioStreamWAV.new()
	wav.format = AudioStreamWAV.FORMAT_16_BITS
	wav.mix_rate = SAMPLE_RATE
	wav.data = pcm
	var fixture_path: String = output_dir.path_join("spatial_tones.wav")
	if wav.save_to_wav(fixture_path) != OK:
		return null
	return FmodAudioStream.load_from_file(fixture_path,
		FmodAudioStream.MODE_SAMPLE | FmodAudioStream.MODE_LOOP | FmodAudioStream.MODE_3D
	)


func _check_filter(db: float, frequency: float, label: String) -> void:
	_system.update()
	var state: Dictionary = _channel.get_3d_distance_filter()
	var expected: float = 1.0 - pow(10.0, db / 20.0)
	_expect(bool(state.get("custom", false)), label + ": custom override enabled")
	_expect(absf(float(state.get("custom_level", -1.0)) - expected) < 0.000001,
		label + ": dB mapped to native filter amount (%s dB)" % db
	)
	_expect(is_equal_approx(float(state.get("center_freq", 0.0)), frequency),
		label + ": center frequency preserved"
	)


func _check_angles(output_dir: String) -> void:
	var found_angle: bool
	for property: Dictionary in _player.get_property_list():
		if property["name"] == "emission_angle":
			found_angle = true
			var hint: String = String(property["hint_string"])
			_expect(int(property["hint"]) == PROPERTY_HINT_RANGE, "Angle uses a range inspector")
			_expect(hint.contains("degrees") and not hint.contains("radians_as_degrees"),
				"Inspector shows stored degrees without radian conversion"
			)
			_expect(hint.split(",")[1] == "90", "Inspector maximum matches the 90 degree setter clamp")
		elif property["name"] == "emission_angle_filter_attenuation_db":
			var hint: String = String(property["hint_string"])
			_expect(hint.split(",")[1] == "0", "Cone dB inspector maximum matches setter clamp")
	_expect(found_angle, "Angle inspector property is registered")
	_player.emission_angle_enabled = true
	_player.emission_angle = 45.0
	_player.emission_angle_filter_attenuation_db = -18.0
	var cone: Dictionary = _channel.get_3d_cone_settings()
	_expect(is_equal_approx(float(cone["inside_cone_angle"]), 45.0), "45 degrees reaches native cone unchanged")
	_expect(is_equal_approx(float(cone["outside_cone_angle"]), 90.0), "Outer cone uses twice the inner angle")
	_expect(is_equal_approx(float(cone["outside_volume_db"]), -18.0), "Cone attenuation remains in dB")
	var packed: PackedScene = PackedScene.new()
	_expect(packed.pack(_player) == OK, "Player scene packs successfully")
	var scene_path: String = output_dir.path_join("angle_round_trip.tscn")
	_expect(ResourceSaver.save(packed, scene_path) == OK, "Angle scene saved in test logs")
	var loaded: PackedScene = ResourceLoader.load(scene_path, "PackedScene", ResourceLoader.CACHE_MODE_IGNORE) as PackedScene
	var restored: FmodAudioStreamPlayer3D = loaded.instantiate() as FmodAudioStreamPlayer3D
	_expect(restored.emission_angle == 45.0, "Serialized degree value remains compatible")
	restored.free()
	_player.emission_angle = 200.0
	_expect(_player.emission_angle == 90.0, "Angle setter clamps to 90 degrees")
	_player.emission_angle_filter_attenuation_db = 20.0
	_expect(_player.emission_angle_filter_attenuation_db == 0.0, "Cone filter clamps positive dB to unity")


func _measure(distance: float) -> float:
	return _render_rms(_player, _capture, distance)


func _render_rms(player: FmodAudioStreamPlayer3D, capture: FmodAudioEffectCapture, distance: float) -> float:
	player.global_position = Vector3(0.0, 0.0, distance)
	player.notification(Node.NOTIFICATION_PROCESS)
	for block: int in range(32):
		_system.update()
	capture.clear_buffer()
	for block: int in range(8):
		_system.update()
	var samples: PackedVector2Array = capture.get_buffer(capture.get_frames_available())
	var energy: float
	for sample: Vector2 in samples:
		energy += sample.length_squared()
	return sqrt(energy / float(maxi(samples.size(), 1)))


func _check_independent_player(layout: FmodAudioBusLayout, stream: FmodAudioStream) -> void:
	layout.create_audio_bus("Spatial linear peer")
	var bus: FmodAudioBus = layout.get_audio_bus("Spatial linear peer")
	var capture: FmodAudioEffectCapture = FmodAudioEffectCapture.new()
	capture.apply_to(bus.get_bus())
	var peer: FmodAudioStreamPlayer3D = FmodAudioStreamPlayer3D.new()
	peer.bus = &"Spatial linear peer"
	peer.stream = stream
	peer.position = Vector3(0.0, 0.0, 20.0)
	var tree: SceneTree = Engine.get_main_loop() as SceneTree
	tree.root.add_child(peer)
	peer.play()
	_player.attenuation_model = FmodAudioStreamPlayer3D.ATTENUATION_DISABLED
	_measurements["linear_peer_far"] = _render_rms(peer, capture, 20.0)
	_expect(float(_measurements["linear_peer_far"]) < 0.001,
		"Disabling one player does not disable attenuation on other players"
	)
	peer.stop()
	peer.free()
	capture.remove_from_bus(bus.get_bus())
	bus.release()
	layout.remove_audio_bus("Spatial linear peer")


func _same_level(left: float, right: float) -> bool:
	return left > 0.01 and absf(left - right) <= left * 0.02


func _cleanup(layout: FmodAudioBusLayout, bus: FmodAudioBus) -> void:
	_player.stop()
	_player.free()
	_channel = null
	_capture.remove_from_bus(_bus)
	_capture = null
	bus.release()
	layout.remove_audio_bus("Spatial regression")


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("SPATIAL_CHECK_FAILED ", label)
