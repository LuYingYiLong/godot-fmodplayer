@tool
extends SceneTree

const Catalogs: Script = preload("res://addons/fmod_player/translations/fmod_translation_catalogs.gd")


func _initialize() -> void:
	var path: String = Catalogs.CATALOGS[0]["path"]
	var arguments: PackedStringArray = OS.get_cmdline_user_args()
	var index: int = arguments.find("--csv")
	if index >= 0 and index + 1 < arguments.size():
		path = arguments[index + 1]
	var data: Dictionary = Catalogs.read_csv(path)
	var errors: Array = data.get("errors", [])
	for error: String in errors:
		push_error(error)
	print("CSV translations are loaded directly; no GDScript output is generated.")
	quit(0 if errors.is_empty() else 1)
