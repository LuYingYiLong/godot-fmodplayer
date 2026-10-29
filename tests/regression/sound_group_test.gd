extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool


func run(output_dir: String) -> bool:
	var system: FmodSystem = FmodServer.get_main_system()
	var group: FmodSoundGroup = system.create_sound_group("Sound group regression")
	_expect(group.get_max_audible() == -1, "New native sound group defaults to unlimited")
	group.set_max_audible(2)
	_expect(group.get_max_audible() == 2, "Method sets a finite limit")
	var printed_errors: bool = Engine.print_error_messages
	Engine.print_error_messages = false
	group.set_max_audible(-1)
	Engine.print_error_messages = printed_errors
	_expect(group.get_max_audible() == -1, "Method cancels a finite limit with -1")
	group.max_audible = 0
	_expect(group.max_audible == 0, "Property accepts zero")
	Engine.print_error_messages = false
	group.max_audible = -1
	Engine.print_error_messages = printed_errors
	_expect(group.max_audible == -1, "Property cancels a zero limit with -1")
	for limit: int in [-1, 3,]:
		Engine.print_error_messages = false
		group.set_max_audible(limit)
		Engine.print_error_messages = printed_errors
		for invalid: int in [-2, -2147483648,]:
			Engine.print_error_messages = false
			group.set_max_audible(invalid)
			group.max_audible = invalid
			Engine.print_error_messages = printed_errors
			_expect(group.get_max_audible() == limit,
				"Invalid %d preserves limit %d through method and property" % [invalid, limit,]
			)
	group.release()
	_test_playback(system, output_dir)
	var report: FileAccess = FileAccess.open(
		output_dir.path_join("sound_group_regression.json"), FileAccess.WRITE
	)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _test_playback(system: FmodSystem, output_dir: String) -> void:
	var sound: FmodSound = _create_sound(system, output_dir)
	_expect(sound != null, "Generated looping PCM fixture loads")
	if sound == null:
		return
	# 新声音默认属于主声音组，保存并还原设置，避免影响后续套件
	var group: FmodSoundGroup = system.get_master_sound_group()
	var previous_limit: int = group.max_audible
	var previous_behavior: int = group.behavior
	var bus: FmodChannelGroup = system.create_channel_group("Sound group playback")
	system.get_master_channel_group().add_group(bus)
	group.behavior = FmodSoundGroup.BEHAVIOR_FAIL
	group.set_max_audible(1)
	_play(system, sound, bus, true, "Finite limit allows first playback")
	_play(system, sound, bus, false, "Finite limit rejects second playback")
	_expect(group.get_num_playing() == 1, "Finite limit has one native playback")
	var printed_errors: bool = Engine.print_error_messages
	Engine.print_error_messages = false
	group.set_max_audible(-1)
	Engine.print_error_messages = printed_errors
	for index: int in range(5):
		_play(system, sound, bus, true, "Method cancellation allows extra playback %d" % index)
	_expect(group.get_num_playing() == 6, "Unlimited group exceeds its previous limit")
	bus.stop()
	system.update()
	group.max_audible = 0
	_play(system, sound, bus, false, "Zero still rejects playback")
	Engine.print_error_messages = false
	group.max_audible = -1
	Engine.print_error_messages = printed_errors
	_play(system, sound, bus, true, "Property cancellation restores playback after zero")
	_play(system, sound, bus, true, "Property cancellation allows multiple playbacks")
	_expect(group.get_num_playing() == 2, "Property cancellation is applied natively")
	bus.stop()
	system.update()
	group.set_max_audible(1)
	_play(system, sound, bus, true, "Finite limit can be restored after unlimited")
	_play(system, sound, bus, false, "Restored finite limit rejects excess playback")
	bus.stop()
	system.update()
	Engine.print_error_messages = false
	group.max_audible = previous_limit
	Engine.print_error_messages = printed_errors
	group.behavior = previous_behavior
	sound = null
	bus.release()


func _create_sound(system: FmodSystem, output_dir: String) -> FmodSound:
	var pcm: PackedByteArray
	pcm.resize(48000 * 2)
	for frame: int in range(48000):
		var sample: float = 0.25 * sin(TAU * 440.0 * float(frame) / 48000.0)
		pcm.encode_s16(frame * 2, int(sample * 32767.0))
	var wav: AudioStreamWAV = AudioStreamWAV.new()
	wav.format = AudioStreamWAV.FORMAT_16_BITS
	wav.mix_rate = 48000
	wav.data = pcm
	var path: String = output_dir.path_join("sound_group_tone.wav")
	if wav.save_to_wav(path) != OK:
		return null
	return system.create_sound_from_memory(
		FileAccess.get_file_as_bytes(path), FmodSystem.FMOD_MODE_LOOP_NORMAL
	)


func _play(
	system: FmodSystem, sound: FmodSound, bus: FmodChannelGroup, expected: bool, label: String
) -> void:
	var printed_errors: bool = Engine.print_error_messages
	Engine.print_error_messages = false
	var channel: FmodChannel = system.play_sound(sound, bus)
	Engine.print_error_messages = printed_errors
	_expect((channel != null) == expected, label)
	system.update()


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("SOUND_GROUP_CHECK_FAILED ", label)
