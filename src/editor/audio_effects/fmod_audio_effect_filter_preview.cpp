#include "editor/fmod_editor_translation.h"
#include "fmod_audio_effect_filter_preview.h"

#include "core/fmod_server.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
	namespace {
		constexpr float MIN_FREQUENCY = 1.0f;
		constexpr float MAX_FREQUENCY = 22000.0f;
		constexpr float MAX_CUTOFF = 20500.0f;
		constexpr float MIN_DB = -60.0f;
		constexpr float MAX_DB = 24.0f;
		constexpr int CURVE_POINTS = 384;

		float frequency_ratio(float p_frequency) {
			return std::log(CLAMP(p_frequency, MIN_FREQUENCY, MAX_FREQUENCY)) / std::log(MAX_FREQUENCY);
		}
	}

	void FmodAudioEffectFilterPreview::_bind_methods() {
	}

	FmodAudioEffectFilterPreview::FmodAudioEffectFilterPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		fmod_set_editor_tooltip(this, "Approximate low-pass response. The cutoff node stays on the 0 dB reference line. Drag horizontally to adjust cutoff, vertically to adjust resonance relative to its starting value (12/18/24 dB).");
	}

	float FmodAudioEffectFilterPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectFilterPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 8.0f) * scale, Vector2(MAX(get_size().x - 16.0f * scale, 0.0f), MAX(get_size().y - 38.0f * scale, 0.0f)));
	}

	void FmodAudioEffectFilterPreview::set_effect(const Ref<FmodAudioEffectFilter>& p_effect) {
		if (effect == p_effect) {
			return;
		}
		_disconnect_effect();
		effect = p_effect;
		dragging = false;
		hovered = false;
		if (is_inside_tree()) {
			_connect_effect();
		}
		_on_effect_changed();
	}

	void FmodAudioEffectFilterPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectFilterPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectFilterPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectFilterPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectFilterPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectFilterPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectFilterPreview::_on_effect_changed() {
		curve_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectFilterPreview::_notification(int p_what) {
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
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered = false;
			if (!dragging) set_default_cursor_shape(CURSOR_ARROW);
			queue_redraw();
			break;
		}
	}

	float FmodAudioEffectFilterPreview::_zero_line_y() const {
		const Rect2 plot = _plot_rect();
		return plot.position.y + MAX_DB / (MAX_DB - MIN_DB) * plot.size.y;
	}

	Vector2 FmodAudioEffectFilterPreview::_handle_position() const {
		const Rect2 plot = _plot_rect();
		return Vector2(plot.position.x + frequency_ratio(effect->get_cutoff_hz()) * plot.size.x, _zero_line_y());
	}

	bool FmodAudioEffectFilterPreview::_hit_handle(const Vector2& p_position) const {
		return effect.is_valid() && _handle_position().distance_squared_to(p_position) <= 100.0f * _scale() * _scale();
	}

	void FmodAudioEffectFilterPreview::_edit_at(const Vector2& p_position, bool p_changing) {
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || is_read_only() || plot.size.x <= 0.0f || plot.size.y <= 0.0f) {
			return;
		}
		const Vector2 delta = p_position - drag_start_position;
		const float cutoff = CLAMP(drag_start_cutoff * std::pow(MAX_FREQUENCY / MIN_FREQUENCY, delta.x / plot.size.x), MIN_FREQUENCY, MAX_CUTOFF);
		const float resonance = effect->get_db() == FmodAudioEffectFilter::FILTER_6DB ? effect->get_resonance() : CLAMP(drag_start_resonance - delta.y / plot.size.y, 0.0f, 1.0f);
		if (Math::is_equal_approx(cutoff, effect->get_cutoff_hz()) && Math::is_equal_approx(resonance, effect->get_resonance()) && (p_changing || !gesture_changed)) {
			return;
		}
		gesture_changed = true;
		if (effect->get_db() == FmodAudioEffectFilter::FILTER_6DB) {
			emit_changed("cutoff_hz", cutoff, StringName(), p_changing);
		}
		else {
			EditorUndoRedoManager* undo_redo = EditorInterface::get_singleton()->get_editor_undo_redo();
			undo_redo->create_action(fmod_editor_translate("Move filter cutoff"), UndoRedo::MERGE_ENDS, effect.ptr());
			PackedStringArray properties;
			properties.push_back("cutoff_hz");
			properties.push_back("resonance");
			Array values;
			values.push_back(cutoff);
			values.push_back(resonance);
			emit_signal("multiple_properties_changed", properties, values, p_changing);
			undo_redo->commit_action();
		}
	}

	void FmodAudioEffectFilterPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) {
			return;
		}
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragging = _hit_handle(button->get_position());
				gesture_changed = false;
				if (!dragging) {
					return;
				}
				drag_start_position = button->get_position();
				drag_start_cutoff = effect->get_cutoff_hz();
				drag_start_resonance = effect->get_resonance();
			}
			else if (dragging) {
				if (!button->is_canceled()) {
					_edit_at(button->get_position(), false);
				}
				dragging = false;
				hovered = _hit_handle(button->get_position());
			}
			else {
				return;
			}
			set_default_cursor_shape(dragging || hovered ? (effect->get_db() == FmodAudioEffectFilter::FILTER_6DB ? CURSOR_HSIZE : CURSOR_MOVE) : CURSOR_ARROW);
			queue_redraw();
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) {
					_edit_at(motion->get_position(), true);
				}
				else {
					dragging = false;
					hovered = _hit_handle(motion->get_position());
					set_default_cursor_shape(hovered ? (effect->get_db() == FmodAudioEffectFilter::FILTER_6DB ? CURSOR_HSIZE : CURSOR_MOVE) : CURSOR_ARROW);
				}
				accept_event();
			}
			else {
				const bool hit = _hit_handle(motion->get_position());
				if (hovered != hit) {
					hovered = hit;
					set_default_cursor_shape(hit ? (effect->get_db() == FmodAudioEffectFilter::FILTER_6DB ? CURSOR_HSIZE : CURSOR_MOVE) : CURSOR_ARROW);
					queue_redraw();
				}
			}
		}
	}

	void FmodAudioEffectFilterPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectFilterPreview::_set_read_only(bool p_read_only) {
		dragging = false;
		set_default_cursor_shape(p_read_only || !hovered ? CURSOR_ARROW : (effect.is_valid() && effect->get_db() == FmodAudioEffectFilter::FILTER_6DB ? CURSOR_HSIZE : CURSOR_MOVE));
		queue_redraw();
	}

	void FmodAudioEffectFilterPreview::_rebuild_curve() {
		curve_dirty = false;
		curve.clear();
		if (effect.is_null()) {
			return;
		}
		FmodServer* server = FmodServer::get_singleton();
		Ref<FmodSystem> system = server ? server->get_main_system() : Ref<FmodSystem>();
		if (system.is_valid()) {
			sample_rate = MAX(float(system->get_software_format().get("sample_rate", 48000)), 100.0f);
		}
		const Rect2 plot = _plot_rect();
		const fmod_eq_preview::Band response = fmod_filter_preview::make_response(effect->get_db(), effect->get_cutoff_hz(), effect->get_resonance(), sample_rate);
		curve.resize(CURVE_POINTS);
		for (int i = 0; i < CURVE_POINTS; i++) {
			const float ratio = float(i) / float(CURVE_POINTS - 1);
			const float db = float(response.response_db(std::pow(MAX_FREQUENCY, ratio), sample_rate));
			curve.set(i, plot.position + Vector2(ratio * plot.size.x, (MAX_DB - CLAMP(db, MIN_DB, MAX_DB)) / (MAX_DB - MIN_DB) * plot.size.y));
		}
	}

	void FmodAudioEffectFilterPreview::_draw() {
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
		const float zero_y = _zero_line_y();
		draw_line(Vector2(plot.position.x, zero_y), Vector2(plot.get_end().x, zero_y), muted, scale);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		float label_end = -1.0f;
		for (float frequency : { 1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 22000.0f }) {
			const String label = frequency >= 1000.0f ? String::num(frequency / 1000.0f, 1) + "k" : String::num(frequency, 0);
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			const float start = CLAMP(plot.position.x + frequency_ratio(frequency) * plot.size.x - width * 0.5f, 0.0f, MAX(size.x - width, 0.0f));
			if (start >= label_end + 3.0f * scale) {
				draw_string(font, Vector2(start, plot.get_end().y + 6.0f * scale + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, width, font_size, muted);
				label_end = start + width;
			}
		}
		draw_polyline(curve, accent, 2.0f * scale, true);
		draw_circle(_handle_position(), (hovered || dragging ? 5.0f : 3.5f) * scale, hovered || dragging ? accent : muted, true, -1.0f, true);
	}
}
