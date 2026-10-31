@tool
extends RefCounted

const CATALOGS: Array[Dictionary] = [
	{"name": "Properties", "domain": &"godot.properties", "format": "csv", "path": "res://addons/fmod_player/translations/fmod_property_translations.csv"},
	{"name": "Editor Interface", "domain": &"godot.editor", "format": "csv", "path": "res://addons/fmod_player/translations/fmod_editor_translations.csv"},
	{"name": "API Documentation", "domain": &"godot.documentation", "format": "po", "path": "res://addons/fmod_player/translations/documentation"},
]


static func read_csv(path: String) -> Dictionary:
	var errors: Array[String]
	var file: FileAccess = FileAccess.open(path, FileAccess.READ)
	if file == null:
		return {"errors": [_error("Cannot open file: %s") % path]}
	var header: PackedStringArray = file.get_csv_line()
	if header.size() < 2 or header[0] != "source":
		return {"errors": [_error("CSV header must start with source.")]}
	var context_column: int = header.find("context")
	var locales: PackedStringArray
	var columns: Array[int]
	for column: int in range(1, header.size()):
		if column != context_column:
			locales.append(header[column])
			columns.append(column)
	var entries: Array[Dictionary]
	while not file.eof_reached():
		var offset: int = file.get_position()
		var row: PackedStringArray = file.get_csv_line()
		if row.is_empty() or row[0].is_empty():
			continue
		if row.size() != header.size():
			errors.append(_error("CSV column count does not match the header: %s") % row[0])
			continue
		var values: Dictionary
		for index: int in range(locales.size()):
			values[locales[index]] = row[columns[index]]
		entries.append({
			"source": row[0],
			"context": row[context_column] if context_column >= 0 else "",
			"translations": values,
			"locations": [path + " @ " + str(offset)],
		})
	var data: Dictionary = {"locales": locales, "entries": entries, "errors": errors}
	errors.append_array(validate_csv(data))
	return data


static func validate_csv(data: Dictionary) -> Array[String]:
	var errors: Array[String]
	var locales: PackedStringArray = PackedStringArray(data.get("locales", []))
	var locale_pattern: RegEx = RegEx.new()
	locale_pattern.compile("^[a-z]{2,3}(_[A-Za-z]{2,4})?(_[A-Za-z0-9]{2,3})?$")
	if locales.is_empty():
		errors.append(_error("At least one language is required."))
	for index: int in range(locales.size()):
		var locale: String = locales[index]
		if locale_pattern.search(locale) == null or not TranslationServer.get_all_languages().has(locale.get_slice("_", 0)):
			errors.append(_error("Invalid locale: %s") % locale)
		for previous: int in range(index):
			if TranslationServer.compare_locales(locale, locales[previous]) >= 9:
				errors.append(_error("Duplicate locale: %s") % locale)
	var seen: Dictionary
	for entry: Dictionary in data.get("entries", []):
		var source: String = entry.get("source", "")
		var key: Array[String] = [source, String(entry.get("context", ""))]
		if source.is_empty() or seen.has(key):
			errors.append(_error("Empty or duplicate source: %s") % source)
		seen[key] = true
		var values: Dictionary = entry.get("translations", {})
		for locale: String in locales:
			var value: String = String(values.get(locale, ""))
			if not value.is_empty() and _placeholders(source) != _placeholders(value):
				errors.append(_error("Placeholder mismatch (%s): %s") % [locale, source])
	return errors


static func _placeholders(text: String) -> Array[String]:
	var expression: RegEx = RegEx.new()
	expression.compile("%(?:[0-9]+\\$)?[-+#0 ]*[0-9]*(?:\\.[0-9]+)?[sdifuxX]")
	var result: Array[String]
	for match: RegExMatch in expression.search_all(text.replace("%%", "")):
		result.append(match.get_string().right(1))
	result.sort()
	return result


static func create_csv_translations(data: Dictionary) -> Array[Translation]:
	var result: Array[Translation]
	for locale: String in data.get("locales", []):
		var translation: Translation = Translation.new()
		translation.locale = locale
		for entry: Dictionary in data.get("entries", []):
			var values: Dictionary = entry.get("translations", {})
			var value: String = String(values.get(locale, ""))
			if not value.is_empty():
				translation.add_message(StringName(entry["source"]), StringName(value), StringName(entry.get("context", "")))
		result.append(translation)
	return result


static func write_csv(path: String, data: Dictionary) -> Error:
	if not validate_csv(data).is_empty():
		return ERR_INVALID_DATA
	var temporary_path: String = path + ".tmp"
	var file: FileAccess = FileAccess.open(temporary_path, FileAccess.WRITE)
	if file == null:
		return FileAccess.get_open_error()
	var locales: PackedStringArray = PackedStringArray(data["locales"])
	var header: PackedStringArray = PackedStringArray(["source", "context"])
	header.append_array(locales)
	file.store_csv_line(header)
	for entry: Dictionary in data["entries"]:
		var row: PackedStringArray = PackedStringArray([entry["source"], entry.get("context", "")])
		var values: Dictionary = entry["translations"]
		for locale: String in locales:
			row.append(String(values.get(locale, "")))
		file.store_csv_line(row)
	file.flush()
	var error: Error = file.get_error()
	file.close()
	if error != OK:
		return error
	return DirAccess.rename_absolute(ProjectSettings.globalize_path(temporary_path), ProjectSettings.globalize_path(path))


static func paths(catalog: Dictionary) -> PackedStringArray:
	if catalog["format"] == "csv":
		return PackedStringArray([catalog["path"]])
	var result: PackedStringArray
	for filename: String in DirAccess.get_files_at(catalog["path"]):
		if filename.get_extension() == "po":
			result.append(String(catalog["path"]).path_join(filename))
	return result


static func _error(source: String) -> String:
	return TranslationServer.get_or_add_domain(&"godot.editor").translate(source)
