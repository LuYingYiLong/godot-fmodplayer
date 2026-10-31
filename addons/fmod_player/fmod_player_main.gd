@tool
extends EditorPlugin

# This is the main plugin that actually uses GDExtension classes.
# It will only be loaded after confirming fmod.dll exists.

var importer: EditorImportPlugin
var inspector: EditorInspectorPlugin
var fmod_audio_stream_player_3d_gizom_plugin: EditorNode3DGizmoPlugin
var fmod_reverb_zone_3d_gizmo_plugin: EditorNode3DGizmoPlugin
var fmod_geometry_mesh_tools_plugin: EditorPlugin
var android_build_template_plugin: EditorPlugin
var _editor_interface: EditorInterface



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



func _exit_tree() -> void:
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
