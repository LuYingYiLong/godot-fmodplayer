@tool
extends EditorPlugin

# This script does not reference any GDExtension-registered classes,
# so it can load normally even if the FMOD runtime is missing.

const FMOD_DOWNLOAD_URL: String = "https://www.fmod.com/download"
const FMOD_DOCS_URL: String = "https://github.com/LuYingYiLong/Godot-FmodPlayer#quick-start"
# 引导依赖在 UID 缓存建立前解析
const TranslationService: Script = preload("res://addons/fmod_player/translations/fmod_translation_service.gd")

var warning_dialog: AcceptDialog
var main_plugin: EditorPlugin
var _translation_service: Node
var _translation_dialog: AcceptDialog


func _enter_tree() -> void:
	_translation_service = TranslationService.new()
	add_child(_translation_service)
	set_translation_domain(&"godot.editor")
	add_tool_menu_item("FMOD Translations", _open_translation_tools)
	# Check if FMOD runtime library exists
	if not _check_fmod_runtime():
		_show_fmod_missing_warning()
		return

	# FMOD exists, load main plugin
	_load_main_plugin()


func _exit_tree() -> void:
	remove_tool_menu_item("FMOD Translations")
	if _translation_dialog:
		_translation_dialog.queue_free()
	_unload_main_plugin()
	if warning_dialog:
		warning_dialog.queue_free()
		warning_dialog = null


func _notification(what: int) -> void:
	if what == NOTIFICATION_TRANSLATION_CHANGED and warning_dialog:
		warning_dialog.title = tr("FMOD Runtime Missing")
		warning_dialog.dialog_text = _get_warning_text()
		warning_dialog.ok_button_text = tr("Open FMOD Download Page")


func _check_fmod_runtime() -> bool:
	# Check multiple possible locations
	var paths_to_check: Array[String] = [
		"res://addons/fmod_player/bin/fmod.dll",
		"res://addons/fmod_player/bin/libfmod.so",
		"res://addons/fmod_player/bin/libfmodL.so.15",
		"res://addons/fmod_player/bin/libfmod.so.15",
		"res://addons/fmod_player/bin/libfmod.dylib",
	]

	for path: String in paths_to_check:
		if FileAccess.file_exists(path):
			return true

	return false


func _show_fmod_missing_warning() -> void:
	push_warning(tr("FMOD runtime library was not detected."))
	if DisplayServer.get_name() == "headless":
		return

	# Create warning dialog
	warning_dialog = AcceptDialog.new()
	warning_dialog.title = tr("FMOD Runtime Missing")
	warning_dialog.dialog_text = _get_warning_text()
	warning_dialog.ok_button_text = tr("Open FMOD Download Page")
	warning_dialog.add_cancel_button(tr("Install Later"))

	warning_dialog.confirmed.connect(func() -> void:
		OS.shell_open(FMOD_DOWNLOAD_URL)
	)

	# Delay display to ensure editor UI is ready
	call_deferred("_show_dialog_deferred")


func _get_warning_text() -> String:
	return tr("Install the FMOD Core SDK 2.04, copy the matching runtime library to addons/fmod_player/bin/, then restart Godot.") + "\n\n" + tr("Windows: fmod.dll\nLinux: libfmodL.so.15 / libfmod.so.15\nmacOS: libfmod.dylib")


func _open_translation_tools() -> void:
	if _translation_dialog == null:
		_translation_dialog = AcceptDialog.new()
		_translation_dialog.title = "FMOD Translations"
		_translation_dialog.set_translation_domain(&"godot.editor")
		var scene: PackedScene = load("uid://dccdv74ostt1m") as PackedScene
		_translation_dialog.add_child(scene.instantiate())
		EditorInterface.get_base_control().add_child(_translation_dialog)
	_translation_dialog.popup_centered(Vector2i(1080, 720) * EditorInterface.get_editor_scale())


func _show_dialog_deferred() -> void:
	var base_control: Control = get_editor_interface().get_base_control()
	base_control.add_child(warning_dialog)
	warning_dialog.popup_centered(Vector2(550, 400))


func _load_main_plugin() -> void:
	# Dynamically load main plugin script to avoid parse errors when fmod.dll is missing
	var main_script: Script = load("res://addons/fmod_player/fmod_player_main.gd")
	if main_script:
		main_plugin = main_script.new()
		main_plugin.set_editor_interface(get_editor_interface())
		add_child(main_plugin)


func _unload_main_plugin() -> void:
	if main_plugin:
		main_plugin.queue_free()
		main_plugin = null
