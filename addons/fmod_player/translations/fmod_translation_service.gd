@tool
extends Node

const Catalogs: Script = preload("res://addons/fmod_player/translations/fmod_translation_catalogs.gd")

var last_errors: Array[String]
var _groups: Dictionary
var _reload_queued: bool


func _enter_tree() -> void:
	add_to_group(&"fmod_translation_services")
	if Engine.is_editor_hint():
		EditorInterface.get_editor_settings().settings_changed.connect(queue_registration)
	reload_catalogs()


func _exit_tree() -> void:
	if Engine.is_editor_hint():
		var settings: EditorSettings = EditorInterface.get_editor_settings()
		if settings.settings_changed.is_connected(queue_registration):
			settings.settings_changed.disconnect(queue_registration)
	unregister_catalogs(false)
	# 仅在编辑器继续运行时刷新，退出阶段不再通知已卸载的原生插件
	if Engine.is_editor_hint():
		get_tree().process_frame.connect(_notify_editor_after_unload.bind(EditorInterface.get_base_control()), CONNECT_ONE_SHOT)


func queue_registration() -> void:
	if not is_inside_tree() or _reload_queued:
		return
	_reload_queued = true
	_register_cached.call_deferred()


func _register_cached() -> void:
	_reload_queued = false
	if not is_inside_tree():
		return
	for domain_name: StringName in _groups:
		var domain: TranslationDomain = TranslationServer.get_or_add_domain(domain_name)
		for translation: Translation in _groups[domain_name]:
			domain.add_translation(translation)
	_notify_changed()


func reload_catalogs() -> bool:
	var next_groups: Dictionary
	last_errors.clear()
	for catalog: Dictionary in Catalogs.CATALOGS:
		var loaded: Array[Translation]
		for path: String in Catalogs.paths(catalog):
			if catalog["format"] == "csv":
				var data: Dictionary = Catalogs.read_csv(path)
				last_errors.append_array(data.get("errors", []))
				if data.get("errors", []).is_empty():
					loaded.append_array(Catalogs.create_csv_translations(data))
			else:
				var translation: Translation = ResourceLoader.load(path, "Translation", ResourceLoader.CACHE_MODE_IGNORE) as Translation
				if translation:
					loaded.append(translation)
				else:
					last_errors.append("Cannot load translation: %s" % path)
		next_groups[catalog["domain"]] = loaded
	if not last_errors.is_empty():
		push_error("\n".join(last_errors))
		return false
	unregister_catalogs(false)
	_groups = next_groups
	_register_cached()
	return true


func unregister_catalogs(notify: bool = true) -> void:
	for domain_name: StringName in _groups:
		var domain: TranslationDomain = TranslationServer.get_or_add_domain(domain_name)
		for translation: Translation in _groups[domain_name]:
			domain.remove_translation(translation)
	if notify:
		_notify_changed()


func _notify_changed() -> void:
	if is_inside_tree():
		get_tree().root.propagate_notification(MainLoop.NOTIFICATION_TRANSLATION_CHANGED)


static func _notify_editor_after_unload(base_control: Control) -> void:
	if is_instance_valid(base_control) and base_control.is_inside_tree():
		base_control.propagate_notification(MainLoop.NOTIFICATION_TRANSLATION_CHANGED)
