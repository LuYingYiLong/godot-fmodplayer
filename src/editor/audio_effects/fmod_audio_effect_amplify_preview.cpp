#include "fmod_audio_effect_amplify_preview.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	namespace {
		constexpr float MIN_GAIN_DB = -80.0f;
		constexpr float MAX_GAIN_DB = 24.0f;
	}

	void FmodAudioEffectAmplifyPreview::_bind_methods() {
	}

	FmodAudioEffectAmplifyPreview::FmodAudioEffectAmplifyPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
	}

	float FmodAudioEffectAmplifyPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectAmplifyPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(MAX((get_size().x - 88.0f * scale) * 0.5f, 8.0f * scale), 12.0f * scale), Vector2(MIN(32.0f * scale, MAX(get_size().x - 64.0f * scale, 0.0f)), MAX(get_size().y - 24.0f * scale, 0.0f)));
	}

	Vector2 FmodAudioEffectAmplifyPreview::_gain_position(float p_gain) const {
		const Rect2 plot = _plot_rect();
		return plot.position + Vector2(plot.size.x * 0.5f, (MAX_GAIN_DB - p_gain) / (MAX_GAIN_DB - MIN_GAIN_DB) * plot.size.y);
	}

	bool FmodAudioEffectAmplifyPreview::_hit_handle(const Vector2& p_position) const {
		return effect.is_valid() && _plot_rect().grow(8.0f * _scale()).has_point(p_position);
	}

	void FmodAudioEffectAmplifyPreview::set_effect(const Ref<FmodAudioEffectAmplify>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragging = false;
		hovered = false;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectAmplifyPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectAmplifyPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectAmplifyPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectAmplifyPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectAmplifyPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectAmplifyPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectAmplifyPreview::_update_cursor() {
		set_default_cursor_shape(!is_read_only() && (hovered || dragging) ? CURSOR_VSIZE : CURSOR_ARROW);
	}

	void FmodAudioEffectAmplifyPreview::_on_effect_changed() {
		if (effect.is_valid()) {
			set_tooltip_text(tr("Gain") + ": " + String::num(effect->get_volume_db(), 2) + " dB (" + String::num(effect->get_volume_linear(), 4) + "x)\n" + tr("Drag the gain bar vertically to adjust."));
		}
		_update_cursor();
		queue_redraw();
	}

	void FmodAudioEffectAmplifyPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragging = false;
			_disconnect_effect();
			break;
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_RESIZED:
			queue_redraw();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered = false;
			_update_cursor();
			queue_redraw();
			break;
		}
	}

	void FmodAudioEffectAmplifyPreview::_edit_gain_at(const Vector2& p_position, bool p_changing) {
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || is_read_only() || plot.size.y <= 0.0f) return;
		const float gain = CLAMP(drag_start_gain + (drag_start_position.y - p_position.y) / plot.size.y * (MAX_GAIN_DB - MIN_GAIN_DB), MIN_GAIN_DB, MAX_GAIN_DB);
		if (Math::is_equal_approx(gain, effect->get_volume_db()) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed("volume_db", gain, StringName(), p_changing);
	}

	void FmodAudioEffectAmplifyPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragging = _hit_handle(button->get_position());
				if (!dragging) return;
				drag_start_position = button->get_position();
				drag_start_gain = effect->get_volume_db();
				gesture_changed = false;
			}
			else if (dragging) {
				if (!button->is_canceled()) _edit_gain_at(button->get_position(), false);
				dragging = false;
				hovered = _hit_handle(button->get_position());
			}
			else {
				return;
			}
			_update_cursor();
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) {
					_edit_gain_at(motion->get_position(), true);
				}
				else {
					dragging = false;
					_update_cursor();
				}
				accept_event();
			}
			else {
				const bool hit = _hit_handle(motion->get_position());
				if (hit != hovered) {
					hovered = hit;
					_update_cursor();
					queue_redraw();
				}
			}
		}
	}

	void FmodAudioEffectAmplifyPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectAmplifyPreview::_set_read_only(bool p_read_only) {
		dragging = false;
		set_default_cursor_shape(!p_read_only && hovered ? CURSOR_VSIZE : CURSOR_ARROW);
		queue_redraw();
	}

	void FmodAudioEffectAmplifyPreview::_draw() {
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color track = get_theme_color("dark_color_1", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		draw_rect(Rect2(Vector2(), get_size()), background);
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 8.0f * scale || plot.size.y < 32.0f * scale) return;
		draw_rect(plot, track);
		const bool silent = effect->get_volume_linear() <= 0.0f;
		const float gain_y = _gain_position(effect->get_volume_db()).y;
		const float fill_top = silent ? plot.get_end().y : gain_y;
		//if (fill_top < plot.get_end().y) {
			draw_rect(Rect2(Vector2(plot.position.x, fill_top), Vector2(plot.size.x, plot.get_end().y - fill_top)), accent);
		//}
		draw_rect(plot, muted, false, scale);
		const float zero_y = _gain_position(0.0f).y;
		draw_line(Vector2(plot.position.x - 4.0f * scale, zero_y), Vector2(plot.get_end().x + 8.0f * scale, zero_y), muted, scale);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		for (int output : { 24, 0, -24, -48, -60, -80 }) {
			const float y = _gain_position(float(output)).y;
			draw_line(Vector2(plot.get_end().x + 4.0f * scale, y), Vector2(plot.get_end().x + 8.0f * scale, y), muted, scale);
			const String label = output > 0 ? String("+") + String::num_int64(output) : (output == 0 ? String("0 dB") : String::num_int64(output));
			draw_string(font, Vector2(plot.get_end().x + 12.0f * scale, y + font->get_ascent(font_size) * 0.4f), label, HORIZONTAL_ALIGNMENT_LEFT, get_size().x - plot.get_end().x - 16.0f * scale, font_size, muted);
		}
	}
}
