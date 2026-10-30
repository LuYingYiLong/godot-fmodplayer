#include "fmod_audio_effect_hard_limiter_preview.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	void FmodAudioEffectHardLimiterPreview::_bind_methods() {
	}

	FmodAudioEffectHardLimiterPreview::FmodAudioEffectHardLimiterPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		set_tooltip_text(tr("Steady-state input/output limiting curve. Drag the round node vertically for Pre Gain and the square node for Ceiling. Release controls recovery time."));
	}

	float FmodAudioEffectHardLimiterPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectHardLimiterPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 12.0f) * scale, Vector2(MAX(get_size().x - 64.0f * scale, 0.0f), MAX(get_size().y - 24.0f * scale, 0.0f)));
	}

	Vector2 FmodAudioEffectHardLimiterPreview::_point(float p_input, float p_output) const {
		const Rect2 plot = _plot_rect();
		return plot.position + Vector2((p_input + 80.0f) / 80.0f * plot.size.x, (6.0f - p_output) / 86.0f * plot.size.y);
	}

	Vector2 FmodAudioEffectHardLimiterPreview::_handle_position(int p_handle) const {
		if (p_handle == 1) return _point(0.0f, effect->get_ceiling_db());
		// 该输入点在全部参数范围内均位于限幅拐点之前
		return _point(-56.0f, -56.0f + effect->get_pre_gain_db());
	}

	int FmodAudioEffectHardLimiterPreview::_hit_handle(const Vector2& p_position) const {
		if (effect.is_null()) return -1;
		const float gain_distance = _handle_position(0).distance_squared_to(p_position);
		const float ceiling_distance = _handle_position(1).distance_squared_to(p_position);
		if (MIN(gain_distance, ceiling_distance) > 100.0f * _scale() * _scale()) return -1;
		return gain_distance <= ceiling_distance ? 0 : 1;
	}

	void FmodAudioEffectHardLimiterPreview::set_effect(const Ref<FmodAudioEffectHardLimiter>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragged_handle = hovered_handle = -1;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectHardLimiterPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectHardLimiterPreview::_on_effect_changed))) effect->connect("changed", callable_mp(this, &FmodAudioEffectHardLimiterPreview::_on_effect_changed));
	}

	void FmodAudioEffectHardLimiterPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectHardLimiterPreview::_on_effect_changed))) effect->disconnect("changed", callable_mp(this, &FmodAudioEffectHardLimiterPreview::_on_effect_changed));
	}

	void FmodAudioEffectHardLimiterPreview::_on_effect_changed() { curve_dirty = true; queue_redraw(); }

	void FmodAudioEffectHardLimiterPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragged_handle = -1;
			_disconnect_effect();
			break;
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered_handle = -1;
			if (dragged_handle < 0) set_default_cursor_shape(CURSOR_ARROW);
			queue_redraw();
			break;
		}
	}

	void FmodAudioEffectHardLimiterPreview::_rebuild_curve() {
		curve_dirty = false;
		curve.clear();
		const float gain = effect->get_pre_gain_db();
		const float ceiling = effect->get_ceiling_db();
		const float begin = MAX(-80.0f, -80.0f - gain);
		curve.push_back(_point(begin, MIN(begin + gain, ceiling)));
		const float knee = ceiling - gain;
		if (knee > begin && knee < 0.0f) curve.push_back(_point(knee, ceiling));
		curve.push_back(_point(0.0f, MIN(gain, ceiling)));
	}

	void FmodAudioEffectHardLimiterPreview::_edit_at(float p_y, bool p_changing) {
		if (effect.is_null() || is_read_only() || dragged_handle < 0 || _plot_rect().size.y <= 0.0f) return;
		const bool ceiling = dragged_handle == 1;
		const float value = CLAMP(drag_start_value + (drag_start_y - p_y) / _plot_rect().size.y * 86.0f, -24.0f, ceiling ? 0.0f : 24.0f);
		const float current = ceiling ? effect->get_ceiling_db() : effect->get_pre_gain_db();
		if (Math::is_equal_approx(value, current) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed(ceiling ? "ceiling_db" : "pre_gain_db", value, StringName(), p_changing);
	}

	void FmodAudioEffectHardLimiterPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragged_handle = _hit_handle(button->get_position());
				if (dragged_handle < 0) return;
				hovered_handle = dragged_handle;
				drag_start_y = button->get_position().y;
				drag_start_value = dragged_handle == 1 ? effect->get_ceiling_db() : effect->get_pre_gain_db();
				gesture_changed = false;
			}
			else if (dragged_handle >= 0) {
				if (!button->is_canceled()) _edit_at(button->get_position().y, false);
				dragged_handle = -1;
				hovered_handle = _hit_handle(button->get_position());
			}
			else return;
			set_default_cursor_shape(dragged_handle >= 0 || hovered_handle >= 0 ? CURSOR_VSIZE : CURSOR_ARROW);
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragged_handle >= 0) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_at(motion->get_position().y, true);
				else dragged_handle = -1;
				accept_event();
			}
			const int hovered = _hit_handle(motion->get_position());
			if (hovered != hovered_handle) { hovered_handle = hovered; queue_redraw(); }
			set_default_cursor_shape(dragged_handle >= 0 || hovered_handle >= 0 ? CURSOR_VSIZE : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectHardLimiterPreview::_update_property() { _on_effect_changed(); }
	void FmodAudioEffectHardLimiterPreview::_set_read_only(bool p_read_only) { dragged_handle = hovered_handle = -1; set_default_cursor_shape(CURSOR_ARROW); queue_redraw(); }

	void FmodAudioEffectHardLimiterPreview::_draw() {
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 48.0f * scale || plot.size.y < 32.0f * scale) return;
		if (curve_dirty) _rebuild_curve();
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		Color grid = muted;
		grid.a *= 0.5f;
		draw_line(_point(-80.0f, -80.0f), _point(0.0f, 0.0f), grid, scale, true);
		draw_line(_point(-80.0f, 0.0f), _point(0.0f, 0.0f), grid, scale);
		draw_polyline(curve, accent, 2.0f * scale, true);
		for (int handle : { 0, 1 }) {
			const bool selected = !is_read_only() && (hovered_handle == handle || dragged_handle == handle);
			const float radius = (selected ? 5.0f : 3.5f) * scale;
			const Vector2 position = _handle_position(handle);
			const Color color = selected ? accent : muted;
			if (handle == 0) {
				draw_circle(position, radius, color, true, -1.0f, true);
			}
			else {
				draw_rect(Rect2(position - Vector2(radius, radius), Vector2(radius, radius) * 2.0f), color);
			}
		}
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		for (int output : { 0, -12, -24, -48, -80 }) {
			const float y = _point(0.0f, float(output)).y;
			draw_line(Vector2(plot.get_end().x + 8.0f * scale, y), Vector2(plot.get_end().x + 12.0f * scale, y), muted, scale);
			const String label = output == 0 ? String("0 dB") : (output > 0 ? String("+") : String()) + String::num_int64(output);
			draw_string(font, Vector2(plot.get_end().x + 16.0f * scale, y + font->get_ascent(font_size) * 0.4f), label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		}
	}
}
