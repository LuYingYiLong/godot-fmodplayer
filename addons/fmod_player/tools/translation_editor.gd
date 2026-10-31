@tool
extends VBoxContainer

const Catalogs: Script = preload("res://addons/fmod_player/translations/fmod_translation_catalogs.gd")
const TranslationService: Script = preload("res://addons/fmod_player/translations/fmod_translation_service.gd")

var _data: Dictionary
var _po_entries: Array[Dictionary]
var _index: int
var _selected: int = -1
var _pending_catalog: int = -1
var _path: String
var _checksum: String
var _current_locale: String
var _dirty: bool
var _updating: bool
var _hint_sources: Dictionary

@onready var _catalog: OptionButton = %Catalog
@onready var _language: OptionButton = %Language
@onready var _search: LineEdit = %Search
@onready var _missing_only: CheckButton = %MissingOnly
@onready var _tree: Tree = %Entries
@onready var _source: TextEdit = %Source
@onready var _target: TextEdit = %Translation
@onready var _context: LineEdit = %Context
@onready var _fuzzy: CheckBox = %Fuzzy
@onready var _locations: Label = %Locations
@onready var _status: Label = %Status
@onready var _source_root: LineEdit = %SourceRoot
@onready var _python: LineEdit = %Python
@onready var _add_button: Button = %AddEntry
@onready var _delete_button: Button = %DeleteEntry
@onready var _extract_button: Button = %ExtractSources
@onready var _language_name: LineEdit = %Locale
@onready var _add_language_button: Button = %AddLanguage
@onready var _unsaved_dialog: Window = %UnsavedDialog
@onready var _filter_timer: Timer = %FilterTimer


func _ready() -> void:
	for control: Control in [_source, _target, _context, _source_root, _language_name, _python]:
		var property: StringName = &"tooltip_text" if control == _python else &"placeholder_text"
		_hint_sources[control] = {"property": property, "source": control.get(property)}
	if get_tree().get_first_node_in_group(&"fmod_translation_services") == null:
		add_child(TranslationService.new())
	_source_root.text = _find_source_root()
	var configuration: ConfigFile = ConfigFile.new()
	if configuration.load("user://fmod_localization.cfg") == OK:
		_source_root.text = configuration.get_value("tools", "source_root", _source_root.text)
		_python.text = configuration.get_value("tools", "python", _python.text)
	_tree.set_column_title(0, tr("Source"))
	_tree.set_column_title(1, tr("Translation"))
	_refresh_editable_hints()
	_load_catalog(0)


func _exit_tree() -> void:
	if _source_root and _python:
		var configuration: ConfigFile = ConfigFile.new()
		configuration.set_value("tools", "source_root", _source_root.text)
		configuration.set_value("tools", "python", _python.text)
		configuration.save("user://fmod_localization.cfg")


func _notification(what: int) -> void:
	if what == NOTIFICATION_TRANSLATION_CHANGED and _tree:
		_tree.set_column_title(0, tr("Source"))
		_tree.set_column_title(1, tr("Translation"))
		_refresh_editable_hints()
		if _dirty:
			_set_status(tr("Unsaved changes"))


func save() -> bool:
	_flush_entry()
	if FileAccess.get_sha256(_path) != _checksum:
		_set_status(tr("The file changed on disk. Reload it before saving."))
		return false
	if _is_po():
		var temporary: String = "user://fmod_localization_edits.json"
		var file: FileAccess = FileAccess.open(temporary, FileAccess.WRITE)
		if file == null:
			return false
		file.store_string(JSON.stringify(_po_entries))
		file.close()
		var result: Dictionary = _backend("po-write", PackedStringArray(["--input", ProjectSettings.globalize_path(temporary), "--expected", _checksum]))
		if not result.get("ok", false):
			return false
	else:
		var errors: Array[String] = Catalogs.validate_csv(_data)
		if not errors.is_empty():
			_set_status("\n".join(errors))
			return false
		var error: Error = Catalogs.write_csv(_path, _data)
		if error != OK:
			_set_status(tr("Cannot save file: %s") % error_string(error))
			return false
	_checksum = FileAccess.get_sha256(_path)
	_dirty = false
	for service: Node in get_tree().get_nodes_in_group(&"fmod_translation_services"):
		if not service.reload_catalogs():
			_set_status("\n".join(service.last_errors))
			return false
	_set_status(tr("Saved and reloaded translations."))
	return true


