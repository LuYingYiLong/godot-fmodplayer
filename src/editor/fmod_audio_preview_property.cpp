#include "fmod_audio_preview_property.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture2d.hpp>

namespace godot {
	void FmodAudioPreviewProperty::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_stream", "stream"), &FmodAudioPreviewProperty::set_stream);
		ClassDB::bind_method(D_METHOD("get_stream"), &FmodAudioPreviewProperty::get_stream);
		ClassDB::bind_method(D_METHOD("_draw_indicator"), &FmodAudioPreviewProperty::_draw_indicator);
		ClassDB::bind_method(D_METHOD("_on_indicator_gui_input", "event"), &FmodAudioPreviewProperty::_on_indicator_gui_input);
		ClassDB::bind_method(D_METHOD("_play"), &FmodAudioPreviewProperty::_play);
		ClassDB::bind_method(D_METHOD("_stop"), &FmodAudioPreviewProperty::_stop);
		ClassDB::bind_method(D_METHOD("_on_finished"), &FmodAudioPreviewProperty::_on_finished);
	}

	FmodAudioPreviewProperty::FmodAudioPreviewProperty() {
		EditorInterface* editor = EditorInterface::get_singleton();
		const float editor_scale = editor ? editor->get_editor_scale() : 1.0f;
		set_custom_minimum_size(Vector2(1.0f, 100.0f) * editor_scale);
		set_process(false);

		_player = memnew(FmodAudioStreamPlayer);
		_player->connect("finished", callable_mp(this, &FmodAudioPreviewProperty::_on_finished));
		add_child(_player);

		VBoxContainer* vbox = memnew(VBoxContainer);
		vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		add_child(vbox);

		_preview = memnew(FmodAudioWaveformView);
		_preview->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		_preview->connect("waveform_updated", callable_mp(this, &FmodAudioPreviewProperty::_on_waveform_updated));
		vbox->add_child(_preview);

		_indicator = memnew(Control);
		_indicator->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
		_indicator->set_mouse_filter(Control::MOUSE_FILTER_STOP);
		_indicator->connect("draw", callable_mp(this, &FmodAudioPreviewProperty::_draw_indicator));
		_indicator->connect("gui_input", callable_mp(this, &FmodAudioPreviewProperty::_on_indicator_gui_input));
		_preview->add_child(_indicator);

		HBoxContainer* hbox = memnew(HBoxContainer);
		hbox->add_theme_constant_override("separation", 0);
		vbox->add_child(hbox);

		_play_button = memnew(Button);
		_play_button->set_flat(true);
		_play_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
		_play_button->set_accessibility_name("Play");
		_play_button->connect("pressed", callable_mp(this, &FmodAudioPreviewProperty::_play));
		hbox->add_child(_play_button);

		_stop_button = memnew(Button);
		_stop_button->set_flat(true);
		_stop_button->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
		_stop_button->set_accessibility_name("Stop");
		_stop_button->connect("pressed", callable_mp(this, &FmodAudioPreviewProperty::_stop));
		hbox->add_child(_stop_button);

		_current_label = memnew(Label);
		_current_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
		_current_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_current_label->set_modulate(Color(1, 1, 1, 0.5));
		hbox->add_child(_current_label);

		_duration_label = memnew(Label);
		hbox->add_child(_duration_label);

		_update_time_labels();
	}

	FmodAudioPreviewProperty::~FmodAudioPreviewProperty() {
	}

	void FmodAudioPreviewProperty::_notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_THEME_CHANGED: {
				const Ref<Font> font = get_theme_font("status_source", "EditorFonts");
				_current_label->add_theme_font_override("font", font);
				_duration_label->add_theme_font_override("font", font);

				if (_play_button) {
					_play_button->set_button_icon(FmodUtils::get_editor_theme_icon(is_playing ? "Pause" : "MainPlay"));
				}
				if (_stop_button) {
					_stop_button->set_button_icon(FmodUtils::get_editor_theme_icon("Stop"));
				}
				set_color(FmodUtils::get_editor_theme_color("dark_color_1", "Editor"));
				_queue_preview_redraw();
			} break;

			case NOTIFICATION_PROCESS: {
				_process_playback();
			} break;

			case NOTIFICATION_READY: {
				_update_preview();
			} break;

			case NOTIFICATION_RESIZED: {
				_queue_preview_redraw();
			} break;

			case NOTIFICATION_VISIBILITY_CHANGED: {
				if (!is_visible_in_tree()) {
					_stop();
				}
				_queue_preview_redraw();
			} break;
		}
	}

	void FmodAudioPreviewProperty::set_stream(const Ref<FmodAudioStream>& p_stream) {
		if (stream == p_stream) {
			return;
		}

		stream = p_stream;
		_update_preview();
	}

	Ref<FmodAudioStream> FmodAudioPreviewProperty::get_stream() const {
		return stream;
	}

	void FmodAudioPreviewProperty::_process_playback() {
		if (!is_playing || _player == nullptr) {
			return;
		}

		playback_position = _player->get_playback_position();
		_update_time_labels();
		_queue_preview_redraw();

		if (!_player->is_playing()) {
			_on_finished();
		}
	}

	void FmodAudioPreviewProperty::_update_time_labels() {
		if (_current_label) {
			_current_label->set_text(String::num(playback_position, 2).pad_decimals(2) + " /");
		}
		if (_duration_label) {
			_duration_label->set_text(String::num(stream_duration, 2).pad_decimals(2) + "s");
		}
	}

	void FmodAudioPreviewProperty::_queue_preview_redraw() {
		if (_preview) {
			_preview->queue_redraw();
		}
		if (_indicator) {
			_indicator->queue_redraw();
		}
	}

	void FmodAudioPreviewProperty::_update_preview() {
		if (_player != nullptr) {
			_player->stop();
		}

		is_playing = false;
		is_pausing = false;
		is_dragging = false;
		playback_position = 0.0f;
		stream_duration = 0.0f;
		set_process(false);

		if (_play_button) {
			_play_button->set_button_icon(FmodUtils::get_editor_theme_icon("MainPlay"));
		}

		if (stream.is_valid()) {
			if (_player != nullptr) {
				_player->set_stream(stream);
			}
		}
		_preview->set_stream(stream);
		_on_waveform_updated();

		set_visible(stream.is_valid());
		_update_time_labels();
		_queue_preview_redraw();
	}

	void FmodAudioPreviewProperty::_on_waveform_updated() {
		const Ref<FmodAudioWaveform> waveform = _preview->get_waveform();
		stream_duration = waveform.is_valid() ? float(waveform->get_length()) : 0.0f;
		_update_time_labels();
		_queue_preview_redraw();
	}

	void FmodAudioPreviewProperty::_draw_indicator() {
		if (stream.is_null() || stream_duration <= 0.0f || _indicator == nullptr) {
			return;
		}

		Size2 size = _indicator->get_size();
		if (size.width <= 0 || size.height <= 0) {
			return;
		}

		float ofs_x = CLAMP(playback_position / stream_duration * size.width, 0.0f, size.width);
		Color color = FmodUtils::get_editor_theme_color("accent_color", "Editor");
		Ref<Texture2D> icon = FmodUtils::get_editor_theme_icon("TimelineIndicator");

		RID canvas_item = _indicator->get_canvas_item();
		EditorInterface* editor = EditorInterface::get_singleton();
		const float editor_scale = editor ? editor->get_editor_scale() : 1.0f;
		RenderingServer::get_singleton()->canvas_item_add_line(canvas_item, Point2(ofs_x, 0), Point2(ofs_x, size.height), color, std::round(2.0f * editor_scale));

		if (icon.is_valid()) {
			RenderingServer::get_singleton()->canvas_item_add_texture_rect(
				canvas_item,
				Rect2(ofs_x - icon->get_width() * 0.5f, 0, icon->get_width(), icon->get_height()),
				icon->get_rid(),
				false,
				color
			);
		}
	}

	void FmodAudioPreviewProperty::_on_indicator_gui_input(const Ref<InputEvent>& p_event) {
		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (mb->is_pressed()) {
				_seek_to(mb->get_position().x);
			}
			is_dragging = mb->is_pressed();
		}

		Ref<InputEventMouseMotion> mm = p_event;
		if (mm.is_valid() && is_dragging) {
			_seek_to(mm->get_position().x);
		}
	}

	void FmodAudioPreviewProperty::_update_playback_position(float p_position) {
		playback_position = CLAMP(p_position, 0.0f, MAX(stream_duration, 0.0f));
		_update_time_labels();
		_queue_preview_redraw();
	}

	void FmodAudioPreviewProperty::_seek_to(float p_x) {
		if (stream.is_null() || stream_duration <= 0.0f || _indicator == nullptr) {
			return;
		}

		float width = _indicator->get_size().x;
		if (width <= 0.0f) {
			return;
		}

		_update_playback_position((p_x / width) * stream_duration);
		if (_player != nullptr && is_playing) {
			_player->seek(playback_position);
		}
	}

	void FmodAudioPreviewProperty::_play() {
		if (stream.is_null() || !stream->is_data_loaded() || _player == nullptr) {
			return;
		}

		if (is_playing) {
			is_pausing = true;
			playback_position = _player->get_playback_position();
			_player->stop();
			is_playing = false;
			set_process(false);
			if (_play_button) {
				_play_button->set_button_icon(FmodUtils::get_editor_theme_icon("MainPlay"));
			}
			_update_playback_position(playback_position);
		}
		else {
			is_pausing = false;
			_player->play(playback_position);
			is_playing = true;
			set_process(true);
			if (_play_button) {
				_play_button->set_button_icon(FmodUtils::get_editor_theme_icon("Pause"));
			}
		}
	}

	void FmodAudioPreviewProperty::_stop() {
		if (_player != nullptr) {
			_player->stop();
		}

		is_playing = false;
		is_pausing = false;
		is_dragging = false;
		set_process(false);

		if (_play_button) {
			_play_button->set_button_icon(FmodUtils::get_editor_theme_icon("MainPlay"));
		}

		_update_playback_position(0.0f);

		if (_player != nullptr && stream.is_valid()) {
			_player->set_stream(stream);
		}
	}

	void FmodAudioPreviewProperty::_on_finished() {
		is_playing = false;
		set_process(false);

		if (_play_button) {
			_play_button->set_button_icon(FmodUtils::get_editor_theme_icon("MainPlay"));
		}

		if (!is_pausing) {
			_update_playback_position(0.0f);
		}
		else {
			_update_playback_position(playback_position);
		}
		is_pausing = false;
	}
}
