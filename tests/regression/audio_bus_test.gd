extends RefCounted

var _checks: Array[Dictionary]
var _failed: bool


func run(output_dir: String) -> bool:
	var bus: FmodAudioBus = FmodAudioBus.new()
	for solo: bool in [false, true,]:
		for mute: bool in [false, true,]:
			bus.set_solo(solo)
			bus.set_mute(mute)
			_expect(bus.is_solo() == solo, "is_solo is independent of mute: solo=%s mute=%s" % [solo, mute,])
			_expect(bus.solo == solo and bus.mute == mute, "Property getters match their own flags")
	bus.solo = true
	bus.mute = false
	_expect(bus.is_solo() and not bus.is_mute(), "Property setters preserve independent flags")
	var system: FmodSystem = FmodServer.get_main_system()
	var layout: FmodAudioBusLayout = FmodAudioBusLayout.new()
	layout.create_audio_bus("RoutingParent")
	var parent: FmodAudioBus = layout.get_audio_bus("RoutingParent")
	layout.create_audio_bus("RoutingChild", parent)
	var child: FmodAudioBus = layout.get_audio_bus("RoutingChild")
	layout.create_audio_bus("RoutingGrandchild", child)
	var grandchild: FmodAudioBus = layout.get_audio_bus("RoutingGrandchild")
	layout.create_audio_bus("RoutingRoot")
	var root: FmodAudioBus = layout.get_audio_bus("RoutingRoot")
	_expect(child.get_parent().get_name() == "RoutingParent", "Child caches the requested parent group")
	_expect(child.get_bus().get_parent_group().get_name() == "RoutingParent", "Child is natively routed to requested parent")
	_expect(grandchild.get_bus().get_parent_group().get_name() == "RoutingChild", "Nested parent routing")
	_expect(root.get_bus().get_parent_group().get_name() == system.get_master_channel_group().get_name(), "Omitted parent routes to master")
	layout.set_bus_solo("RoutingChild", true)
	_expect(child.is_solo() and layout.bus_is_solo("RoutingChild"), "Layout and script solo getters agree")
	layout.set_bus_solo("RoutingChild", false)
	layout.set_bus_mute("RoutingChild", true)
	_expect(not child.is_solo() and child.is_mute(), "Layout mute does not masquerade as solo")
	var printed_errors: bool = Engine.print_error_messages
	Engine.print_error_messages = false
	layout.create_audio_bus("InvalidParentChild", bus)
	Engine.print_error_messages = printed_errors
	_expect(not layout.has_audio_bus("InvalidParentChild"), "Uninitialized parent is rejected without publishing a bus")
	var released_parent: FmodAudioBus = FmodAudioBus.new()
	released_parent.init_bus("Released parent")
	released_parent.get_bus().release()
	Engine.print_error_messages = false
	layout.create_audio_bus("ReleasedParentChild", released_parent)
	Engine.print_error_messages = printed_errors
	_expect(not layout.has_audio_bus("ReleasedParentChild"), "Released parent group is rejected without publishing a bus")
	grandchild.release()
	child.release()
	root.release()
	parent.release()
	var report: FileAccess = FileAccess.open(output_dir.path_join("audio_bus_regression.json"), FileAccess.WRITE)
	if report == null:
		return false
	report.store_string(JSON.stringify({"passed": not _failed, "checks": _checks,}, "\t"))
	return not _failed


func _expect(condition: bool, label: String) -> void:
	_checks.append({"name": label, "passed": condition,})
	if not condition:
		_failed = true
		print("BUS_CHECK_FAILED ", label)
