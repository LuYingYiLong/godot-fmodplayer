#include "fmod_audio_effect_eq_preview.h"

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
		constexpr float MIN_FREQUENCY = 10.0f;
		constexpr float MAX_FREQUENCY = 22000.0f;
		constexpr float MIN_GAIN = -60.0f;
		constexpr float MAX_GAIN = 24.0f;
		constexpr float MIN_Q = 0.01f;
		constexpr float MAX_Q = 10.0f;
		constexpr float Q_SCROLL_RATIO = 1.1f;
		constexpr int CURVE_POINTS = 384;
		static_assert(int(fmod_eq_preview::Filter::HIGHPASS_6DB) == FmodAudioEffectEQ::FILTER_HIGHPASS_6DB);
		static_assert(int(fmod_eq_preview::Filter::PEAKING) == FmodAudioEffectEQ::FILTER_PEAKING);

		float frequency_ratio(float p_frequency) {
			return std::log(CLAMP(p_frequency, MIN_FREQUENCY, MAX_FREQUENCY) / MIN_FREQUENCY) / std::log(MAX_FREQUENCY / MIN_FREQUENCY);
		}

		float frequency_at(float p_ratio) {
			return MIN_FREQUENCY * std::pow(MAX_FREQUENCY / MIN_FREQUENCY, CLAMP(p_ratio, 0.0f, 1.0f));
		}

		String frequency_label(float p_frequency) {
			return p_frequency >= 1000.0f ? String::num(p_frequency / 1000.0f, 1) + "k" : String::num(p_frequency, 0);
		}
	}

	void FmodAudioEffectEQPreview::_bind_methods() {
	}

	FmodAudioEffectEQPreview::FmodAudioEffectEQPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
	}

	float FmodAudioEffectEQPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectEQPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 8.0f) * scale, Vector2(MAX(get_size().x - 16.0f * scale, 0.0f), MAX(get_size().y - 38.0f * scale, 0.0f)));
	}

	void FmodAudioEffectEQPreview::set_effect(const Ref<FmodAudioEffectEQ>& p_effect) {
		if (effect == p_effect) {
			return;
		}
		_disconnect_effect();
		effect = p_effect;
		band_count = effect.is_valid() ? effect->get_band_count() : 0;
		selected_band = -1;
		hovered_band = -1;
		dragging = false;
		if (is_inside_tree()) {
			_connect_effect();
		}
		curves_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectEQPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectEQPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectEQPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectEQPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectEQPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectEQPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectEQPreview::_on_effect_changed() {
		if (effect.is_valid() && band_count != effect->get_band_count()) {
			band_count = effect->get_band_count();
			selected_band = -1;
			hovered_band = -1;
			dragging = false;
		}
		curves_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectEQPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragging = false;
			_disconnect_effect();
			break;
		case NOTIFICATION_THEME_CHANGED:
			curves_dirty = true;
			queue_redraw();
			break;
		case NOTIFICATION_RESIZED:
			curves_dirty = true;
			queue_redraw();
			break;
		case NOTIFICATION_MOUSE_EXIT:
			hovered_band = -1;
			queue_redraw();
			break;
		}
	}

	bool FmodAudioEffectEQPreview::_uses_gain(int p_band) const {
		const FmodAudioEffectEQ::FilterType type = effect->get_band_filter_type(p_band);
		return type == FmodAudioEffectEQ::FILTER_PEAKING || type == FmodAudioEffectEQ::FILTER_LOWSHELF || type == FmodAudioEffectEQ::FILTER_HIGHSHELF;
	}

	Vector2 FmodAudioEffectEQPreview::_band_position(int p_band) const {
		const Rect2 plot = _plot_rect();
		const float gain = _uses_gain(p_band) ? effect->get_band_gain_db(p_band) : 0.0f;
		return plot.position + Vector2(frequency_ratio(effect->get_band_frequency(p_band)) * plot.size.x, (MAX_GAIN - gain) / (MAX_GAIN - MIN_GAIN) * plot.size.y);
	}

	int FmodAudioEffectEQPreview::_hit_band(const Vector2& p_position) const {
		if (effect.is_null()) {
			return -1;
		}
		float closest = 10.0f * _scale();
		closest *= closest;
		int result = -1;
		for (int i = 0; i < effect->get_band_count(); i++) {
			const float distance = _band_position(i).distance_squared_to(p_position);
			if (distance < closest || (distance == closest && i == selected_band)) {
				closest = distance;
				result = i;
			}
		}
		return result;
	}

	void FmodAudioEffectEQPreview::_edit_band_at(const Vector2& p_position, bool p_changing) {
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || is_read_only() || selected_band < 0 || selected_band >= effect->get_band_count() || plot.size.x <= 0.0f || plot.size.y <= 0.0f) {
			return;
		}
		const float frequency = frequency_at((p_position.x - plot.position.x) / plot.size.x);
		const float gain = _uses_gain(selected_band) ? Math::lerp(MAX_GAIN, MIN_GAIN, CLAMP((p_position.y - plot.position.y) / plot.size.y, 0.0f, 1.0f)) : effect->get_band_gain_db(selected_band);
		if (Math::is_equal_approx(frequency, effect->get_band_frequency(selected_band)) && Math::is_equal_approx(gain, effect->get_band_gain_db(selected_band)) && (p_changing || !gesture_changed)) {
			return;
		}
		gesture_changed = true;
		const String prefix = "band_" + String::num_int64(selected_band) + "_";
		if (_uses_gain(selected_band)) {
			EditorUndoRedoManager* undo_redo = EditorInterface::get_singleton()->get_editor_undo_redo();
			// 提前指定资源历史，避免检查器将两个属性分别提交为独立操作
			undo_redo->create_action(tr("Move EQ band") + String(" ") + String::num_int64(selected_band + 1), UndoRedo::MERGE_ENDS, effect.ptr());
			PackedStringArray properties;
			properties.push_back(prefix + String("frequency"));
			properties.push_back(prefix + String("gain"));
			Array values;
			values.push_back(frequency);
			values.push_back(gain);
			// 同一次编辑同时提交频率和增益，交由检查器合并撤销记录
			emit_signal("multiple_properties_changed", properties, values, p_changing);
			undo_redo->commit_action();
		}
		else {
			emit_changed(prefix + String("frequency"), frequency, StringName(), p_changing);
		}
	}

	void FmodAudioEffectEQPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) {
			return;
		}
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && (button->get_button_index() == MOUSE_BUTTON_WHEEL_UP || button->get_button_index() == MOUSE_BUTTON_WHEEL_DOWN)) {
			if (!button->is_pressed() || button->is_canceled() || dragging) {
				return;
			}
			const int band = _hit_band(button->get_position());
			const float factor = button->get_factor();
			if (band < 0 || !std::isfinite(factor) || factor <= 0.0f) {
				return;
			}
			const float direction = button->get_button_index() == MOUSE_BUTTON_WHEEL_UP ? 1.0f : -1.0f;
			const float current_q = effect->get_band_q(band);
			const float q = CLAMP(current_q * std::pow(Q_SCROLL_RATIO, direction * factor), MIN_Q, MAX_Q);
			selected_band = band;
			hovered_band = band;
			set_default_cursor_shape(CURSOR_MOVE);
			if (!Math::is_equal_approx(q, current_q)) {
				emit_changed("band_" + String::num_int64(band) + "_q", q);
			}
			queue_redraw();
			accept_event();
			return;
		}
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				selected_band = _hit_band(button->get_position());
				dragging = selected_band >= 0;
				gesture_changed = false;
				queue_redraw();
			}
			else if (dragging) {
				if (!button->is_canceled()) {
					_edit_band_at(button->get_position(), false);
				}
				dragging = false;
			}
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid()) {
			if (dragging) {
				if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) {
					_edit_band_at(motion->get_position(), true);
				}
				else {
					dragging = false;
				}
				accept_event();
			}
			else {
				const int hovered = _hit_band(motion->get_position());
				if (hovered != hovered_band) {
					hovered_band = hovered;
					set_default_cursor_shape(hovered >= 0 ? CURSOR_MOVE : CURSOR_ARROW);
					queue_redraw();
				}
			}
		}
	}

	void FmodAudioEffectEQPreview::_update_property() {
		curves_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectEQPreview::_set_read_only(bool p_read_only) {
		dragging = false;
		set_default_cursor_shape(p_read_only || hovered_band < 0 ? CURSOR_ARROW : CURSOR_MOVE);
		queue_redraw();
	}

	void FmodAudioEffectEQPreview::_rebuild_curves() {
		curves_dirty = false;
		total_curve.clear();
		band_curves.clear();
		if (effect.is_null()) {
			return;
		}
		FmodServer* server = FmodServer::get_singleton();
		Ref<FmodSystem> system = server ? server->get_main_system() : Ref<FmodSystem>();
		if (system.is_valid()) {
			sample_rate = MAX(float(system->get_software_format().get("sample_rate", 48000)), 100.0f);
		}
		const Rect2 plot = _plot_rect();
		const int count = effect->get_band_count();
		band_curves.resize(count);
		Vector<fmod_eq_preview::Band> responses;
		responses.resize(count);
		for (int band = 0; band < count; band++) {
			responses.write[band] = fmod_eq_preview::make_band(static_cast<fmod_eq_preview::Filter>(effect->get_band_filter_type(band)), effect->get_band_frequency(band), effect->get_band_gain_db(band), effect->get_band_q(band), sample_rate);
			band_curves.write[band].resize(CURVE_POINTS);
		}
		total_curve.resize(CURVE_POINTS);
		for (int i = 0; i < CURVE_POINTS; i++) {
			const float ratio = float(i) / float(CURVE_POINTS - 1);
			const float frequency = frequency_at(ratio);
			double total_db = 0.0;
			for (int band = 0; band < count; band++) {
				const double db = responses[band].response_db(frequency, sample_rate);
				total_db += db;
				band_curves.write[band].set(i, plot.position + Vector2(ratio * plot.size.x, (MAX_GAIN - CLAMP(float(db), MIN_GAIN, MAX_GAIN)) / (MAX_GAIN - MIN_GAIN) * plot.size.y));
			}
			total_curve.set(i, plot.position + Vector2(ratio * plot.size.x, (MAX_GAIN - CLAMP(float(total_db), MIN_GAIN, MAX_GAIN)) / (MAX_GAIN - MIN_GAIN) * plot.size.y));
		}
	}

	void FmodAudioEffectEQPreview::_draw() {
		const Vector2 size = get_size();
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color text = get_theme_color("font_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		draw_rect(Rect2(Vector2(), size), background);
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 24.0f * scale || plot.size.y < 32.0f * scale) {
			return;
		}
		if (curves_dirty) {
			_rebuild_curves();
		}
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float ascent = font->get_ascent(font_size);
		const float zero_y = plot.position.y + MAX_GAIN / (MAX_GAIN - MIN_GAIN) * plot.size.y;
		draw_line(Vector2(plot.position.x, zero_y), Vector2(plot.get_end().x, zero_y), muted, scale);
		const float frequencies[] = { 10.0f, 100.0f, 1000.0f, 10000.0f, 22000.0f };
		float label_end = -1.0f;
		for (float frequency : frequencies) {
			const float x = plot.position.x + frequency_ratio(frequency) * plot.size.x;
			draw_line(Vector2(x, plot.position.y), Vector2(x, plot.get_end().y), muted, scale);
			const String label = frequency_label(frequency);
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			const float start = CLAMP(x - width * 0.5f, 0.0f, MAX(size.x - width, 0.0f));
			if (start >= label_end + 3.0f * scale) {
				draw_string(font, Vector2(start, plot.get_end().y + 6.0f * scale + ascent), label, HORIZONTAL_ALIGNMENT_LEFT, width, font_size, muted);
				label_end = start + width;
			}
		}
		if (selected_band >= 0 && selected_band < band_curves.size()) {
			draw_polyline(band_curves[selected_band], text, scale, true);
		}
		draw_polyline(total_curve, accent, 2.0f * scale, true);
		for (int band = 0; band < effect->get_band_count(); band++) {
			const bool selected = band == selected_band || band == hovered_band;
			const bool disabled = effect->get_band_filter_type(band) == FmodAudioEffectEQ::FILTER_DISABLED;
			draw_circle(_band_position(band), (selected ? 5.0f : 3.5f) * scale, selected ? accent : muted, !disabled, disabled ? scale : -1.0f, true);
		}
	}
}
