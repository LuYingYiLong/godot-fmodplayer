#include "editor/fmod_editor_translation.h"
#include "fmod_audio_effect_spectrum_analyzer_preview.h"

#include "core/fmod_server.h"
#include <cmath>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	void FmodAudioEffectSpectrumAnalyzerPreview::_bind_methods() {
	}

	FmodAudioEffectSpectrumAnalyzerPreview::FmodAudioEffectSpectrumAnalyzerPreview() {
		set_mouse_filter(MOUSE_FILTER_IGNORE);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		fmod_set_editor_tooltip(this, "Spectrum of this analyzer instance in the editor process. L uses the accent color; R uses the text color. Attach the resource to a bus to receive audio. The annotation shows FFT frequency resolution.");
		refresh_timer = memnew(Timer);
		refresh_timer->set_wait_time(0.1);
		refresh_timer->connect("timeout", callable_mp(this, &FmodAudioEffectSpectrumAnalyzerPreview::_refresh_snapshot));
		add_child(refresh_timer);
	}

	float FmodAudioEffectSpectrumAnalyzerPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectSpectrumAnalyzerPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 24.0f) * scale, Vector2(MAX(get_size().x - 54.0f * scale, 0.0f), MAX(get_size().y - 54.0f * scale, 0.0f)));
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::set_effect(const Ref<FmodAudioEffectSpectrumAnalyzer>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		runtime_effect.unref();
		last_bins = -1;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
		_update_refresh_timer();
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectSpectrumAnalyzerPreview::_on_effect_changed))) effect->connect("changed", callable_mp(this, &FmodAudioEffectSpectrumAnalyzerPreview::_on_effect_changed));
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectSpectrumAnalyzerPreview::_on_effect_changed))) effect->disconnect("changed", callable_mp(this, &FmodAudioEffectSpectrumAnalyzerPreview::_on_effect_changed));
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_on_effect_changed() {
		geometry_dirty = true;
		_refresh_snapshot();
		queue_redraw();
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_update_refresh_timer() {
		if (!refresh_timer || !refresh_timer->is_inside_tree()) return;
		if (effect.is_valid() && is_visible_in_tree()) {
			if (refresh_timer->is_stopped()) refresh_timer->start();
			_refresh_snapshot();
		}
		else refresh_timer->stop();
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
			fmod_refresh_editor_control(this);
			_on_effect_changed();
			break;
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			callable_mp(this, &FmodAudioEffectSpectrumAnalyzerPreview::_update_refresh_timer).call_deferred();
			break;
		case NOTIFICATION_READY:
		case NOTIFICATION_VISIBILITY_CHANGED:
			_update_refresh_timer();
			break;
		case NOTIFICATION_EXIT_TREE:
			refresh_timer->stop();
			_disconnect_effect();
			break;
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		}
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_refresh_snapshot() {
		const Ref<FmodAudioEffectSpectrumAnalyzer> effect = runtime_effect.is_valid() ? runtime_effect : this->effect;
		if (effect.is_null() || !is_inside_tree() || !is_visible_in_tree()) return;
		// 每次只刷新一次共享快照，随后直接聚合正频率部分
		const int bins = effect->get_bin_count();
		if (bins == 0 && last_bins == 0 && !geometry_dirty) return;
		float rate = effect->sample_rate;
		if (bins == 0) {
			const Ref<FmodSystem> system = FmodServer::get_main_system();
			if (system.is_valid()) rate = float(system->get_software_format().get("sample_rate", 48000));
		}
		rate = MAX(rate, 100.0f);
		bool changed = geometry_dirty || last_bins != bins || sample_rate != rate;
		sample_rate = rate;
		last_bins = bins;
		const Rect2 plot = _plot_rect();
		int points = 0;
		for (int band = 0; band < int(magnitudes.size()); band++) {
			const float begin = 20.0f * std::pow(1000.0f, float(band) / float(magnitudes.size()));
			const float end = 20.0f * std::pow(1000.0f, float(band + 1) / float(magnitudes.size()));
			Vector2 peak;
			if (bins > 1 && begin < sample_rate * 0.5f) {
				const int first = CLAMP(int(std::floor(begin * bins / sample_rate)), 0, bins / 2);
				const int last = CLAMP(int(std::ceil(end * bins / sample_rate)), 0, bins / 2);
				for (int bin = first; bin <= last; bin++) {
					const Vector2 value = effect->spectrum_data[bin];
					peak.x = MAX(peak.x, value.x);
					peak.y = MAX(peak.y, value.y);
				}
				points++;
			}
			if (magnitudes[band] != peak) changed = true;
			magnitudes[band] = peak;
		}
		if (!changed) return;
		geometry_dirty = false;
		left_curve.resize(points);
		right_curve.resize(points);
		fill_lines.clear();
		for (int band = 0; band < points; band++) {
			const float x = plot.position.x + (float(band) + 0.5f) / float(magnitudes.size()) * plot.size.x;
			const Vector2 db = Vector2(20.0f * std::log10(MAX(float(magnitudes[band].x), 0.0001f)), 20.0f * std::log10(MAX(float(magnitudes[band].y), 0.0001f)));
			left_curve.set(band, Vector2(x, plot.position.y + CLAMP(-db.x / 80.0f, 0.0f, 1.0f) * plot.size.y));
			right_curve.set(band, Vector2(x, plot.position.y + CLAMP(-db.y / 80.0f, 0.0f, 1.0f) * plot.size.y));
			if (left_curve[band].y < plot.get_end().y - 0.25f * _scale()) {
				fill_lines.push_back(Vector2(x, plot.get_end().y));
				fill_lines.push_back(left_curve[band]);
			}
		}
		queue_redraw();
	}

	void FmodAudioEffectSpectrumAnalyzerPreview::_update_property() { _on_effect_changed(); }

	void FmodAudioEffectSpectrumAnalyzerPreview::_draw() {
		draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 48.0f * scale || plot.size.y < 32.0f * scale) return;
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		Color right_color = get_theme_color("font_color", "Editor");
		right_color.a *= 0.6f;
		Color fill = accent;
		fill.a *= 0.15f;
		draw_line(plot.position, Vector2(plot.get_end().x, plot.position.y), muted, scale);
		if (fill_lines.size() >= 2) draw_multiline(fill_lines, fill, plot.size.x / float(magnitudes.size()), false);
		if (left_curve.size() >= 2) draw_polyline(left_curve, accent, 1.5f * scale, true);
		if (right_curve.size() >= 2) draw_polyline(right_curve, right_color, scale, true);
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float top = 6.0f * scale + font->get_ascent(font_size);
		draw_string(font, Vector2(plot.position.x, top), "L", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, accent);
		draw_string(font, Vector2(plot.get_end().x - font->get_string_size("R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x, top), "R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, right_color);
		const String resolution = String::num(sample_rate / float(256 << int(effect->get_fft_size())), 1).trim_suffix(".0") + " Hz/bin";
		draw_string(font, Vector2(plot.position.x + 18.0f * scale, top), resolution, HORIZONTAL_ALIGNMENT_CENTER, plot.size.x - 36.0f * scale, font_size, muted);
		for (int db : { 0, -24, -48, -80 }) {
			const float y = plot.position.y - float(db) / 80.0f * plot.size.y;
			draw_line(Vector2(plot.get_end().x + 4.0f * scale, y), Vector2(plot.get_end().x + 8.0f * scale, y), muted, scale);
			draw_string(font, Vector2(plot.get_end().x + 12.0f * scale, y + font->get_ascent(font_size) * 0.4f), db == 0 ? String("0 dB") : String::num_int64(db), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		}
		float label_end = -1.0f;
		for (float frequency : { 20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f }) {
			const float x = plot.position.x + std::log(frequency / 20.0f) / std::log(1000.0f) * plot.size.x;
			const String label = frequency >= 1000.0f ? String::num(frequency / 1000.0f, 0) + "k" : String::num(frequency, 0);
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			const float start = MAX(4.0f * scale, x - width * 0.5f);
			if (start >= label_end + 4.0f * scale) {
				draw_string(font, Vector2(start, plot.get_end().y + 6.0f * scale + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
				label_end = start + width;
			}
		}
	}
}