func _search_text_changed(_value: String) -> void:
	_filter_timer.start()


func _missing_only_toggled(_pressed: bool) -> void:
	_rebuild_tree()


func _context_text_changed(_value: String) -> void:
	_entry_changed()


func _fuzzy_toggled(_pressed: bool) -> void:
	_entry_changed()


func _source_root_text_changed(value: String) -> void:
	_extract_button.disabled = _index != 0 and not FileAccess.file_exists(value.path_join("tools/documentation_translations.py"))


func _save_before_switch() -> void:
	if save():
		_unsaved_dialog.hide()
		_load_catalog(_pending_catalog)


func _discard_before_switch() -> void:
	_unsaved_dialog.hide()
	_load_catalog(_pending_catalog)


func _unsaved_dialog_input(event: InputEvent) -> void:
	if event.is_action_pressed(&"ui_cancel"):
		_unsaved_dialog.hide()
		_unsaved_dialog.set_input_as_handled()


func _show_unsaved_dialog() -> void:
	_unsaved_dialog.popup_centered()
	(%SaveChanges as Button).grab_focus()


func _refresh_editable_hints() -> void:
	for control: Control in _hint_sources:
		var hint: Dictionary = _hint_sources[control]
		control.set(hint["property"], tr(hint["source"]))


func _is_po() -> bool:
	return Catalogs.CATALOGS[_index]["format"] == "po"


func _catalog_selected(index: int) -> void:
	if _dirty:
		_catalog.select(_index)
		_pending_catalog = index
		_show_unsaved_dialog()
	else:
		_load_catalog(index)


func _load_catalog(index: int) -> void:
	_index = index
	_catalog.select(index)
	_extract_button.disabled = index != 0 and not FileAccess.file_exists(_source_root.text.path_join("tools/documentation_translations.py"))
	_selected = -1
	_dirty = false
	_language.clear()
	var catalog: Dictionary = Catalogs.CATALOGS[index]
	var paths: PackedStringArray = Catalogs.paths(catalog)
	if paths.is_empty():
		_set_status(tr("No translation files found."))
		return
	if _is_po():
		for path: String in paths:
			_language.add_item(path.get_file().get_basename())
			_language.set_item_metadata(_language.item_count - 1, path)
		_path = paths[0]
		_load_po()
	else:
		_path = paths[0]
		_data = Catalogs.read_csv(_path)
		for locale: String in _data.get("locales", []):
			_language.add_item(locale)
		_current_locale = _language.get_item_text(0) if _language.item_count > 0 else ""
	_add_button.disabled = _is_po()
	_delete_button.disabled = _is_po()
	_add_language_button.disabled = _is_po()
	_source.editable = not _is_po()
	_context.editable = not _is_po()
	_fuzzy.visible = _is_po()
	_checksum = FileAccess.get_sha256(_path)
	_rebuild_tree()
	_set_status("\n".join(_data.get("errors", [])) if not _is_po() and not _data.get("errors", []).is_empty() else _path)


func _load_po() -> void:
	_po_entries.clear()
	var result: Dictionary = _backend("po-read")
	if result.get("ok", false):
		for entry: Dictionary in result["data"]["entries"]:
			_po_entries.append(entry)


