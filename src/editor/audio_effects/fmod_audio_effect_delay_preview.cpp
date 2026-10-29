#include "fmod_audio_effect_delay_preview.h"

#include "core/fmod_utils.hpp"
#include <cmath>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
	namespace {
		constexpr float PI = 3.14159265358979323846f;
		constexpr int ARC_SEGMENTS = 96;
	}

	void FmodAudioEffectDelayPreview::_bind_methods() {
	}

	FmodAudioEffectDelayPreview::FmodAudioEffectDelayPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
	}

	float FmodAudioEffectDelayPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Vector2 FmodAudioEffectDelayPreview::_center() const {
		return Vector2(get_size().x * 0.5f, get_size().y - 12.0f * _scale());
	}

	float FmodAudioEffectDelayPreview::_radius() const {
		return MAX(0.0f, MIN(get_size().x * 0.5f - 14.0f * _scale(), get_size().y - 24.0f * _scale()));
	}

	float FmodAudioEffectDelayPreview::_inner_radius() const {
		return 12.0f * _scale();
	}

	FmodAudioEffectDelayPreview::Tap FmodAudioEffectDelayPreview::_tap(int p_index) const {
		if (p_index == 0) return { effect->is_tap1_active(), effect->get_tap1_delay_ms(), effect->get_tap1_level_db(), effect->get_tap1_pan() };
		return { effect->is_tap2_active(), effect->get_tap2_delay_ms(), effect->get_tap2_level_db(), effect->get_tap2_pan() };
	}

	float FmodAudioEffectDelayPreview::_time_range() const {
		if (dragging) return drag_range;
		float time = 0.0f;
		if (effect.is_valid()) {
			for (int i = 0; i < 2; i++) {
				const Tap tap = _tap(i);
				if (tap.active) time = MAX(time, tap.time);
			}
			if (effect->is_feedback_active() && effect->get_feedback_level_db() > -60.0f) time += effect->get_feedback_delay_ms() * 2.0f;
		}
		return MAX(1000.0f, std::ceil(time / 500.0f) * 500.0f);
	}

	float FmodAudioEffectDelayPreview::_time_radius(float p_time) const {
		return _inner_radius() + p_time / _time_range() * (_radius() - _inner_radius());
	}

	Vector2 FmodAudioEffectDelayPreview::_tap_position(int p_index) const {
		const Tap tap = _tap(p_index);
		const float angle = (tap.pan - 1.0f) * PI * 0.5f;
		return _center() + Vector2(std::cos(angle), std::sin(angle)) * _time_radius(tap.time);
	}

	int FmodAudioEffectDelayPreview::_hit_tap(const Vector2& p_position, bool p_alternate) const {
		if (effect.is_null()) return -1;
		const float threshold = 100.0f * _scale() * _scale();
		const bool hit_0 = _tap(0).active && _tap_position(0).distance_squared_to(p_position) <= threshold;
		const bool hit_1 = _tap(1).active && _tap_position(1).distance_squared_to(p_position) <= threshold;
		if (hit_0 && hit_1) {
			if (p_alternate) return 1 - selected_tap;
			const float distance_0 = _tap_position(0).distance_squared_to(p_position);
			const float distance_1 = _tap_position(1).distance_squared_to(p_position);
			return Math::is_equal_approx(distance_0, distance_1) ? selected_tap : (distance_0 < distance_1 ? 0 : 1);
		}
		return hit_0 ? 0 : (hit_1 ? 1 : -1);
	}

	void FmodAudioEffectDelayPreview::set_effect(const Ref<FmodAudioEffectDelay>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragging = false;
		hovered_tap = -1;
		selected_tap = 0;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectDelayPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectDelayPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectDelayPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectDelayPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectDelayPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectDelayPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectDelayPreview::_on_effect_changed() {
		geometry_dirty = true;
		if (effect.is_valid()) {
			String tooltip = tr("Delay tap layout: radius shows time, angle shows pan, and brightness shows level. Drag a node to edit time and pan. Alt + click selects an overlapping tap.");
			tooltip += "\n" + tr("Time range") + ": 0 - " + String::num(_time_range(), 0) + " ms";
			for (int i = 0; i < 2; i++) {
				const Tap tap = _tap(i);
				tooltip += "\nTap " + String::num_int64(i + 1) + ": " + (tap.active ? String::num(tap.time, 1) + " ms, " + String::num(tap.level, 1) + " dB, Pan " + String::num(tap.pan, 2) : tr("Disabled"));
			}
			if (effect->is_feedback_active()) tooltip += "\n" + tr("Feedback rings show repeat timing and gain before low-pass filtering.");
			set_tooltip_text(tooltip);
		}
		queue_redraw();
	}

	void FmodAudioEffectDelayPreview::_notification(int p_what) {
		switch (p_what) {
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
			hovered_tap = -1;
			if (!dragging) set_default_cursor_shape(CURSOR_ARROW);
			queue_redraw();
			break;
		}
	}

	void FmodAudioEffectDelayPreview::_rebuild_geometry() {
		geometry_dirty = false;
		echoes.clear();
		const Vector2 center = _center();
		std::array<Vector2, ARC_SEGMENTS + 1> offsets;
		for (int i = 0; i <= ARC_SEGMENTS; i++) {
			const float angle = -PI + PI * float(i) / float(ARC_SEGMENTS);
			offsets[i] = Vector2(std::cos(angle), std::sin(angle));
		}
		const auto make_arc = [&](float p_radius) {
			PackedVector2Array arc;
			arc.resize(ARC_SEGMENTS + 1);
			for (int i = 0; i <= ARC_SEGMENTS; i++) arc.set(i, center + offsets[i] * p_radius);
			return arc;
		};
		for (int i = 0; i < int(grid_arcs.size()); i++) grid_arcs[i] = make_arc(_radius() * float(i + 1) / float(grid_arcs.size()));
		for (int i = 0; i < 2; i++) {
			const Tap tap = _tap(i);
			tap_arcs[i] = tap.active ? make_arc(_time_radius(tap.time)) : PackedVector2Array();
		}
		if (!effect->is_feedback_active()) return;
		const float feedback_gain = float(FmodUtils::db_to_linear(effect->get_feedback_level_db()));
		const float interval = effect->get_feedback_delay_ms();
		// 小于一个显示像素的重复不叠画，0 ms 的反馈在音频线程中按一个采样处理
		if (feedback_gain <= 0.0f || interval / _time_range() * (_radius() - _inner_radius()) < _scale()) return;
		const auto add_echoes = [&](float p_start, float p_gain) {
			for (int repeat = 1; repeat <= 6; repeat++) {
				const float time = p_start + interval * float(repeat);
				const float gain = p_gain * std::pow(feedback_gain, float(repeat));
				if (time > _time_range() || gain < 0.003f) break;
				echoes.push_back({ make_arc(_time_radius(time)), gain });
			}
		};
		add_echoes(0.0f, effect->get_dry());
		for (int i = 0; i < 2; i++) {
			const Tap tap = _tap(i);
			if (tap.active) add_echoes(tap.time, float(FmodUtils::db_to_linear(tap.level)));
		}
	}

	void FmodAudioEffectDelayPreview::_edit_tap_at(const Vector2& p_position, bool p_changing) {
		if (effect.is_null() || is_read_only() || !_tap(selected_tap).active || _radius() <= _inner_radius()) return;
		const Vector2 offset = p_position - _center();
		const Tap tap = _tap(selected_tap);
		const float time = CLAMP((offset.length() - _inner_radius()) / (_radius() - _inner_radius()) * drag_range, 0.0f, 3000.0f);
		const float angle = offset.y >= 0.0f ? (offset.x < 0.0f ? -PI : 0.0f) : std::atan2(offset.y, offset.x);
		const float pan = offset.length_squared() < 1.0f ? tap.pan : CLAMP(angle / PI * 2.0f + 1.0f, -1.0f, 1.0f);
		if (Math::is_equal_approx(time, tap.time) && Math::is_equal_approx(pan, tap.pan) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		EditorUndoRedoManager* undo_redo = EditorInterface::get_singleton()->get_editor_undo_redo();
		undo_redo->create_action(tr("Move delay tap") + " " + String::num_int64(selected_tap + 1), UndoRedo::MERGE_ENDS, effect.ptr());
		PackedStringArray properties;
		const String prefix = selected_tap == 0 ? "tap1_" : "tap2_";
		properties.push_back(prefix + String("delay_ms"));
		properties.push_back(prefix + String("pan"));
		Array values;
		values.push_back(time);
		values.push_back(pan);
		emit_signal("multiple_properties_changed", properties, values, p_changing);
		undo_redo->commit_action();
	}

	void FmodAudioEffectDelayPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				const int hit = _hit_tap(button->get_position(), button->is_alt_pressed());
				if (hit < 0) return;
				selected_tap = hit;
				drag_range = _time_range();
				dragging = true;
				gesture_changed = false;
			}
			else if (dragging) {
				if (!button->is_canceled()) _edit_tap_at(button->get_position(), false);
				dragging = false;
				_on_effect_changed();
				hovered_tap = _hit_tap(button->get_position());
			}
			else return;
			set_default_cursor_shape(dragging || hovered_tap >= 0 ? CURSOR_MOVE : CURSOR_ARROW);
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_tap_at(motion->get_position(), true);
				else {
					dragging = false;
					_on_effect_changed();
				}
				accept_event();
			}
			const int hovered = _hit_tap(motion->get_position());
			if (hovered_tap != hovered) {
				hovered_tap = hovered;
				queue_redraw();
			}
			set_default_cursor_shape(dragging || hovered_tap >= 0 ? CURSOR_MOVE : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectDelayPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectDelayPreview::_set_read_only(bool p_read_only) {
		dragging = false;
		hovered_tap = -1;
		set_default_cursor_shape(CURSOR_ARROW);
		_on_effect_changed();
	}

	void FmodAudioEffectDelayPreview::_draw() {
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const float scale = _scale();
		if (effect.is_null() || _radius() < 48.0f * scale) return;
		if (geometry_dirty) _rebuild_geometry();
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		for (int i = 0; i < int(grid_arcs.size()); i++) {
			Color color = accent;
			color.a *= 0.16f - 0.08f * float(i) / float(grid_arcs.size() - 1);
			draw_polyline(grid_arcs[i], color, scale, true);
		}
		for (const Echo& echo : echoes) {
			Color color = accent;
			color.a *= 0.4f * std::sqrt(echo.gain);
			draw_polyline(echo.arc, color, scale, true);
		}
		for (int i = 0; i < 2; i++) {
			const Tap tap = _tap(i);
			if (!tap.active) continue;
			const float gain = float(FmodUtils::db_to_linear(tap.level));
			Color color = gain > 0.0f ? accent : muted;
			color.a *= gain > 0.0f ? 0.3f + 0.7f * std::sqrt(gain) : 0.4f;
			if (gain > 0.0f) draw_polyline(tap_arcs[i], color, 2.0f * scale, true);
			const bool highlighted = !is_read_only() && (hovered_tap == i || (dragging && selected_tap == i));
			draw_circle(_tap_position(i), (highlighted ? 4.5f : 3.5f) * scale, color, true, -1.0f, true);
		}
		if (effect->get_dry() > 0.0f) {
			Color color = accent;
			color.a *= 0.25f * effect->get_dry();
			draw_circle(_center(), 2.0f * scale, color, true, -1.0f, true);
		}
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float y = 8.0f * scale + font->get_ascent(font_size);
		draw_string(font, Vector2(8.0f * scale, y), "L", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		const float width = font->get_string_size("R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
		draw_string(font, Vector2(get_size().x - 8.0f * scale - width, y), "R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
	}
}
