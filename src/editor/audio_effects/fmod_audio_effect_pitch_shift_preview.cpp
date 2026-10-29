#include "fmod_audio_effect_pitch_shift_preview.h"

#include <cmath>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	namespace {
		constexpr float PI = 3.14159265358979323846f;

		Vector2 dial_point(const Vector2& p_center, float p_radius, float p_angle) {
			return p_center + Vector2(std::cos(p_angle), std::sin(p_angle)) * p_radius;
		}
	}

	void FmodAudioEffectPitchShiftPreview::_bind_methods() {
	}

	FmodAudioEffectPitchShiftPreview::FmodAudioEffectPitchShiftPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_force_pass_scroll_events(false);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
	}

	float FmodAudioEffectPitchShiftPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Vector2 FmodAudioEffectPitchShiftPreview::_dial_center() const {
		return Vector2(get_size().x * 0.5f, get_size().y - 12.0f * _scale());
	}

	float FmodAudioEffectPitchShiftPreview::_dial_radius() const {
		return MAX(0.0f, MIN(get_size().x * 0.5f - 14.0f * _scale(), get_size().y - 24.0f * _scale()));
	}

	float FmodAudioEffectPitchShiftPreview::_semitones() const {
		return effect.is_valid() ? 12.0f * std::log2(effect->get_pitch_scale()) : 0.0f;
	}

	float FmodAudioEffectPitchShiftPreview::_dial_range() const {
		return dragging ? drag_range : MAX(24.0f, std::ceil(std::abs(_semitones()) / 12.0f - 0.00001f) * 12.0f);
	}

	bool FmodAudioEffectPitchShiftPreview::_hit_dial(const Vector2& p_position, bool p_ring_only) const {
		const float radius = _dial_radius();
		const Vector2 offset = p_position - _dial_center();
		const float distance = offset.length();
		return radius >= 48.0f * _scale() && offset.y <= 6.0f * _scale() && distance <= radius + 10.0f * _scale() &&
			(!p_ring_only || distance >= radius - 24.0f * _scale());
	}

	void FmodAudioEffectPitchShiftPreview::set_effect(const Ref<FmodAudioEffectPitchShift>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragging = false;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectPitchShiftPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectPitchShiftPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectPitchShiftPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectPitchShiftPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectPitchShiftPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectPitchShiftPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectPitchShiftPreview::_on_effect_changed() {
		if (effect.is_valid()) {
			set_tooltip_text(String::num(_semitones(), 2).pad_decimals(2) + " st (" + String::num(effect->get_pitch_scale(), 4) + "x)\n" +
				tr("Drag along the dial to adjust pitch. Scroll: 1 semitone; Shift + scroll: 0.1 semitone. The scale expands beyond two octaves."));
		}
		queue_redraw();
	}

	void FmodAudioEffectPitchShiftPreview::_notification(int p_what) {
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
			if (!dragging) set_default_cursor_shape(CURSOR_ARROW);
			break;
		}
	}

	void FmodAudioEffectPitchShiftPreview::_edit_pitch(float p_semitones, bool p_changing) {
		if (effect.is_null() || is_read_only()) return;
		const float pitch = CLAMP(std::exp2(p_semitones / 12.0f), 0.01f, 16.0f);
		if (Math::is_equal_approx(pitch, effect->get_pitch_scale()) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed("pitch_scale", pitch, StringName(), p_changing);
	}

	void FmodAudioEffectPitchShiftPreview::_edit_pitch_at(const Vector2& p_position, bool p_changing) {
		const Vector2 offset = p_position - _dial_center();
		if (offset.length_squared() < 1.0f) return;
		const float angle = offset.y >= 0.0f ? (offset.x < 0.0f ? -PI : 0.0f) : std::atan2(offset.y, offset.x);
		_edit_pitch((angle / PI * 2.0f + 1.0f) * drag_range, p_changing);
	}

	void FmodAudioEffectPitchShiftPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP || button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN)) {
			if (dragging || !button->is_pressed() || !_hit_dial(button->get_position(), false) || !std::isfinite(button->get_factor()) || button->get_factor() <= 0.0f) return;
			const float direction = button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ? 1.0f : -1.0f;
			gesture_changed = false;
			_edit_pitch(_semitones() + direction * button->get_factor() * (button->is_shift_pressed() ? 0.1f : 1.0f), false);
			accept_event();
			return;
		}
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				if (!_hit_dial(button->get_position(), true)) return;
				drag_range = _dial_range();
				dragging = true;
				gesture_changed = false;
				_edit_pitch_at(button->get_position(), true);
			}
			else if (dragging) {
				if (!button->is_canceled()) _edit_pitch_at(button->get_position(), false);
				dragging = false;
				queue_redraw();
			}
			else return;
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_pitch_at(motion->get_position(), true);
				else dragging = false;
				accept_event();
			}
			set_default_cursor_shape(dragging || _hit_dial(motion->get_position(), true) ? CURSOR_POINTING_HAND : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectPitchShiftPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectPitchShiftPreview::_set_read_only(bool p_read_only) {
		dragging = false;
		set_default_cursor_shape(CURSOR_ARROW);
		queue_redraw();
	}

	void FmodAudioEffectPitchShiftPreview::_draw() {
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const float scale = _scale();
		const float radius = _dial_radius();
		if (effect.is_null() || radius < 48.0f * scale) return;
		const Vector2 center = _dial_center();
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		const float range = _dial_range();
		const float angle = (CLAMP(_semitones() / range, -1.0f, 1.0f) - 1.0f) * PI * 0.5f;
		for (int tick = 0; tick <= 48; tick++) {
			const float tick_angle = -PI + float(tick) / 48.0f * PI;
			const bool major = tick % 12 == 0;
			const float length = (major ? 10.0f : (tick % 6 == 0 ? 7.0f : 4.0f)) * scale;
			draw_line(dial_point(center, radius - length, tick_angle), dial_point(center, radius, tick_angle), muted, (major ? 1.5f : 1.0f) * scale, true);
		}
		const float indicator_inner = radius - 12.0f * scale;
		const float indicator_outer = radius + 1.0f * scale;
		Color fill = accent;
		fill.a *= 0.18f;
		if (!Math::is_zero_approx(_semitones())) {
			draw_arc(center, (indicator_inner + indicator_outer) * 0.5f, MIN(-PI * 0.5f, angle), MAX(-PI * 0.5f, angle), 64, fill, indicator_outer - indicator_inner, true);
		}
		draw_line(dial_point(center, indicator_inner, angle), dial_point(center, indicator_outer, angle), accent, 2.0f * scale, true);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float label_radius = MAX(radius - 27.0f * scale, 0.0f);
		for (int mark = 0; mark <= 4; mark++) {
			const float fraction = float(mark) * 0.25f;
			const float octaves = (fraction * 2.0f - 1.0f) * range / 12.0f;
			const String label = String::num(octaves, 1).trim_suffix(".0") + "oct";
			Vector2 position = dial_point(center, label_radius, -PI + fraction * PI);
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			if (mark == 0) position.x += width * 0.5f;
			if (mark == 4) position.x -= width * 0.5f;
			position += Vector2(-width * 0.5f, font->get_ascent(font_size) * 0.35f);
			draw_string(font, position, label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		}
		const String value = String::num(_semitones(), 2).pad_decimals(2) + " st";
		const int value_size = MAX(font_size, int(MIN(28.0f * scale, radius * 0.24f)));
		const float value_width = font->get_string_size(value, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, value_size).x;
		draw_string(font, Vector2(center.x - value_width * 0.5f, center.y - radius * 0.25f), value, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, value_size, accent);
	}
}
