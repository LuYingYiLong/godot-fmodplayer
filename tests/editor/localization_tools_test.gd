@tool
extends RefCounted

const Catalogs: Script = preload("uid://c3j2dr7j8ldm1")
const CAPTURE_HINT: String = "Peak ranges of unread stereo frames in this editor instance. Attach Capture to an FMOD bus to receive audio."
const EXPECTED: Dictionary = {
	"zh_Hans": ["FMOD 本地化", "音量（dB）"],
	"zh_Hant": ["FMOD 本地化", "音量（dB）"],
	"ja": ["FMOD ローカライズ", "音量（dB）"],
	"ru": ["Локализация FMOD", "Громкость (дБ)"],
}

var _checks: Array[Dictionary]


func run(output_dir: String, tree: SceneTree) -> bool:
	var service: Node = tree.get_first_node_in_group(&"fmod_translation_services")
	_expect(service != null, "Bootstrap owns the shared translation service")
	if service == null:
		return false
	var original_locale: String = TranslationServer.get_locale()
	var domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.editor")
	var properties: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.properties")
	for catalog: Dictionary in Catalogs.CATALOGS:
		if catalog["format"] == "csv":
			var data: Dictionary = Catalogs.read_csv(catalog["path"])
			_expect(data["errors"].is_empty(), "CSV catalog validates: " + catalog["name"])
			var complete: bool = true
			for entry: Dictionary in data["entries"]:
				for locale: String in EXPECTED:
					complete = complete and not String(entry["translations"].get(locale, "")).is_empty()
			_expect(complete, "Four languages are complete: " + catalog["name"])
	var capture: Control
	var capture_effect: Resource
	if ClassDB.class_exists(&"FmodAudioEffectCapture"):
		capture_effect = ClassDB.instantiate(&"FmodAudioEffectCapture") as Resource
		EditorInterface.inspect_object(capture_effect)
		await _settle(tree)
		var candidates: Array[Node] = EditorInterface.get_base_control().find_children("*", "FmodAudioEffectCapturePreview", true, false)
		_expect(not candidates.is_empty(), "Inspector creates the native effect preview")
		if not candidates.is_empty():
			capture = candidates[0] as Control
	for locale: String in EXPECTED:
		TranslationServer.set_locale(locale)
		service.queue_registration()
		await _settle(tree)
		_expect(domain.translate(&"FMOD Translations") == EXPECTED[locale][0], "Editor domain: " + locale)
		_expect(properties.translate(&"Volume Db") == EXPECTED[locale][1], "Properties domain: " + locale)
		if capture:
			_expect(capture.tooltip_text == domain.translate(CAPTURE_HINT), "Constructor tooltip and locale refresh: " + locale)
			_expect(capture.tooltip_text != CAPTURE_HINT, "Preview tooltip is localized: " + locale)
		_expect(String(service.get_parent().call(&"_get_warning_text")).contains("addons/fmod_player/bin/"), "Runtime guidance preserves the installation path: " + locale)
	TranslationServer.set_locale("zh_Hans")
	await _settle(tree)
	if ClassDB.class_exists(&"FmodSystem"):
		_test_retained_sounds()
	await _test_csv_and_panel(output_dir, tree, service)
	TranslationServer.set_locale(original_locale)
	service.queue_registration()
	await _settle(tree)
	var passed: bool = true
	for check: Dictionary in _checks:
		passed = passed and check["passed"]
	var file: FileAccess = FileAccess.open(output_dir.path_join("localization_tools.json"), FileAccess.WRITE)
	file.store_string(JSON.stringify({"passed": passed, "checks": _checks}, "\t"))
	return passed


func _test_retained_sounds() -> void:
	var factory: RefCounted = ClassDB.instantiate(&"FmodSystem") as RefCounted
	var retained: Array[RefCounted]
	var wav: PackedByteArray
	wav.resize(44 + 512)
	wav.encode_u32(0, 0x46464952)
	wav.encode_u32(4, wav.size() - 8)
	wav.encode_u32(8, 0x45564157)
	wav.encode_u32(12, 0x20746d66)
	wav.encode_u32(16, 16)
	wav.encode_u16(20, 1)
	wav.encode_u16(22, 1)
	wav.encode_u32(24, 8000)
	wav.encode_u32(28, 16000)
	wav.encode_u16(32, 2)
	wav.encode_u16(34, 16)
	wav.encode_u32(36, 0x61746164)
	wav.encode_u32(40, 512)
	for iteration: int in range(4):
		var system: RefCounted = factory.call(&"create_system") as RefCounted
		system.call(&"set_output", ClassDB.class_get_integer_constant(&"FmodSystem", &"FMOD_OUTPUT_TYPE_NOSOUND_NRT"))
		system.call(&"init", 16, ClassDB.class_get_integer_constant(&"FmodSystem", &"FMOD_INIT_FLAG_STREAM_FROM_UPDATE"))
		var sound: RefCounted = system.call(&"create_sound_from_memory", wav, 0) as RefCounted
		_expect(sound != null, "Lifetime fixture creates a native sound")
		retained.append(sound)
		system.call(&"release")
	retained.clear()
	_expect(true, "Cached sounds from released systems can be destroyed after native handle reuse")


