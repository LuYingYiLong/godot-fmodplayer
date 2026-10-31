#include "editor/fmod_editor_translation.h"
#include "fmod_audio_effect_phaser_preview.h"

#include "core/fmod_server.h"
#include "fmod_phaser_preview_response.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	namespace {
		constexpr float MIN_FREQUENCY = 10.0f;
		constexpr float MAX_FREQUENCY = 20000.0f;
		constexpr float MIN_GAIN_DB = -36.0f;
		constexpr float MAX_GAIN_DB = 36.0f;
		constexpr int CURVE_POINTS = 768;
	}

	void FmodAudioEffectPhaserPreview::_bind_methods() {
	}

	FmodAudioEffectPhaserPreview::FmodAudioEffectPhaserPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		fmod_set_editor_tooltip(this, "Output gain (solid, dB) and filtered-path phase (faint, degrees) at the sweep midpoint. The shaded range shows the LFO limits; drag its edges to adjust. Depth and Feedback shape the output response; Rate controls sweep speed.");
	}

	float FmodAudioEffectPhaserPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectPhaserPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 18.0f) * scale, Vector2(MAX(get_size().x - 16.0f * scale, 0.0f), MAX(get_size().y - 48.0f * scale, 0.0f)));
	}

	float FmodAudioEffectPhaserPreview::_frequency_x(float p_frequency) const {
		const Rect2 plot = _plot_rect();
		return plot.position.x + std::log(CLAMP(p_frequency, MIN_FREQUENCY, MAX_FREQUENCY) / MIN_FREQUENCY) / std::log(MAX_FREQUENCY / MIN_FREQUENCY) * plot.size.x;
	}

	int FmodAudioEffectPhaserPreview::_hit_range(const Vector2& p_position) const {
		if (effect.is_null() || !_plot_rect().grow(6.0f * _scale()).has_point(p_position)) return -1;
		const float min_distance = std::abs(p_position.x - _frequency_x(effect->get_range_min_hz()));
		const float max_distance = std::abs(p_position.x - _frequency_x(effect->get_range_max_hz()));
		if (MIN(min_distance, max_distance) > 8.0f * _scale()) return -1;
		if (Math::is_equal_approx(min_distance, max_distance)) return p_position.x <= _frequency_x(effect->get_range_min_hz()) ? 0 : 1;
		return min_distance < max_distance ? 0 : 1;
	}

	void FmodAudioEffectPhaserPreview::set_effect(const Ref<FmodAudioEffectPhaser>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		dragged_range = -1;
		hovered_range = -1;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectPhaserPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectPhaserPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectPhaserPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectPhaserPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectPhaserPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectPhaserPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectPhaserPreview::_on_effect_changed() {
		curve_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectPhaserPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
			fmod_refresh_editor_control(this);
			_on_effect_changed();
			break;
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragged_range = -1;
			_disconnect_effect();
			break;
		case NOTIFICATION_RESIZED:
			curve_dirty = true;
			queue_redraw();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered_range = -1;
			if (dragged_range < 0) set_default_cursor_shape(CURSOR_ARROW);
			queue_redraw();
			break;
		}
	}

	void FmodAudioEffectPhaserPreview::_rebuild_curve() {
		curve_dirty = false;
		phase_curves.clear();
		gain_curve.resize(CURVE_POINTS);
		float sample_rate = 48000.0f;
		FmodServer* server = FmodServer::get_singleton();
		Ref<FmodSystem> system = server ? server->get_main_system() : Ref<FmodSystem>();
		if (system.is_valid()) sample_rate = MAX(float(system->get_software_format().get("sample_rate", 48000)), 100.0f);
		const Rect2 plot = _plot_rect();
		PackedVector2Array segment;
		float previous_phase = 0.0f;
		int point_count = 0;
		for (int i = 0; i < CURVE_POINTS; i++) {
			const float ratio = float(i) / float(CURVE_POINTS - 1);
			const float frequency = MIN_FREQUENCY * std::pow(MAX_FREQUENCY / MIN_FREQUENCY, ratio);
			if (frequency >= sample_rate * 0.5f) break;
			const std::complex<double> filtered = fmod_phaser_preview::filtered_response(frequency, effect->get_range_min_hz(), effect->get_range_max_hz(), effect->get_feedback(), sample_rate);
			const float phase = float(std::arg(filtered) * 180.0 / fmod_phaser_preview::PI);
			const float gain = float(fmod_phaser_preview::gain_db(fmod_phaser_preview::mix_with_dry(filtered, effect->get_depth())));
			gain_curve.set(i, plot.position + Vector2(ratio * plot.size.x, (MAX_GAIN_DB - CLAMP(gain, MIN_GAIN_DB, MAX_GAIN_DB)) / (MAX_GAIN_DB - MIN_GAIN_DB) * plot.size.y));
			point_count++;
			// 主值在 ±180° 跳变时拆分，避免出现跨越整张图的竖线
			if (i > 0 && std::abs(phase - previous_phase) > 180.0f) {
				if (segment.size() >= 2) phase_curves.push_back(segment);
				segment = PackedVector2Array();
			}
			segment.push_back(plot.position + Vector2(ratio * plot.size.x, (phase + 180.0f) / 360.0f * plot.size.y));
			previous_phase = phase;
		}
		if (segment.size() >= 2) phase_curves.push_back(segment);
		gain_curve.resize(point_count);
	}

	void FmodAudioEffectPhaserPreview::_edit_range_at(float p_x, bool p_changing) {
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || is_read_only() || dragged_range < 0 || plot.size.x <= 0.0f) return;
		const float frequency = drag_start_frequency * std::pow(MAX_FREQUENCY / MIN_FREQUENCY, (p_x - drag_start_x) / plot.size.x);
		const float current = dragged_range == 0 ? effect->get_range_min_hz() : effect->get_range_max_hz();
		const float value = dragged_range == 0 ? CLAMP(frequency, 10.0f, effect->get_range_max_hz()) : CLAMP(frequency, effect->get_range_min_hz(), 10000.0f);
		if (Math::is_equal_approx(value, current) && (p_changing || !gesture_changed)) return;
		gesture_changed = true;
		emit_changed(dragged_range == 0 ? "range_min_hz" : "range_max_hz", value, StringName(), p_changing);
	}

	void FmodAudioEffectPhaserPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) return;
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragged_range = _hit_range(button->get_position());
				if (dragged_range < 0) return;
				drag_start_x = button->get_position().x;
				drag_start_frequency = dragged_range == 0 ? effect->get_range_min_hz() : effect->get_range_max_hz();
				gesture_changed = false;
			}
			else if (dragged_range >= 0) {
				if (!button->is_canceled()) _edit_range_at(button->get_position().x, false);
				dragged_range = -1;
				hovered_range = _hit_range(button->get_position());
			}
			else return;
			set_default_cursor_shape(hovered_range >= 0 || dragged_range >= 0 ? CURSOR_HSIZE : CURSOR_ARROW);
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragged_range >= 0) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) _edit_range_at(motion->get_position().x, true);
				else dragged_range = -1;
				accept_event();
			}
			const int hovered = _hit_range(motion->get_position());
			if (hovered != hovered_range) {
				hovered_range = hovered;
				queue_redraw();
			}
			set_default_cursor_shape(hovered_range >= 0 || dragged_range >= 0 ? CURSOR_HSIZE : CURSOR_ARROW);
		}
	}

	void FmodAudioEffectPhaserPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectPhaserPreview::_set_read_only(bool p_read_only) {
		dragged_range = -1;
		set_default_cursor_shape(CURSOR_ARROW);
		queue_redraw();
	}

	void FmodAudioEffectPhaserPreview::_draw() {
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 48.0f * scale || plot.size.y < 32.0f * scale) return;
		if (curve_dirty) _rebuild_curve();
		const float min_x = _frequency_x(effect->get_range_min_hz());
		const float max_x = _frequency_x(effect->get_range_max_hz());
		Color fill = accent;
		fill.a *= 0.12f;
		if (max_x > min_x) draw_rect(Rect2(Vector2(min_x, plot.position.y), Vector2(max_x - min_x, plot.size.y)), fill);
		Color zero = muted;
		zero.a *= 0.5f;
		draw_line(Vector2(plot.position.x, plot.get_center().y), Vector2(plot.get_end().x, plot.get_center().y), zero, scale);
		Color phase_color = accent;
		phase_color.a *= 0.25f;
		for (const PackedVector2Array& segment : phase_curves) draw_polyline(segment, phase_color, scale, true);
		if (gain_curve.size() >= 2) draw_polyline(gain_curve, accent, 2.0f * scale, true);
		if (!is_read_only()) {
			const int selected = dragged_range >= 0 ? dragged_range : hovered_range;
			if (selected >= 0) {
				const float x = selected == 0 ? min_x : max_x;
				draw_line(Vector2(x, plot.position.y), Vector2(x, plot.get_end().y), accent, scale);
			}
		}
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		draw_string(font, Vector2(plot.position.x, plot.position.y - 3.0f * scale), "+36 dB", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		draw_string(font, Vector2(plot.position.x, plot.get_center().y - 3.0f * scale), "0 dB", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		draw_string(font, Vector2(plot.position.x, plot.get_end().y - 3.0f * scale), "-36 dB", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		Color phase_label = muted;
		phase_label.a *= 0.65f;
		for (int mark = 0; mark < 2; mark++) {
			const String label = String::utf8(mark == 0 ? "-180°" : "180°");
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			const float y = (mark == 0 ? plot.position.y : plot.get_end().y) - 3.0f * scale;
			draw_string(font, Vector2(plot.get_end().x - width, y), label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, phase_label);
		}
		float label_end = -1.0f;
		for (float frequency : { 20.0f, 100.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f }) {
			const float x = _frequency_x(frequency);
			draw_line(Vector2(x, plot.get_end().y + 5.0f * scale), Vector2(x, plot.get_end().y + 9.0f * scale), muted, scale);
			const String label = frequency >= 1000.0f ? String::num(frequency / 1000.0f, 0) + "k" : String::num(frequency, 0);
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			const float start = CLAMP(x - width * 0.5f, 4.0f * scale, MAX(get_size().x - width - 4.0f * scale, 4.0f * scale));
			if (start >= label_end + 4.0f * scale) {
				draw_string(font, Vector2(start, plot.get_end().y + 12.0f * scale + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, width, font_size, muted);
				label_end = start + width;
			}
		}
	}
}
