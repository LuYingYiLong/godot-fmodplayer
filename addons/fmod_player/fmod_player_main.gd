@tool
extends EditorPlugin

const FmodPropertyTranslations: Script = preload("res://addons/fmod_player/translations/fmod_property_translations.gd")

# This is the main plugin that actually uses GDExtension classes.
# It will only be loaded after confirming fmod.dll exists.

var importer: EditorImportPlugin
var inspector: EditorInspectorPlugin
var fmod_audio_stream_player_3d_gizom_plugin: EditorNode3DGizmoPlugin
var fmod_reverb_zone_3d_gizmo_plugin: EditorNode3DGizmoPlugin
var fmod_geometry_mesh_tools_plugin: EditorPlugin
var android_build_template_plugin: EditorPlugin
var _editor_interface: EditorInterface
var _fmod_property_translations: Array[Translation]
var _fmod_editor_translations: Array[Translation]


func set_editor_interface(iface: EditorInterface) -> void:
	_editor_interface = iface


func get_editor_interface() -> EditorInterface:
	return _editor_interface


func _enter_tree() -> void:
	# Use load() instead of preload to ensure GDExtension is loaded
	load("res://addons/fmod_player/bin/fmod_player.gdextension")

	# Dynamically instantiate classes
	if ClassDB.class_exists("AudioImporterFmod"):
		importer = ClassDB.instantiate("AudioImporterFmod")
		add_import_plugin(importer)

	if ClassDB.class_exists("FmodAudioPreviewInspector"):
		inspector = ClassDB.instantiate("FmodAudioPreviewInspector")
		add_inspector_plugin(inspector)

	if ClassDB.class_exists("FmodAudioStreamPlayer3DGizmoPlugin"):
		fmod_audio_stream_player_3d_gizom_plugin = ClassDB.instantiate("FmodAudioStreamPlayer3DGizmoPlugin")
		add_node_3d_gizmo_plugin(fmod_audio_stream_player_3d_gizom_plugin)

	if ClassDB.class_exists("FmodReverbZone3DGizmoPlugin"):
		fmod_reverb_zone_3d_gizmo_plugin = ClassDB.instantiate("FmodReverbZone3DGizmoPlugin")
		add_node_3d_gizmo_plugin(fmod_reverb_zone_3d_gizmo_plugin)

	if ClassDB.class_exists("FmodGeometryMeshToolsPlugin"):
		fmod_geometry_mesh_tools_plugin = ClassDB.instantiate("FmodGeometryMeshToolsPlugin")
		add_child(fmod_geometry_mesh_tools_plugin)

	if ClassDB.class_exists("ModifyAndroidBuildTemplatePlugin"):
		android_build_template_plugin = ClassDB.instantiate("ModifyAndroidBuildTemplatePlugin")
		add_child(android_build_template_plugin)

	var editor_settings: EditorSettings = get_editor_interface().get_editor_settings()
	if editor_settings and not editor_settings.settings_changed.is_connected(_queue_fmod_property_translations_update):
		editor_settings.settings_changed.connect(_queue_fmod_property_translations_update)

	_queue_fmod_property_translations_update()


func _exit_tree() -> void:
	var editor_settings: EditorSettings = get_editor_interface().get_editor_settings() if get_editor_interface() else null
	if editor_settings and editor_settings.settings_changed.is_connected(_queue_fmod_property_translations_update):
		editor_settings.settings_changed.disconnect(_queue_fmod_property_translations_update)

	_remove_fmod_property_translations()

	if fmod_geometry_mesh_tools_plugin:
		fmod_geometry_mesh_tools_plugin.queue_free()
		fmod_geometry_mesh_tools_plugin = null
	if android_build_template_plugin:
		android_build_template_plugin.queue_free()
		android_build_template_plugin = null
	if importer:
		remove_import_plugin(importer)
		importer = null
	if inspector:
		remove_inspector_plugin(inspector)
		inspector = null
	if fmod_audio_stream_player_3d_gizom_plugin:
		remove_node_3d_gizmo_plugin(fmod_audio_stream_player_3d_gizom_plugin)
		fmod_audio_stream_player_3d_gizom_plugin = null
	if fmod_reverb_zone_3d_gizmo_plugin:
		remove_node_3d_gizmo_plugin(fmod_reverb_zone_3d_gizmo_plugin)
		fmod_reverb_zone_3d_gizmo_plugin = null


func _queue_fmod_property_translations_update() -> void:
	call_deferred("_add_fmod_property_translations")


func _add_fmod_property_translations() -> void:
	_ensure_fmod_property_translations()

	var domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.properties")
	for translation: Translation in _fmod_property_translations:
		domain.remove_translation(translation)
		domain.add_translation(translation)
	var editor_domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.editor")
	for translation: Translation in _fmod_editor_translations:
		editor_domain.remove_translation(translation)
		editor_domain.add_translation(translation)

	_notify_fmod_property_translations_changed()


func _remove_fmod_property_translations() -> void:
	if _fmod_property_translations.is_empty() and _fmod_editor_translations.is_empty():
		return

	var domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.properties")
	for translation: Translation in _fmod_property_translations:
		domain.remove_translation(translation)
	_fmod_property_translations.clear()
	var editor_domain: TranslationDomain = TranslationServer.get_or_add_domain(&"godot.editor")
	for translation: Translation in _fmod_editor_translations:
		editor_domain.remove_translation(translation)
	_fmod_editor_translations.clear()

	_notify_fmod_property_translations_changed()


func _ensure_fmod_property_translations() -> void:
	if not _fmod_property_translations.is_empty():
		return

	_fmod_property_translations = FmodPropertyTranslations.create_translations()
	# CSV 使用 keep 导入，保持可直接读取的插件翻译源
	var file: FileAccess = FileAccess.open("res://addons/fmod_player/translations/fmod_editor_translations.csv", FileAccess.READ)
	if file == null:
		push_error("Cannot open FMOD editor translations")
		return
	var header: PackedStringArray = file.get_csv_line()
	for column: int in range(1, header.size()):
		var translation: Translation = Translation.new()
		translation.locale = header[column]
		_fmod_editor_translations.append(translation)
	while not file.eof_reached():
		var row: PackedStringArray = file.get_csv_line()
		if row.is_empty() or row[0].is_empty():
			continue
		for column: int in range(1, mini(row.size(), header.size())):
			if not row[column].is_empty():
				_fmod_editor_translations[column - 1].add_message(StringName(row[0].c_unescape()), StringName(row[column].c_unescape()))


func _notify_fmod_property_translations_changed() -> void:
	var tree: SceneTree = get_tree()
	if tree and tree.root:
		tree.root.propagate_notification(MainLoop.NOTIFICATION_TRANSLATION_CHANGED)