func _test_csv_and_panel(output_dir: String, tree: SceneTree, service: Node) -> void:
	var fixture: String = output_dir.path_join("translations_fixture.csv")
	var data: Dictionary = {
		"locales": PackedStringArray(["zh_Hans", "zh_Hant", "ja", "ru"]),
		"entries": [
			{"source": "First %s", "context": "", "translations": {"zh_Hans": "第一 %s", "zh_Hant": "第一 %s", "ja": "最初 %s", "ru": "Первый %s"}},
			{"source": "Unread", "context": "", "translations": {}},
			{"source": "Line\n%s", "context": "sample", "translations": {"zh_Hans": "行\n%s"}},
		],
	}
	_expect(Catalogs.write_csv(fixture, data) == OK, "CSV writes atomically")
	var restored: Dictionary = Catalogs.read_csv(fixture)
	_expect(restored["entries"].size() == 3, "Empty translations remain editable after a CSV round trip")
	_expect(restored["entries"][2]["source"] == "Line\n%s", "CSV preserves multiline keys without C-style unescaping")
	_expect(restored["entries"][2]["context"] == "sample", "CSV preserves message context")
	var translations: Array[Translation] = Catalogs.create_csv_translations(restored)
	_expect(translations[0].get_message(&"Line\n%s", &"sample") == &"行\n%s", "CSV context is used when creating Translation messages")
	var invalid: Dictionary = data.duplicate(true)
	invalid["locales"] = PackedStringArray(["ja", "ja_2"])
	_expect(not Catalogs.validate_csv(invalid).is_empty(), "Invalid locale suffixes are rejected")
	invalid = data.duplicate(true)
	invalid["entries"][0]["translations"]["zh_Hans"] = "丢失占位符"
	_expect(not Catalogs.validate_csv(invalid).is_empty(), "Placeholder loss is rejected before saving")
	invalid = data.duplicate(true)
	invalid["entries"].append(invalid["entries"][0].duplicate(true))
	_expect(not Catalogs.validate_csv(invalid).is_empty(), "Duplicate keys are rejected")
	service.get_parent().call(&"_open_translation_tools")
	await _settle(tree)
	var panel: Control
	for node: Node in EditorInterface.get_base_control().find_children("*", "VBoxContainer", true, false):
		var script: Script = node.get_script() as Script
		if script and script.resource_path.ends_with("/translation_editor.gd"):
			panel = node as Control
			break
	_expect(panel != null, "Tools menu opens the shared translation panel")
	if panel == null:
		return
	_expect(panel.get_translation_domain() == &"godot.editor", "Tool panel uses the editor domain")
	_expect(panel.get_theme_font(&"font", &"Label") == EditorInterface.get_base_control().get_theme_font(&"font", &"Label"), "Panel inherits the editor theme font")
	_expect((panel.get("_source_root") as LineEdit).placeholder_text != "Source repository (required for extraction)", "Path input keeps a localized placeholder while its contents stay untranslated")
	_expect((panel.get("_locations") as Label).max_lines_visible == 2 and (panel.get("_status") as Label).max_lines_visible == 3, "Long source locations and backend errors have bounded display height")
	panel.set("_path", fixture)
	panel.set("_data", restored)
	panel.set("_checksum", FileAccess.get_sha256(fixture))
	panel.set("_selected", 0)
	panel.call(&"_show_entry")
	(panel.get_node("Toolbar/AddEntry") as Button).emit_signal(&"pressed")
	_expect((panel.get("_source") as TextEdit).text.is_empty(), "Adding an entry starts with empty source text")
	_expect(panel.get("_data")["entries"][0]["source"] == "First %s", "Adding an entry preserves existing rows")
	(panel.get_node("Toolbar/DeleteEntry") as Button).emit_signal(&"pressed")
	_expect(panel.get("_data")["entries"].size() == 3, "Deleting an entry removes only the selected row")
	panel.set("_selected", 0)
	panel.call(&"_show_entry")
	var target: TextEdit = panel.get("_target") as TextEdit
	target.text = "修改 %s"
	target.emit_signal(&"text_changed")
	var languages: OptionButton = panel.get("_language") as OptionButton
	languages.select(1)
	panel.call(&"_language_selected", 1)
	_expect(target.text == "第一 %s", "Changing languages does not overwrite the next language")
	languages.select(0)
	panel.call(&"_language_selected", 0)
	_expect(target.text == "修改 %s", "Unsaved edits remain associated with their original language")
	_expect(panel.call(&"save"), "Panel saves and refreshes the shared loader")
	_expect(Catalogs.read_csv(fixture)["entries"][0]["translations"]["zh_Hans"] == "修改 %s", "Panel edit was saved to CSV")
	target.text = "待保存 %s"
	target.emit_signal(&"text_changed")
	(panel.get("_catalog") as OptionButton).emit_signal(&"item_selected", 1)
	var unsaved: Window = panel.get("_unsaved_dialog") as Window
	_expect(unsaved.visible and panel.get("_index") == 0, "Scene catalog signal opens the unsaved prompt before switching")
	var actions: Node = unsaved.get_node("Panel/Margin/Content/Actions")
	(actions.get_node("Cancel") as Button).emit_signal(&"pressed")
	_expect(not unsaved.visible and panel.get("_dirty") and panel.get("_index") == 0, "Scene Cancel button preserves unsaved edits and the current catalog")
	panel.call(&"_catalog_selected", 1)
	(actions.get_node("SaveChanges") as Button).emit_signal(&"pressed")
	_expect(not unsaved.visible and panel.get("_index") == 1, "Scene Save button saves the fixture and switches catalogs")
	_expect(Catalogs.read_csv(fixture)["entries"][0]["translations"]["zh_Hans"] == "待保存 %s", "Unsaved prompt saved the edited translation")
	panel.set("_dirty", true)
	panel.call(&"_catalog_selected", 0)
	(actions.get_node("Discard") as Button).emit_signal(&"pressed")
	_expect(not unsaved.visible and not panel.get("_dirty") and panel.get("_index") == 0, "Scene Discard button switches catalogs without saving")
	panel.set("_path", fixture)
	panel.set("_data", Catalogs.read_csv(fixture))
	panel.set("_checksum", FileAccess.get_sha256(fixture))
	panel.set("_selected", 0)
	panel.call(&"_show_entry")
	var file: FileAccess = FileAccess.open(fixture, FileAccess.READ_WRITE)
	file.seek_end()
	file.store_string("\n")
	file.close()
	target.text = "后续 %s"
	target.emit_signal(&"text_changed")
	_expect(not panel.call(&"save"), "External file changes prevent overwriting newer data")
	_expect(panel.get("_dirty"), "A failed save keeps the unsaved state")
	panel.set("_dirty", false)
	panel.call(&"_load_catalog", 2)
	await _settle(tree)
	var entries: Array = panel.get("_po_entries")
	_expect(entries.size() > 1500, "PO editor loads source, translation, review flags and locations")
	_expect(not (panel.get("_source") as TextEdit).editable, "PO source keys are read-only")
	_expect((panel.get("_fuzzy") as CheckBox).visible, "PO review state is editable")
	panel.set("_selected", 0)
	panel.call(&"_show_entry")
	var po_fixture: String = output_dir.path_join("documentation_fixture.po")
	var original: String = FileAccess.get_file_as_string(panel.get("_path"))
	file = FileAccess.open(po_fixture, FileAccess.WRITE)
	file.store_string(original)
	file.close()
	panel.set("_path", po_fixture)
	panel.set("_checksum", FileAccess.get_sha256(po_fixture))
	(panel.get("_fuzzy") as CheckBox).button_pressed = true
	_expect(panel.call(&"save"), "PO edits preserve the catalog through the Babel backend")
	var round_trip: Dictionary = panel.call(&"_backend", "po-read")
	_expect(round_trip.get("ok", false) and round_trip["data"]["entries"][0]["fuzzy"], "PO fuzzy flag survives a save and reload")
	_expect(FileAccess.get_file_as_string(po_fixture).contains("#. Class reference template"), "PO source comments survive editing")
	panel.get_parent().hide()


func _settle(tree: SceneTree) -> void:
	for frame: int in range(6):
		await tree.process_frame


func _expect(condition: bool, description: String) -> void:
	_checks.append({"passed": condition, "description": description})
	print("FMOD_LOCALIZATION_TEST ", condition, " ", description)