func _language_selected(index: int) -> void:
	_flush_entry()
	if _is_po():
		if _dirty:
			_set_status(tr("Save or reload changes before changing the PO file."))
			for item: int in range(_language.item_count):
				if _language.get_item_metadata(item) == _path:
					_language.select(item)
			return
		_path = _language.get_item_metadata(index)
		_selected = -1
		_load_po()
		_checksum = FileAccess.get_sha256(_path)
	else:
		_current_locale = _language.get_item_text(index)
		_show_entry()
	_rebuild_tree()


func _entries() -> Array:
	return _po_entries if _is_po() else _data.get("entries", [])


func _entry_translation(entry: Dictionary) -> String:
	if _is_po():
		return String(entry.get("translation", ""))
	if _language.selected < 0:
		return ""
	return String(entry.get("translations", {}).get(_current_locale, ""))


func _rebuild_tree() -> void:
	_flush_entry()
	_updating = true
	_tree.clear()
	var root: TreeItem = _tree.create_item()
	var entries: Array = _entries()
	for index: int in range(entries.size()):
		var entry: Dictionary = entries[index]
		var value: String = _entry_translation(entry)
		if _missing_only.button_pressed and not value.is_empty() and not entry.get("fuzzy", false):
			continue
		if not _search.text.is_empty() and not (String(entry["source"]) + value).containsn(_search.text):
			continue
		var item: TreeItem = _tree.create_item(root)
		item.set_text(0, String(entry["source"]).replace("\n", " ").left(120))
		item.set_text(1, value.replace("\n", " ").left(120))
		item.set_tooltip_text(0, entry["source"])
		item.set_tooltip_text(1, value)
		item.set_metadata(0, index)
		if index == _selected:
			item.select(0)
	_updating = false
	if _selected < 0:
		_show_entry()


func _entry_selected() -> void:
	if _updating:
		return
	_flush_entry()
	_selected = int(_tree.get_selected().get_metadata(0))
	_show_entry()


func _show_entry() -> void:
	_updating = true
	var entry: Dictionary = _entries()[_selected] if _selected >= 0 and _selected < _entries().size() else {}
	_source.text = entry.get("source", "")
	_target.text = _entry_translation(entry)
	_context.text = entry.get("context", "")
	_fuzzy.button_pressed = entry.get("fuzzy", false)
	_locations.text = ", ".join(entry.get("locations", []))
	_locations.tooltip_text = _locations.text
	_updating = false


func _flush_entry() -> void:
	if _updating or _selected < 0 or _selected >= _entries().size():
		return
	var entry: Dictionary = _entries()[_selected]
	if _is_po():
		entry["translation"] = _target.text
		entry["fuzzy"] = _fuzzy.button_pressed
	elif _language.selected >= 0:
		entry["source"] = _source.text
		entry["context"] = _context.text
		entry["translations"][_current_locale] = _target.text


func _entry_changed() -> void:
	if _updating or _selected < 0:
		return
	_dirty = true
	_flush_entry()
	var item: TreeItem = _tree.get_selected()
	if item:
		item.set_text(0, _source.text.replace("\n", " ").left(120))
		item.set_text(1, _target.text.replace("\n", " ").left(120))
		item.set_tooltip_text(0, _source.text)
		item.set_tooltip_text(1, _target.text)
	_set_status(tr("Unsaved changes"))


func _add_entry() -> void:
	_flush_entry()
	var values: Dictionary
	for locale: String in _data["locales"]:
		values[locale] = ""
	_data["entries"].append({"source": "", "context": "", "translations": values})
	_selected = _data["entries"].size() - 1
	_dirty = true
	_search.clear()
	_show_entry()
	_rebuild_tree()
	_source.grab_focus()


func _delete_entry() -> void:
	if _selected >= 0:
		_data["entries"].remove_at(_selected)
		_selected = -1
		_dirty = true
		_rebuild_tree()
		_set_status(tr("Unsaved changes"))


