extends Node2D

var _status: Label

@onready var _fmod_player: FmodAudioStreamPlayer2D = $Sprite2D/FmodAudioStreamPlayer2D
@onready var _godot_player: AudioStreamPlayer2D = $Sprite2D2/AudioStreamPlayer2D


func _ready() -> void:
	var controls: HBoxContainer = HBoxContainer.new()
	controls.position = Vector2(16.0, 100.0)
	$Navigation.add_child(controls)
	_add_button(controls, "仅播放 FMOD", _on_fmod_pressed)
	_add_button(controls, "仅播放 Godot", _on_godot_pressed)
	_add_button(controls, "同时播放", _on_both_pressed)
	_add_button(controls, "停止", _on_stop_pressed)
	_status = Label.new()
	_status.position = Vector2(16.0, 144.0)
	_status.text = "两个音源使用相同 MP3，分别通过 FMOD 和 Godot 导入"
	$Navigation.add_child(_status)


func _on_fmod_pressed() -> void:
	_godot_player.stop()
	_fmod_player.play()
	_status.text = "正在播放 FMOD 音源"


func _on_godot_pressed() -> void:
	_fmod_player.stop()
	_godot_player.play()
	_status.text = "正在播放 Godot 音源"


func _on_both_pressed() -> void:
	_fmod_player.play()
	_godot_player.play()
	_status.text = "正在同时播放两个音源"


func _on_stop_pressed() -> void:
	_fmod_player.stop()
	_godot_player.stop()
	_status.text = "已停止"


func _add_button(parent: HBoxContainer, text: String, callback: Callable) -> void:
	var button: Button = Button.new()
	button.text = text
	button.pressed.connect(callback)
	parent.add_child(button)
