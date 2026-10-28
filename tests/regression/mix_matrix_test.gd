extends RefCounted

const MAX_CHANNELS: int = 32

var _checks: Array[Dictionary]
var _failed: bool


func run(output_dir: String) -> bool:
	var system: FmodSystem = FmodServer.get_main_system()
	var source: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_FADER)
	var sink: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_FADER)
	var connection: FmodDSPConnection = sink.add_input(source)
	var bus: FmodChannelGroup = system.create_channel_group("Matrix regression")
	system.get_master_channel_group().add_group(bus)
	var oscillator: FmodDSP = system.create_dsp_by_type(FmodDSP.DSP_TYPE_OSCILLATOR)
	var channel: FmodChannel = system.play_dsp(oscillator, bus)
	system.update()
	var compact: PackedFloat32Array = PackedFloat32Array([0.25, 0.5, -0.25, 0.75,])
	connection.set_mix_matrix(compact, 2, 2)
	_expect(connection.get_mix_matrix(2, 2) == compact, "Compact connection round trip")
	for hop: int in [4, MAX_CHANNELS,]:
		var padded: PackedFloat32Array
		padded.resize(hop + 2)
		padded.fill(99.0)
		padded[0] = compact[0]
		padded[1] = compact[1]
		padded[hop] = compact[2]
		padded[hop + 1] = compact[3]
		var unchanged: PackedFloat32Array = padded.duplicate()
		connection.set_mix_matrix(padded, 2, 2, hop)
		_expect(padded == unchanged, "Connection setter preserves caller array, hop=%d" % hop)
		_expect(connection.get_mix_matrix(2, 2) == compact, "Padded input compacts correctly, hop=%d" % hop)
		_check_padded(connection.get_mix_matrix(2, 2, hop), compact, 2, 2, hop)
		bus.set_mix_matrix(padded, 2, 2, hop)
		system.update()
		var channel_matrix: Dictionary = bus.get_mix_matrix()
		var rows: int = int(channel_matrix.get("outchannels", 0))
		var columns: int = int(channel_matrix.get("inchannels", 0))
		_expect(rows >= 2 and rows <= MAX_CHANNELS, "ChannelGroup output dimensions are bounded")
		_expect(columns >= 2 and columns <= MAX_CHANNELS, "ChannelGroup input dimensions are bounded")
		_expect(channel_matrix.get("inchannel_hop") == columns, "ChannelGroup getter reports compact stride")
		_check_padded(channel_matrix["matrix"], compact, rows, columns, columns)
		_expect(padded == unchanged, "ChannelGroup setter preserves caller array")
	_check_padded(connection.get_mix_matrix(4, 4, 8), compact, 4, 4, 8)
	_check_rejections(connection, bus, compact)
	for iteration: int in range(256):
		var padded: PackedFloat32Array = connection.get_mix_matrix(2, 2, MAX_CHANNELS)
		if padded.size() != 64 or padded[32] != compact[2] or padded[33] != compact[3]:
			_expect(false, "Repeated padded matrix snapshots")
			break
		if iteration == 255:
			_expect(true, "Repeated padded matrix snapshots")
	var wide: PackedFloat32Array
	wide.resize(MAX_CHANNELS * MAX_CHANNELS)
	for row: int in range(MAX_CHANNELS):
		for column: int in range(MAX_CHANNELS):
			wide[row * MAX_CHANNELS + column] = float(row * MAX_CHANNELS + column) / 1024.0
	connection.set_mix_matrix(wide, MAX_CHANNELS, MAX_CHANNELS)
	_expect(connection.get_mix_matrix(MAX_CHANNELS, MAX_CHANNELS) == wide, "32 by 32 matrix round trip")
	sink.disconnect_all(true, false)
	channel.stop()
	oscillator.release()
	bus.release()
	sink.release()
	source.release()
	var report: FileAccess = FileAccess.open(output_dir.path_join("mix_matrix_regression.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _check_padded(matrix: PackedFloat32Array, compact: PackedFloat32Array, rows: int, columns: int, hop: int) -> void:
	var matched: bool = matrix.size() == rows * hop
	if matched:
		for row: int in range(rows):
			for column: int in range(hop):
				var expected: float
				if row < 2 and column < 2:
					expected = compact[row * 2 + column]
				matched = matched and matrix[row * hop + column] == expected
	_expect(matched, "Snapshot rows=%d columns=%d hop=%d, including zero padding" % [rows, columns, hop,])


func _check_rejections(connection: FmodDSPConnection, bus: FmodChannelGroup, compact: PackedFloat32Array) -> void:
	var bus_before: Dictionary = bus.get_mix_matrix()
	# 只在预期错误调用期间关闭错误输出，所有拒绝结果仍写入检查报告
	var printed_errors: bool = Engine.print_error_messages
	Engine.print_error_messages = false
	var invalid_shapes: Array[Vector3i] = [
		Vector3i(0, 2, 0),
		Vector3i(2, 0, 0),
		Vector3i(-1, 2, 0),
		Vector3i(2, -1, 0),
		Vector3i(33, 2, 0),
		Vector3i(2, 33, 0),
		Vector3i(2, 2, -1),
		Vector3i(2, 2, 1),
		Vector3i(2, 2, 33),
		Vector3i(65536, 65536, 0),
		Vector3i(2147483647, 2147483647, 2147483647),
	]
	for shape: Vector3i in invalid_shapes:
		connection.set_mix_matrix(compact, shape.x, shape.y, shape.z)
		_expect(connection.get_mix_matrix(2, 2) == compact, "Rejected connection setter preserves state: %s" % shape)
		_expect(connection.get_mix_matrix(shape.x, shape.y, shape.z).is_empty(), "Rejected connection getter: %s" % shape)
		bus.set_mix_matrix(compact, shape.x, shape.y, shape.z)
		_expect(bus.get_mix_matrix() == bus_before, "Rejected ChannelGroup setter preserves state: %s" % shape)
	for shape: Vector2i in [Vector2i(1, 2), Vector2i(2, 1), Vector2i(1, 1),]:
		_expect(connection.get_mix_matrix(shape.x, shape.y).is_empty(), "Undersized destination rejected: %s" % shape)
	for count: int in [0, 3, 5,]:
		var short_matrix: PackedFloat32Array
		short_matrix.resize(count)
		var hop: int = 4 if count == 5 else 0
		connection.set_mix_matrix(short_matrix, 2, 2, hop)
		_expect(connection.get_mix_matrix(2, 2) == compact, "Short connection input rejected: %d" % count)
		bus.set_mix_matrix(short_matrix, 2, 2, hop)
		_expect(bus.get_mix_matrix() == bus_before, "Short ChannelGroup input rejected: %d" % count)
	Engine.print_error_messages = printed_errors


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("MATRIX_CHECK_FAILED ", label)