func _add_language() -> void:
	_flush_entry()
	var locale: String = _language_name.text.strip_edges()
	var candidate: Dictionary = _data.duplicate(true)
	var locales: PackedStringArray = PackedStringArray(candidate["locales"])
	locales.append(locale)
	candidate["locales"] = locales
	var errors: Array[String] = Catalogs.validate_csv(candidate)
	if not errors.is_empty():
		_set_status("\n".join(errors))
		return
	_data = candidate
	for entry: Dictionary in _data["entries"]:
		entry["translations"][locale] = ""
	_language.add_item(locale)
	_dirty = true
	_language_name.clear()
	_set_status(tr("Unsaved changes"))


func _reload_requested() -> void:
	if _dirty:
		_pending_catalog = _index
		_show_unsaved_dialog()
	else:
		_load_catalog(_index)


func _extract_sources() -> void:
	if _dirty:
		_set_status(tr("Save changes before extracting sources."))
		return
	var result: Dictionary = {"ok": true, "data": {"entries": _property_entries()}} if _index == 0 else _backend("extract" if _is_po() else "scan")
	if not result.get("ok", false):
		return
	if _is_po():
		_load_catalog(_index)
	else:
		var existing: Dictionary
		for entry: Dictionary in _data["entries"]:
			existing[[entry["source"], entry.get("context", "")]] = entry
		var candidates: Array = result["data"]["entries"] if _index == 1 else _property_entries()
		for entry: Dictionary in candidates:
			var key: Array = [entry["source"], entry.get("context", "")]
			if not existing.has(key):
				entry["translations"] = {}
				_data["entries"].append(entry)
				existing[key] = entry
			else:
				existing[key]["locations"] = entry.get("locations", [])
		_dirty = true
		_rebuild_tree()
		_set_status(tr("Extracted sources. Review new entries before saving."))


func _property_entries() -> Array[Dictionary]:
	var result: Array[Dictionary]
	for class_name_to_scan: StringName in ClassDB.get_class_list():
		if String(class_name_to_scan).begins_with("Fmod"):
			for property: Dictionary in ClassDB.class_get_property_list(class_name_to_scan, true):
				if property["type"] != TYPE_NIL:
					var leaf: String = String(property["name"]).get_file()
					result.append({"source": leaf.capitalize(), "context": "", "locations": [String(class_name_to_scan)]})
	return result


func _backend(action: String, extra: PackedStringArray = PackedStringArray()) -> Dictionary:
	var result_path: String = "user://fmod_localization_result_%d.json" % Time.get_ticks_usec()
	var arguments: PackedStringArray = PackedStringArray([
		"-X", "utf8",
		ProjectSettings.globalize_path("res://addons/fmod_player/tools/localization_backend.py"),
		action, "--path", ProjectSettings.globalize_path(_path),
		"--output", ProjectSettings.globalize_path(result_path),
	])
	if not _source_root.text.is_empty():
		arguments.append_array(PackedStringArray(["--root", _source_root.text]))
	arguments.append_array(extra)
	var output: Array
	var exit_code: int = OS.execute(_python.text, arguments, output, true, false)
	var parser: JSON = JSON.new()
	var contents: String = FileAccess.get_file_as_string(result_path) if FileAccess.file_exists(result_path) else ""
	var error_code: Error = parser.parse(contents)
	var parsed: Variant = parser.data if error_code == OK else null
	if FileAccess.file_exists(result_path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(result_path))
	if exit_code != 0 or not parsed is Dictionary or not parsed.get("ok", false):
		var error: String = str(parsed.get("error", "")) if parsed is Dictionary else "".join(output)
		_set_status(tr("Localization backend failed. Install Python and Babel, then check the source repository path.") + "\n" + error)
		return {"ok": false}
	return parsed as Dictionary


func _find_source_root() -> String:
	var candidate: String = ProjectSettings.globalize_path("res://").trim_suffix("/")
	for depth: int in range(4):
		if FileAccess.file_exists(candidate.path_join("tools/documentation_translations.py")):
			return candidate
		candidate = candidate.get_base_dir()
	return ""


func _set_status(text: String) -> void:
	_status.text = text
	_status.tooltip_text = text

