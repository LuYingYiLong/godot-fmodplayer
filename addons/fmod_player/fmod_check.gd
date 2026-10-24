@tool
extends EditorPlugin

# This script does not reference any GDExtension-registered classes,
# so it can load normally even if the FMOD runtime is missing.

const FMOD_DOWNLOAD_URL: String = "https://www.fmod.com/download"
const FMOD_DOCS_URL: String = "https://github.com/LuYingYiLong/Godot-FmodPlayer#quick-start"

var warning_dialog: AcceptDialog
var main_plugin: EditorPlugin


func _enter_tree() -> void:
	# Check if FMOD runtime library exists
	if not _check_fmod_runtime():
		_show_fmod_missing_warning()
		return

	# FMOD exists, load main plugin
	_load_main_plugin()


func _exit_tree() -> void:
	_unload_main_plugin()
	if warning_dialog:
		warning_dialog.queue_free()
		warning_dialog = null


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
	push_warning("[FmodPlayer] FMOD runtime library not found! Please install FMOD Engine.")
	if DisplayServer.get_name() == "headless":
		return

	# Create warning dialog
	warning_dialog = AcceptDialog.new()
	warning_dialog.title = "FMOD Runtime Missing"
	warning_dialog.dialog_text = _get_warning_text()
	warning_dialog.ok_button_text = "Open FMOD Download Page"
	warning_dialog.add_cancel_button("Install Later")

	warning_dialog.confirmed.connect(func() -> void:
		OS.shell_open(FMOD_DOWNLOAD_URL)
	)

	# Delay display to ensure editor UI is ready
	call_deferred("_show_dialog_deferred")


func _get_warning_text() -> String:
	return """Welcome to the FmodPlayer Plugin!
FMOD runtime library was not detected.
Due to FMOD's license restrictions, we cannot distribute it with the plugin.
Please follow these steps:
1. Visit https://www.fmod.com/download
2. Register/Login to your FMOD account
3. Download FMOD Engine
4. Copy the runtime library to the plugin directory based on your platform:
   Windows: fmod.dll → addons/fmod_player/bin/
   Linux:   libfmodL.so.15 and libfmod.so.15 → addons/fmod_player/bin/
   macOS:   libfmod.dylib → addons/fmod_player/bin/
5. Restart Godot Editor
For detailed instructions, please check the documentation.
Would you like to open the download page?"""


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
