#include "fmod_audio_effect_reverb_preview.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <array>
#include <cmath>

namespace godot {
	void FmodAudioEffectReverbPreview::_bind_methods() {
	}

	FmodAudioEffectReverbPreview::FmodAudioEffectReverbPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		set_tooltip_text(tr("Reverb parameter illustration. Drag either outer node horizontally to adjust Spread between the L/R reflection rings. Room Size shapes each ring group; Damping fades reflections, Wet controls their brightness, and Dry controls the central source."));
	}

	float FmodAudioEffectReverbPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectReverbPreview::_space_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(12.0f, 28.0f) * scale, Vector2(MAX(get_size().x - 24.0f * scale, 0.0f), MAX(get_size().y - 40.0f * scale, 0.0f)));
	}

	float FmodAudioEffectReverbPreview::_ring_height() const {
		return _space_rect().size.y * (0.62f + 0.30f * effect->get_room_size() + 0.08f * effect->get_predelay_feedback());
	}

	float FmodAudioEffectReverbPreview::_ring_radius_x() const {
		return MIN(_space_rect().size.x * 0.2f, _ring_height() * 0.4f);
	}

	float FmodAudioEffectReverbPreview::_max_separation() const {
		return MIN(42.0f * _scale(), _space_rect().size.x * 0.14f);
	}

	Vector2 FmodAudioEffectReverbPreview::_handle_position(int p_handle) const {
		const Vector2 center = _space_rect().get_center();
		const float offset = _ring_radius_x() + _max_separation() * effect->get_spread();
		return center + Vector2(p_handle == 0 ? -offset : offset, -_ring_height() * 0.17f);
	}

	int FmodAudioEffectReverbPreview::_hit_handle(const Vector2& p_position) const {
		if (effect.is_null() || _space_rect().size.x < 24.0f * _scale() || _space_rect().size.y < 32.0f * _scale()) return -1;
		const float left = _handle_position(0).distance_squared_to(p_position);
		const float right = _handle_position(1).distance_squared_to(p_position);
		if (MIN(left, right) > 100.0f * _scale() * _scale()) return -1;
		return left <= right ? 0 : 1;
	}

	void FmodAudioEffectReverbPreview::set_effect(const Ref<FmodAudioEffectReverb>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragged_handle = hovered_handle = -1;
		set_default_cursor_shape(CURSOR_ARROW);
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectReverbPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectReverbPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectReverbPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectReverbPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectReverbPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectReverbPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectReverbPreview::_on_effect_changed() {
		geometry_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectReverbPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragged_handle = hovered_handle = -1;
			set_default_cursor_shape(CURSOR_ARROW);
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

	void FmodAudioEffectReverbPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectReverbPreview::_set_read_only(bool p_read_only) {
		dragged_handle = hovered_handle = -1;
		set_default_cursor_shape(CURSOR_ARROW);
		queue_redraw();
	}

	void FmodAudioEffectReverbPreview::_edit_spread_at(float p_x, bool p_changing) {
		if (effect.is_null() || is_read_only() || dragged_handle < 0 || drag_max_separation <= 0.0f) return;
		const float direction = dragged_handle == 0 ? -1.0f : 1.0f;
		const float spread = CLAMP(drag_start_spread + (p_x - drag_start_x) * direction / drag_max_separation, 0.0f, 1.0f);
		if (Math::is_equal_approx(spread, effect->get_spread()) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed("spread", spread, StringName(), p_changing);
	}

	void FmodAudioEffectReverbPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragged_handle = _hit_handle(button->get_position());
				if (dragged_handle < 0) return;
				hovered_handle = dragged_handle;
				drag_start_x = button->get_position().x;
				drag_start_spread = effect->get_spread();
				drag_max_separation = _max_separation();
				gesture_changed = false;
			}
			else if (dragged_handle >= 0) {
				if (!button->is_canceled()) _edit_spread_at(button->get_position().x, false);
				dragged_handle = -1;
				hovered_handle = _hit_handle(button->get_position());
			}
			else return;
			set_default_cursor_shape(dragged_handle >= 0 || hovered_handle >= 0 ? CURSOR_HSIZE : CURSOR_ARROW);
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragged_handle >= 0) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_spread_at(motion->get_position().x, true);
				else dragged_handle = -1;
				queue_redraw();
				accept_event();
			}
			const int hovered = _hit_handle(motion->get_position());
			if (hovered != hovered_handle) { hovered_handle = hovered; queue_redraw(); }
			set_default_cursor_shape(dragged_handle >= 0 || hovered_handle >= 0 ? CURSOR_HSIZE : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectReverbPreview::_rebuild_geometry() {
		geometry_dirty = false;
		constexpr int segments = 64;
		constexpr double pi = 3.141592653589793;
		const float scale = _scale();
		const Vector2 center = _space_rect().get_center();
		const float feedback = effect->get_predelay_feedback();
		const float height = _ring_height();
		const float radius_x = _ring_radius_x();
		const float radius_y = height * 0.33f;
		const float stack_height = height - radius_y * 2.0f;
		const int count = 14 + int(std::round(feedback * 6.0f));
		const float spacing_power = 1.2f + effect->get_predelay_msec() / 500.0f * 0.45f;
		std::array<Vector2, segments + 1> front_offsets;
		for (int point = 0; point <= segments; point++) {
			const double angle = pi * double(point) / double(segments);
			front_offsets[point] = Vector2(float(std::cos(angle)) * radius_x, float(std::sin(angle)) * radius_y);
		}
		rings.resize(count);
		for (int i = 0; i < count; i++) {
			const float depth = float(i) / float(count - 1);
			const Vector2 ring_center(center.x, center.y - stack_height * 0.5f + std::pow(depth, spacing_power) * stack_height);
			Ring& ring = rings[i];
			ring.energy = std::pow(1.0f - depth, 0.8f + effect->get_damping() * 1.6f);
			ring.back.resize(segments + 1);
			ring.front.resize(segments + 1);
			for (int point = 0; point <= segments; point++) {
				ring.back.set(point, ring_center - front_offsets[point]);
				ring.front.set(point, ring_center + front_offsets[point]);
			}
		}
		source_marker.resize(24);
		for (int point = 0; point < 24; point++) {
			const double angle = 2.0 * pi * double(point) / 24.0;
			source_marker.set(point, center + Vector2(float(std::cos(angle)) * 3.6f, float(std::sin(angle)) * 2.4f) * scale);
		}
	}

	void FmodAudioEffectReverbPreview::_draw_reflections(bool p_front, float p_brightness) {
		const float line_width = 2.0f * _scale();
		const float separation = _max_separation() * effect->get_spread();
		const bool merged = Math::is_zero_approx(separation);
		const Color accent = get_theme_color("accent_color", "Editor");
		for (int channel = 0; channel < (merged ? 1 : 2); channel++) {
			draw_set_transform(Vector2(merged ? 0.0f : (channel == 0 ? -separation : separation), 0.0f));
			for (int i = int(rings.size()) - 1; i >= 0; i--) {
				Color color = accent;
				color.a *= p_brightness * (p_front ? 0.04f + 0.96f * rings[i].energy : 0.06f + 0.84f * rings[i].energy) * 0.73f;
				// 重合时合并两声道透明度，仅绘制一组环线
				if (merged) color.a = 1.0f - (1.0f - color.a) * (1.0f - color.a);
				draw_polyline(p_front ? rings[i].front : rings[i].back, color, line_width, true);
			}
		}
		draw_set_transform(Vector2());
	}

	void FmodAudioEffectReverbPreview::_draw() {
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		draw_rect(Rect2(Vector2(), get_size()), background);
		const float scale = _scale();
		if (effect.is_null() || _space_rect().size.x < 24.0f * scale || _space_rect().size.y < 32.0f * scale) return;
		if (geometry_dirty) _rebuild_geometry();
		const float brightness = std::sqrt(effect->get_wet());
		if (brightness > 0.0f) _draw_reflections(false, brightness);
		if (effect->get_dry() > 0.0f) {
			Color color = accent;
			color.a *= effect->get_dry();
			draw_colored_polygon(source_marker, color);
		}
		if (brightness > 0.0f) _draw_reflections(true, brightness);
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		for (int handle : { 0, 1 }) {
			const bool selected = !is_read_only() && (hovered_handle == handle || dragged_handle == handle);
			draw_circle(_handle_position(handle), (selected ? 5.0f : 3.5f) * scale, selected ? accent : muted, true, -1.0f, true);
		}
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float label_y = 8.0f * scale + font->get_ascent(font_size);
		draw_string(font, Vector2(8.0f * scale, label_y), "L", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		const float right_width = font->get_string_size("R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
		draw_string(font, Vector2(get_size().x - 8.0f * scale - right_width, label_y), "R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
	}
}
