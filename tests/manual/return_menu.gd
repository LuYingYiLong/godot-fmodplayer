extends CanvasLayer

const MENU_SCENE_UID: String = "uid://dn5d452hgjfbj"

@export var instructions: String = "WASD 移动，左键捕获鼠标，Esc 显示鼠标"


func _ready() -> void:
	var panel: VBoxContainer = VBoxContainer.new()
	panel.position = Vector2(16.0, 16.0)
	panel.add_theme_constant_override("separation", 8)
	add_child(panel)
	var button: Button = Button.new()
	button.text = "返回测试菜单"
	button.pressed.connect(_on_return_pressed)
	panel.add_child(button)
	var hint: Label = Label.new()
	hint.text = instructions
	hint.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.add_child(hint)


func _on_return_pressed() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	get_tree().change_scene_to_file(MENU_SCENE_UID)
