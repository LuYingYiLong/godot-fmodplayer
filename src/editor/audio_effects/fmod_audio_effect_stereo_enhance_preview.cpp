#include "editor/fmod_editor_translation.h"
#include "fmod_audio_effect_stereo_enhance_preview.h"

#include <cmath>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	namespace {
		constexpr float PI = 3.14159265358979323846f;
		constexpr int SECTOR_SEGMENTS = 64;

		Vector2 radar_point(const Vector2& p_center, float p_radius, float p_angle) {
			return p_center + Vector2(std::cos(p_angle), std::sin(p_angle)) * p_radius;
		}
	}

	void FmodAudioEffectStereoEnhancePreview::_bind_methods() {
	}

	FmodAudioEffectStereoEnhancePreview::FmodAudioEffectStereoEnhancePreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
	}

	float FmodAudioEffectStereoEnhancePreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Vector2 FmodAudioEffectStereoEnhancePreview::_radar_center() const {
		return Vector2(get_size().x * 0.5f, get_size().y - 12.0f * _scale());
	}

	float FmodAudioEffectStereoEnhancePreview::_radar_radius() const {
		return MAX(0.0f, MIN(get_size().x * 0.5f - 14.0f * _scale(), get_size().y - 24.0f * _scale()));
	}

	float FmodAudioEffectStereoEnhancePreview::_half_angle() const {
		// 参数示意：Pan Pullout 为 1 时张角 45°，为 4 时展开整个半圆
		return effect.is_valid() ? effect->get_pan_pullout() * PI * 0.125f : 0.0f;
	}

	bool FmodAudioEffectStereoEnhancePreview::_hit_radar(const Vector2& p_position) const {
		const float scale = _scale();
		const float radius = _radar_radius();
		const Vector2 offset = p_position - _radar_center();
		const float distance = offset.length();
		return radius >= 48.0f * scale && offset.y <= 6.0f * scale && distance >= 18.0f * scale && distance <= radius + 8.0f * scale;
	}

	void FmodAudioEffectStereoEnhancePreview::set_effect(const Ref<FmodAudioEffectStereoEnhance>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragging = false;
		hovered = false;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectStereoEnhancePreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectStereoEnhancePreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectStereoEnhancePreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectStereoEnhancePreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectStereoEnhancePreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectStereoEnhancePreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectStereoEnhancePreview::_on_effect_changed() {
		geometry_dirty = true;
		if (effect.is_valid()) {
			set_tooltip_text(fmod_editor_translate("Parameter illustration: fan angle shows Pan Pullout, the inner arc shows Time Pullout, and fill strength shows Surround. Drag the radar to adjust Pan Pullout.") +
				"\nPan Pullout: " + String::num(effect->get_pan_pullout(), 2) + "x\nTime Pullout: " + String::num(effect->get_time_pullout(), 2) + " ms\nSurround: " + String::num(effect->get_surround() * 100.0f, 0) + "%");
		}
		queue_redraw();
	}

	void FmodAudioEffectStereoEnhancePreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
			fmod_refresh_editor_control(this);
			_on_effect_changed();
			break;
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragging = false;
			_disconnect_effect();
			break;
		case NOTIFICATION_RESIZED:
			geometry_dirty = true;
			queue_redraw();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered = false;
			if (!dragging) set_default_cursor_shape(CURSOR_ARROW);
			break;
		}
	}

	void FmodAudioEffectStereoEnhancePreview::_rebuild_geometry() {
		geometry_dirty = false;
		sector.clear();
		const float half_angle = _half_angle();
		if (half_angle < 0.00001f) return;
		const Vector2 center = _radar_center();
		const float radius = _radar_radius();
		sector.resize(SECTOR_SEGMENTS + 2);
		sector.set(0, center);
		for (int i = 0; i <= SECTOR_SEGMENTS; i++) {
			sector.set(i + 1, radar_point(center, radius, -PI * 0.5f - half_angle + 2.0f * half_angle * float(i) / float(SECTOR_SEGMENTS)));
		}
	}

	void FmodAudioEffectStereoEnhancePreview::_edit_width_at(const Vector2& p_position, bool p_changing) {
		if (effect.is_null() || is_read_only()) return;
		const Vector2 offset = p_position - _radar_center();
		if (offset.length_squared() < 1.0f) return;
		const float half_angle = std::atan2(std::abs(offset.x), MAX(-offset.y, 0.0f));
		const float width = CLAMP(half_angle / (PI * 0.125f), 0.0f, 4.0f);
		if (Math::is_equal_approx(width, effect->get_pan_pullout()) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed("pan_pullout", width, StringName(), p_changing);
	}

	void FmodAudioEffectStereoEnhancePreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				if (!_hit_radar(button->get_position())) return;
				dragging = true;
				gesture_changed = false;
				_edit_width_at(button->get_position(), true);
			}
			else if (dragging) {
				if (!button->is_canceled()) _edit_width_at(button->get_position(), false);
				dragging = false;
				hovered = _hit_radar(button->get_position());
			}
			else return;
			set_default_cursor_shape(dragging || hovered ? CURSOR_POINTING_HAND : CURSOR_ARROW);
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_width_at(motion->get_position(), true);
				else dragging = false;
				accept_event();
			}
			hovered = _hit_radar(motion->get_position());
			set_default_cursor_shape(dragging || hovered ? CURSOR_POINTING_HAND : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectStereoEnhancePreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectStereoEnhancePreview::_set_read_only(bool p_read_only) {
		dragging = false;
		hovered = false;
		set_default_cursor_shape(CURSOR_ARROW);
		queue_redraw();
	}

	void FmodAudioEffectStereoEnhancePreview::_draw() {
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const float scale = _scale();
		const float radius = _radar_radius();
		if (effect.is_null() || radius < 48.0f * scale) return;
		if (geometry_dirty) _rebuild_geometry();
		const Vector2 center = _radar_center();
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		Color grid = muted;
		grid.a *= 0.5f;
		draw_arc(center, radius, -PI, 0.0f, 96, grid, scale, true);
		draw_line(center - Vector2(radius, 0.0f), center + Vector2(radius, 0.0f), grid, scale, true);
		for (int spoke = 1; spoke <= 3; spoke++) {
			draw_line(center, radar_point(center, radius, -PI + PI * 0.25f * float(spoke)), grid, scale, true);
		}
		const float half_angle = _half_angle();
		if (sector.size() >= 3) {
			Color fill = accent;
			fill.a *= 0.1f + 0.18f * effect->get_surround();
			draw_colored_polygon(sector, fill);
			draw_arc(center, radius, -PI * 0.5f - half_angle, -PI * 0.5f + half_angle, 64, accent, 2.0f * scale, true);
			if (effect->get_time_pullout() > 0.0f) {
				Color delay_color = accent;
				delay_color.a *= 0.5f;
				const float delay_radius = radius * (0.25f + 0.65f * effect->get_time_pullout() / 50.0f);
				draw_arc(center, delay_radius, -PI * 0.5f - half_angle, -PI * 0.5f + half_angle, 48, delay_color, 2.0f * scale, true);
			}
		}
		draw_line(center, radar_point(center, radius, -PI * 0.5f - half_angle), accent, 2.0f * scale, true);
		if (half_angle >= 0.00001f) draw_line(center, radar_point(center, radius, -PI * 0.5f + half_angle), accent, 2.0f * scale, true);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float y = 8.0f * scale + font->get_ascent(font_size);
		draw_string(font, Vector2(8.0f * scale, y), "L", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		const float right_width = font->get_string_size("R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
		draw_string(font, Vector2(get_size().x - 8.0f * scale - right_width, y), "R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
	}
}
