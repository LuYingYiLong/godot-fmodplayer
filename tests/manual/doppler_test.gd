extends Node3D

const PATH_LIMIT: float = 40.0
const START_POSITION: Vector3 = Vector3(-PATH_LIMIT, 1.5, -5.0)

var _speed: float = 40.0
var _direction: float = 1.0
var _running: bool
var _warmup_frames: int

@onready var _player: FmodAudioStreamPlayer3D = %Source
#@onready var _listener: AudioListener3D = %Listener
@onready var _doppler: CheckButton = %Doppler
@onready var _speed_slider: HSlider = %Speed
@onready var _speed_value: Label = %SpeedValue
@onready var _stop_button: Button = %Stop
@onready var _status: Label = %Status


func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_on_speed_value_changed(_speed_slider.value)
	_on_doppler_toggled(_doppler.button_pressed)
	_on_stop_pressed()


func _physics_process(delta: float) -> void:
	if _warmup_frames > 0:
		_warmup_frames -= 1
		if _warmup_frames == 0:
			_player.play()
			_running = _player.is_playing()
			_stop_button.disabled = not _running
			_update_status()
			if not _running:
				_status.text = "音源未能播放，请检查 Godot 输出面板和 FMOD 运行库"
		return
	if not _running:
		return
	var next_x: float = _player.position.x + _direction * _speed * delta
	if next_x > PATH_LIMIT:
		next_x = 2.0 * PATH_LIMIT - next_x
		_direction = -1.0
	elif next_x < -PATH_LIMIT:
		next_x = -2.0 * PATH_LIMIT - next_x
		_direction = 1.0
	_player.position.x = next_x
	_update_status()


func _on_start_pressed() -> void:
	_player.stop()
	_running = false
	_direction = 1.0
	_player.position = START_POSITION
	# 先让速度追踪器稳定，避免回到起点时的瞬移被当作真实速度
	_warmup_frames = 3
	_stop_button.disabled = false
	_status.text = "准备播放..."


func _on_stop_pressed() -> void:
	_running = false
	_warmup_frames = 0
	_player.stop()
	_stop_button.disabled = true
	_update_status()


func _on_doppler_toggled(enabled: bool) -> void:
	_player.doppler_tracking = (
		FmodAudioStreamPlayer3D.DOPPLER_TRACKING_PHYSICS_STEP
		if enabled else FmodAudioStreamPlayer3D.DOPPLER_TRACKING_DISABLED
	)
	if _running or _warmup_frames > 0:
		_on_start_pressed()
	else:
		_update_status()


func _on_speed_value_changed(value: float) -> void:
	_speed = value
	_speed_value.text = "%d m/s" % int(_speed)


func _update_status() -> void:
	if not _running:
		_status.text = "点击「开始 / 重来」试听，切换多普勒后会从起点重放\n监听器固定，距离衰减已关闭，重点比较音高变化"
		return
	#var offset: Vector3 = _player.global_position - _listener.global_position
	#var phase: String = "接近" if offset.x * _direction < 0.0 else "远离"
	#if absf(offset.x) < 1.0:
		#phase = "掠过监听器"
	#var mode: String = "开启" if _doppler.button_pressed else "关闭"
	#_status.text = "多普勒：%s　|　%s　|　距离 %.1f m\n开启：接近音高升高、远离音高降低；关闭：音高保持稳定" % [
		#mode, phase, offset.length(),
	#]
