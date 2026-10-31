@tool
extends RefCounted

const BUS_MESSAGE: String = "Represents an audio bus for mixing with effects support."

var _checks: Array[Dictionary]


func run(output_dir: String, tree: SceneTree) -> bool:
	var original_locale: String = TranslationServer.get_locale()
	var domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.documentation")
	var translation: Translation = load("res://addons/fmod_player/translations/documentation/zh_CN.po") as Translation
	_expect(translation != null, "Documentation PO loads as a Translation resource")
	if translation == null:
		return false
	var plugin: Node = tree.get_first_node_in_group(&"fmod_translation_services")
	_expect(plugin != null, "Main plugin was loaded through the normal editor lifecycle")
	if plugin == null:
		return false
	var sentinel: Translation = Translation.new()
	sentinel.locale = "zh_Hans"
	sentinel.add_message(&"FMOD_DOCUMENTATION_TEST_SENTINEL", &"保留其他插件的翻译")
	domain.add_translation(sentinel)
	TranslationServer.set_locale("zh_Hans")
	await _settle(tree)
	var messages: PackedStringArray = translation.get_message_list()
	var all_matched: bool = true
	for message: String in messages:
		all_matched = all_matched and domain.translate(message) == translation.get_message(message)
	_expect(all_matched and not messages.is_empty(), "All %d PO messages are registered in godot.documentation" % messages.size())
	if FileAccess.file_exists("res://source_messages.json") and messages.size() > 100:
		var source_messages: Array = JSON.parse_string(FileAccess.get_file_as_string("res://source_messages.json")) as Array
		var all_sources_matched: bool = true
		var classes: Dictionary
		for entry: Dictionary in source_messages:
			var source: String = String(entry["text"]).dedent().strip_edges()
			all_sources_matched = all_sources_matched and not translation.get_message(source).is_empty()
			classes[entry["class"]] = true
		_expect(all_sources_matched and classes.size() == 47, "Godot normalized every XML description from all 47 classes to an existing PO key")
	_expect(domain.translate(&"Undocumented FMOD test message") == &"Undocumented FMOD test message", "Missing messages fall back to English")
	await _check_help("FmodAudioBus", "用于混音的音频总线", "bus_zh", output_dir, tree)
	await _check_help("FmodAudioEffectCapture", "丢弃新帧", "capture_zh", output_dir, tree)
	# 模拟引擎重载文档翻译域，再触发插件的延迟注册
	plugin.unregister_catalogs()
	_expect(domain.translate(BUS_MESSAGE) == BUS_MESSAGE, "Removing the plugin catalog restores English")
	EditorInterface.get_editor_settings().emit_signal(&"settings_changed")
	await _settle(tree)
	_expect(domain.translate(BUS_MESSAGE) == translation.get_message(BUS_MESSAGE), "Deferred registration restores translations after a domain reload")
	TranslationServer.set_locale("en")
	await _settle(tree)
	_expect(domain.translate(BUS_MESSAGE) == BUS_MESSAGE, "English editor locale keeps English descriptions")
	await _check_help("FmodAudioBus", BUS_MESSAGE, "bus_en", output_dir, tree)
	TranslationServer.set_locale("zh_CN")
	await _settle(tree)
	_expect(domain.translate(BUS_MESSAGE) == translation.get_message(BUS_MESSAGE), "zh_CN and zh_Hans select the same Simplified Chinese catalog")
	plugin.unregister_catalogs()
	_expect(domain.translate(BUS_MESSAGE) == BUS_MESSAGE, "Plugin cleanup removes its own documentation catalog")
	_expect(domain.translate(&"FMOD_DOCUMENTATION_TEST_SENTINEL") == &"保留其他插件的翻译", "Plugin cleanup preserves unrelated documentation translations")
	plugin.queue_registration()
	await _settle(tree)
	_expect(domain.translate(BUS_MESSAGE) == translation.get_message(BUS_MESSAGE), "Plugin registration works again after cleanup")
	domain.remove_translation(sentinel)
	TranslationServer.set_locale(original_locale)
	await _settle(tree)
	var passed: bool = true
	for check: Dictionary in _checks:
		passed = passed and check["passed"]
	var file: FileAccess = FileAccess.open(output_dir.path_join("documentation_translation.json"), FileAccess.WRITE)
	if file == null:
		return false
	file.store_string(JSON.stringify({
		"passed": passed,
		"messages": messages.size(),
		"checks": _checks,
	}, "\t"))
	return passed


func _check_help(class_name_to_open: String, expected: String, label: String, output_dir: String, tree: SceneTree) -> void:
	var ancestor: Node = EditorInterface.get_script_editor()
	while ancestor != null:
		if ancestor is EditorDock:
			(ancestor as EditorDock).make_visible()
			break
		ancestor = ancestor.get_parent()
	EditorInterface.get_script_editor().goto_help("class_name:" + class_name_to_open)
	await _settle(tree)
	var found: bool
	var text: String
	var candidates: Array[Node] = EditorInterface.get_script_editor().find_children("*", "RichTextLabel", true, false)
	for node: Node in candidates:
		var rich_text: RichTextLabel = node as RichTextLabel
		var candidate: String = rich_text.get_parsed_text()
		if candidate.contains(expected):
			found = true
			text = candidate
			break
	_expect(found, "Native EditorHelp rendered %s" % label)
	if not found:
		print("HELP_DIAGNOSTIC labels=", candidates.size())
		for node: Node in candidates:
			print((node as RichTextLabel).get_parsed_text().left(180))
	_expect(not text.contains(":ref:`") and not text.contains("[param "), "EditorHelp parses documentation links for %s" % label)
	var file: FileAccess = FileAccess.open(output_dir.path_join(label + ".txt"), FileAccess.WRITE)
	if file:
		file.store_string(text)


func _find_main_plugin(root: Node) -> EditorPlugin:
	for node: Node in root.find_children("*", "EditorPlugin", true, false):
		var script: Script = node.get_script() as Script
		if script and script.resource_path.ends_with("/fmod_player_main.gd"):
			return node as EditorPlugin
	return null


func _settle(tree: SceneTree) -> void:
	for frame: int in range(6):
		await tree.process_frame


func _expect(condition: bool, description: String) -> void:
	_checks.append({
		"passed": condition,
		"description": description,
	})
	print("FMOD_DOCUMENTATION_TEST ", condition, " ", description)
