#include "fmod_audio_effect_distortion_preview.h"

#include "core/fmod_utils.hpp"
#include "dsp/fmod_distortion_shaper.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	void FmodAudioEffectDistortionPreview::_bind_methods() {
	}

	FmodAudioEffectDistortionPreview::FmodAudioEffectDistortionPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		set_tooltip_text(tr("Steady-state input/output amplitude curve. Drag vertically for Drive (Pre Gain in Overdrive mode); Shift + drag for Pre Gain; Ctrl + drag for Post Gain. The vertical scale fits the output. Keep HF controls the audio crossover; Oversample is stored only."));
	}

	float FmodAudioEffectDistortionPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectDistortionPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 12.0f) * scale, Vector2(MAX(get_size().x - 64.0f * scale, 0.0f), MAX(get_size().y - 42.0f * scale, 0.0f)));
	}

	Vector2 FmodAudioEffectDistortionPreview::_point(float p_input, float p_output) const {
		const Rect2 plot = _plot_rect();
		return plot.position + Vector2((p_input + 1.0f) * 0.5f * plot.size.x, (1.0f - p_output / output_span) * 0.5f * plot.size.y);
	}

	void FmodAudioEffectDistortionPreview::set_effect(const Ref<FmodAudioEffectDistortion>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragging = false;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectDistortionPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectDistortionPreview::_on_effect_changed))) effect->connect("changed", callable_mp(this, &FmodAudioEffectDistortionPreview::_on_effect_changed));
	}

	void FmodAudioEffectDistortionPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectDistortionPreview::_on_effect_changed))) effect->disconnect("changed", callable_mp(this, &FmodAudioEffectDistortionPreview::_on_effect_changed));
	}

	void FmodAudioEffectDistortionPreview::_on_effect_changed() {
		curve_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectDistortionPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragging = false;
			_disconnect_effect();
			break;
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			if (!dragging) set_default_cursor_shape(CURSOR_ARROW);
			break;
		}
	}

	void FmodAudioEffectDistortionPreview::_rebuild_curve() {
		curve_dirty = false;
		constexpr int points = 513;
		curve.resize(points);
		const float drive = effect->get_drive();
		const float pre_gain = float(FmodUtils::db_to_linear(effect->get_pre_gain()));
		const float post_gain = float(FmodUtils::db_to_linear(effect->get_post_gain()));
		const float atan_mult = std::pow(10.0f, drive * drive * 3.0f) - 1.0f + 0.001f;
		const float atan_div = 1.0f / (std::atan(atan_mult) * (1.0f + drive * 8.0f));
		const float lofi_mult = std::pow(2.0f, 2.0f + (1.0f - drive) * 14.0f);
		output_span = 1.0f;
		for (int i = 0; i < points; i++) {
			const float input = float(i) / float(points - 1) * 2.0f - 1.0f;
			const float output = fmod_distortion::process_sample(input * pre_gain, effect->get_mode(), drive, atan_mult, atan_div, lofi_mult) * post_gain;
			curve.set(i, Vector2(input, output));
			output_span = MAX(output_span, std::abs(output));
		}
		output_span = std::ceil(output_span * 4.0f) * 0.25f;
		for (int i = 0; i < points; i++) curve.set(i, _point(curve[i].x, curve[i].y));
	}

	void FmodAudioEffectDistortionPreview::_edit_at(float p_y, bool p_changing) {
		if (effect.is_null() || is_read_only() || _plot_rect().size.y <= 0.0f) return;
		const bool drive = drag_property == StringName("drive");
		const bool pre = drag_property == StringName("pre_gain");
		const float minimum = drive ? 0.0f : (pre ? -60.0f : -80.0f);
		const float maximum = drive ? 1.0f : (pre ? 60.0f : 24.0f);
		const float value = CLAMP(drag_start_value + (drag_start_y - p_y) / _plot_rect().size.y * (maximum - minimum), minimum, maximum);
		if (Math::is_equal_approx(value, float(effect->get(drag_property))) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed(drag_property, value, StringName(), p_changing);
	}

	void FmodAudioEffectDistortionPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				if (!_plot_rect().has_point(button->get_position())) return;
				drag_property = button->is_ctrl_pressed() ? "post_gain" : (button->is_shift_pressed() || effect->get_mode() == FmodAudioEffectDistortion::MODE_OVERDRIVE ? "pre_gain" : "drive");
				drag_start_value = effect->get(drag_property);
				drag_start_y = button->get_position().y;
				dragging = true;
				gesture_changed = false;
			}
			else if (dragging) {
				if (!button->is_canceled()) _edit_at(button->get_position().y, false);
				dragging = false;
			}
			else return;
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_at(motion->get_position().y, true);
				else dragging = false;
				accept_event();
			}
			set_default_cursor_shape(dragging || _plot_rect().has_point(motion->get_position()) ? CURSOR_VSIZE : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectDistortionPreview::_update_property() { _on_effect_changed(); }
	void FmodAudioEffectDistortionPreview::_set_read_only(bool p_read_only) { dragging = false; set_default_cursor_shape(CURSOR_ARROW); }

	void FmodAudioEffectDistortionPreview::_draw() {
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const Rect2 plot = _plot_rect();
		const float scale = _scale();
		if (effect.is_null() || plot.size.x < 48.0f * scale || plot.size.y < 32.0f * scale) return;
		if (curve_dirty) _rebuild_curve();
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		Color grid = muted;
		grid.a *= 0.5f;
		draw_line(Vector2(plot.position.x, plot.get_center().y), Vector2(plot.get_end().x, plot.get_center().y), grid, scale);
		draw_line(Vector2(plot.get_center().x, plot.position.y), Vector2(plot.get_center().x, plot.get_end().y), grid, scale);
		draw_line(_point(-1.0f, -1.0f), _point(1.0f, 1.0f), grid, scale, true);
		draw_polyline(curve, accent, 2.0f * scale, true);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const String limit = String::num(output_span, output_span < 10.0f ? 2 : 0).trim_suffix(".00");
		draw_string(font, Vector2(plot.get_end().x + 8.0f * scale, plot.position.y + font->get_ascent(font_size) * 0.4f), "+" + limit, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		draw_string(font, Vector2(plot.get_end().x + 8.0f * scale, plot.get_center().y + font->get_ascent(font_size) * 0.4f), "0", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		draw_string(font, Vector2(plot.get_end().x + 8.0f * scale, plot.get_end().y + font->get_ascent(font_size) * 0.4f), "-" + limit, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		for (int input : { -1, 0, 1 }) {
			const String label = input > 0 ? String("+1") : String::num_int64(input);
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			draw_string(font, Vector2(_point(float(input), 0.0f).x - width * 0.5f, plot.get_end().y + 6.0f * scale + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		}
	}
}
