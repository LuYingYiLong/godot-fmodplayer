#include "editor/fmod_editor_translation.h"
#include "fmod_audio_effect_compressor_preview.h"
#include "fmod_compressor_preview_response.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	namespace {
		constexpr float MIN_INPUT_DB = -80.0f;
		constexpr float MAX_INPUT_DB = 0.0f;
		constexpr float MIN_OUTPUT_DB = -80.0f;
		constexpr float MAX_OUTPUT_DB = 20.0f;
		constexpr int CURVE_POINTS = 161;
	}

	void FmodAudioEffectCompressorPreview::_bind_methods() {
	}

	FmodAudioEffectCompressorPreview::FmodAudioEffectCompressorPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		_update_cursor_and_tooltip();
	}

	float FmodAudioEffectCompressorPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectCompressorPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 12.0f) * scale, Vector2(MAX(get_size().x - 64.0f * scale, 0.0f), MAX(get_size().y - 24.0f * scale, 0.0f)));
	}

	Vector2 FmodAudioEffectCompressorPreview::_point(float p_input, float p_output) const {
		const Rect2 plot = _plot_rect();
		return plot.position + Vector2((p_input - MIN_INPUT_DB) / (MAX_INPUT_DB - MIN_INPUT_DB) * plot.size.x, (MAX_OUTPUT_DB - p_output) / (MAX_OUTPUT_DB - MIN_OUTPUT_DB) * plot.size.y);
	}

	Vector2 FmodAudioEffectCompressorPreview::_handle_position(Handle p_handle) const {
		const float input = p_handle == HANDLE_THRESHOLD ? effect->get_threshold() : MAX_INPUT_DB;
		return _point(input, float(fmod_compressor_preview::output_db(input, effect->get_threshold(), effect->get_ratio(), effect->get_gain(), effect->get_mix())));
	}

	FmodAudioEffectCompressorPreview::Handle FmodAudioEffectCompressorPreview::_hit_handle(const Vector2& p_position) const {
		if (effect.is_null()) {
			return HANDLE_NONE;
		}
		const float radius_squared = 100.0f * _scale() * _scale();
		const float threshold_distance = _handle_position(HANDLE_THRESHOLD).distance_squared_to(p_position);
		const float ratio_distance = _handle_position(HANDLE_RATIO).distance_squared_to(p_position);
		if (threshold_distance <= radius_squared && ratio_distance <= radius_squared && _handle_position(HANDLE_THRESHOLD).is_equal_approx(_handle_position(HANDLE_RATIO))) {
			return HANDLE_SHARED;
		}
		if (threshold_distance <= radius_squared && threshold_distance <= ratio_distance) {
			return HANDLE_THRESHOLD;
		}
		return ratio_distance <= radius_squared ? HANDLE_RATIO : HANDLE_NONE;
	}

	void FmodAudioEffectCompressorPreview::set_effect(const Ref<FmodAudioEffectCompressor>& p_effect) {
		if (effect == p_effect) {
			return;
		}
		_disconnect_effect();
		effect = p_effect;
		hovered_handle = HANDLE_NONE;
		dragging_handle = HANDLE_NONE;
		if (is_inside_tree()) {
			_connect_effect();
		}
		_on_effect_changed();
	}

	void FmodAudioEffectCompressorPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectCompressorPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectCompressorPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectCompressorPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectCompressorPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectCompressorPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectCompressorPreview::_on_effect_changed() {
		curve_dirty = true;
		_update_cursor_and_tooltip();
		queue_redraw();
	}

	void FmodAudioEffectCompressorPreview::_update_cursor_and_tooltip() {
		const Handle handle = dragging_handle != HANDLE_NONE ? dragging_handle : hovered_handle;
		set_default_cursor_shape(is_read_only() || handle == HANDLE_NONE ? CURSOR_ARROW : (handle == HANDLE_SHARED ? CURSOR_MOVE : (handle == HANDLE_THRESHOLD ? CURSOR_HSIZE : CURSOR_VSIZE)));
		if (effect.is_valid() && handle == HANDLE_THRESHOLD) {
			set_tooltip_text(fmod_editor_translate("Threshold") + ": " + String::num(effect->get_threshold(), 1) + " dB\n" + fmod_editor_translate("Drag horizontally to adjust."));
		}
		else if (effect.is_valid() && handle == HANDLE_RATIO) {
			set_tooltip_text(fmod_editor_translate("Ratio") + ": " + String::num(effect->get_ratio(), 1) + ":1\n" + fmod_editor_translate("Drag down to increase compression, up to decrease."));
		}
		else if (effect.is_valid() && handle == HANDLE_SHARED) {
			fmod_set_editor_tooltip(this, "Drag horizontally for threshold, vertically for ratio.");
		}
		else {
			fmod_set_editor_tooltip(this, "Static input/output curve including Gain and Mix. Drag the round node horizontally for threshold, the square node vertically for ratio. Attack/release timing is not shown.");
		}
	}

	void FmodAudioEffectCompressorPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
			fmod_refresh_editor_control(this);
			_on_effect_changed();
			break;
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragging_handle = HANDLE_NONE;
			_disconnect_effect();
			break;
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered_handle = HANDLE_NONE;
			_update_cursor_and_tooltip();
			queue_redraw();
			break;
		}
	}

	void FmodAudioEffectCompressorPreview::_edit_at(const Vector2& p_position, bool p_changing) {
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || is_read_only() || dragging_handle == HANDLE_NONE || plot.size.x <= 0.0f || plot.size.y <= 0.0f) {
			return;
		}
		if (dragging_handle == HANDLE_SHARED) {
			const Vector2 delta = p_position - drag_start_position;
			if (delta.length_squared() < _scale() * _scale()) {
				return;
			}
			dragging_handle = Math::abs(delta.x) >= Math::abs(delta.y) ? HANDLE_THRESHOLD : HANDLE_RATIO;
			_update_cursor_and_tooltip();
		}
		const bool threshold = dragging_handle == HANDLE_THRESHOLD;
		float value;
		if (threshold) {
			value = CLAMP(drag_start_threshold + (p_position.x - drag_start_position.x) / plot.size.x * (MAX_INPUT_DB - MIN_INPUT_DB), -60.0f, 0.0f);
		}
		else if (p_position.is_equal_approx(drag_start_position)) {
			value = drag_start_ratio;
		}
		else if (effect->get_mix() <= 0.000001f || effect->get_threshold() >= MAX_INPUT_DB) {
			// 纯干声和阈值为 0 时没有可反解的压缩段，改用相对位移调节 Ratio
			value = CLAMP(drag_start_ratio * std::pow(48.0f, (p_position.y - drag_start_position.y) / plot.size.y), 1.0f, 48.0f);
		}
		else {
			const float output = MAX_OUTPUT_DB - (p_position.y - plot.position.y) / plot.size.y * (MAX_OUTPUT_DB - MIN_OUTPUT_DB);
			value = float(fmod_compressor_preview::ratio_for_output(MAX_INPUT_DB, output, effect->get_threshold(), effect->get_gain(), effect->get_mix()));
		}
		const float current = threshold ? effect->get_threshold() : effect->get_ratio();
		if (Math::is_equal_approx(value, current) && (p_changing || !gesture_changed)) {
			return;
		}
		gesture_changed = true;
		emit_changed(threshold ? "threshold_db" : "ratio", value, StringName(), p_changing);
	}

	void FmodAudioEffectCompressorPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) {
			return;
		}
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragging_handle = _hit_handle(button->get_position());
				if (dragging_handle == HANDLE_NONE) {
					return;
				}
				drag_start_position = button->get_position();
				drag_start_threshold = effect->get_threshold();
				drag_start_ratio = effect->get_ratio();
				gesture_changed = false;
			}
			else if (dragging_handle != HANDLE_NONE) {
				if (!button->is_canceled()) {
					_edit_at(button->get_position(), false);
				}
				dragging_handle = HANDLE_NONE;
			}
			else {
				return;
			}
			_update_cursor_and_tooltip();
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging_handle != HANDLE_NONE) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) {
					_edit_at(motion->get_position(), true);
				}
				else {
					dragging_handle = HANDLE_NONE;
					_update_cursor_and_tooltip();
				}
				accept_event();
			}
			else {
				const Handle handle = _hit_handle(motion->get_position());
				if (handle != hovered_handle) {
					hovered_handle = handle;
					_update_cursor_and_tooltip();
					queue_redraw();
				}
			}
		}
	}

	void FmodAudioEffectCompressorPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectCompressorPreview::_set_read_only(bool p_read_only) {
		dragging_handle = HANDLE_NONE;
		set_default_cursor_shape(p_read_only || hovered_handle == HANDLE_NONE ? CURSOR_ARROW : (hovered_handle == HANDLE_SHARED ? CURSOR_MOVE : (hovered_handle == HANDLE_THRESHOLD ? CURSOR_HSIZE : CURSOR_VSIZE)));
		queue_redraw();
	}

	void FmodAudioEffectCompressorPreview::_rebuild_curve() {
		curve_dirty = false;
		curve.resize(CURVE_POINTS);
		for (int i = 0; i < CURVE_POINTS; i++) {
			const float input = Math::lerp(MIN_INPUT_DB, MAX_INPUT_DB, float(i) / float(CURVE_POINTS - 1));
			const float output = float(fmod_compressor_preview::output_db(input, effect->get_threshold(), effect->get_ratio(), effect->get_gain(), effect->get_mix()));
			curve.set(i, _point(input, CLAMP(output, MIN_OUTPUT_DB, MAX_OUTPUT_DB)));
		}
	}

	void FmodAudioEffectCompressorPreview::_draw() {
		const Vector2 size = get_size();
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		draw_rect(Rect2(Vector2(), size), background);
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 24.0f * scale || plot.size.y < 32.0f * scale) {
			return;
		}
		if (curve_dirty) {
			_rebuild_curve();
		}
		draw_line(_point(0.0f, 0.0f), _point(MIN_INPUT_DB, MIN_OUTPUT_DB), muted, scale);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		for (int output : { 20, 0, -20, -40, -60, -80 }) {
			const float y = _point(0.0f, float(output)).y;
			draw_line(Vector2(plot.get_end().x + 4.0f * scale, y), Vector2(plot.get_end().x + 8.0f * scale, y), muted, scale);
			const String label = output > 0 ? String("+") + String::num_int64(output) : (output == 0 ? String("0 dB") : String::num_int64(output));
			draw_string(font, Vector2(plot.get_end().x + 12.0f * scale, y + font->get_ascent(font_size) * 0.4f), label, HORIZONTAL_ALIGNMENT_LEFT, size.x - plot.get_end().x - 16.0f * scale, font_size, muted);
		}
		draw_polyline(curve, accent, 2.0f * scale, true);
		for (Handle handle : { HANDLE_THRESHOLD, HANDLE_RATIO }) {
			const bool selected = handle == hovered_handle || handle == dragging_handle || hovered_handle == HANDLE_SHARED || dragging_handle == HANDLE_SHARED;
			const float radius = (selected ? 5.0f : 3.5f) * scale;
			const Vector2 position = _handle_position(handle);
			if (handle == HANDLE_THRESHOLD) {
				draw_circle(position, radius, selected ? accent : muted, true, -1.0f, true);
			}
			else {
				draw_rect(Rect2(position - Vector2(radius, radius), Vector2(radius, radius) * 2.0f), selected ? accent : muted);
			}
		}
	}
}
