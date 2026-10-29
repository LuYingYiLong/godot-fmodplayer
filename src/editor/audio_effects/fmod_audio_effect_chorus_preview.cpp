#include "fmod_audio_effect_chorus_preview.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <cmath>

namespace godot {
	void FmodAudioEffectChorusPreview::_bind_methods() {
	}

	FmodAudioEffectChorusPreview::FmodAudioEffectChorusPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		set_tooltip_text(tr("One-second chorus modulation overview. Vertical position represents pan; rate and depth shape each voice. Curves begin at the base delay."));
	}

	float FmodAudioEffectChorusPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectChorusPreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(8.0f, 22.0f) * scale, Vector2(MAX(get_size().x - 40.0f * scale, 0.0f), MAX(get_size().y - 56.0f * scale, 0.0f)));
	}

	Color FmodAudioEffectChorusPreview::_voice_color(int p_voice) const {
		static const Color defaults[] = {
			Color(0.42f, 0.38f, 0.78f),
			Color(0.35f, 0.65f, 0.31f),
			Color(0.74f, 0.73f, 0.35f),
			Color(0.75f, 0.40f, 0.43f),
		};
		const StringName name = "voice_" + String::num_int64(p_voice + 1) + "_color";
		return has_theme_color(name) ? get_theme_color(name) : defaults[p_voice];
	}

	void FmodAudioEffectChorusPreview::set_effect(const Ref<FmodAudioEffectChorus>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
	}

	void FmodAudioEffectChorusPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectChorusPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectChorusPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectChorusPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectChorusPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectChorusPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectChorusPreview::_on_effect_changed() {
		curves_dirty = true;
		queue_redraw();
	}

	void FmodAudioEffectChorusPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			_disconnect_effect();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		}
	}

	void FmodAudioEffectChorusPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectChorusPreview::_rebuild_curves() {
		curves_dirty = false;
		constexpr int points = 160;
		constexpr double tau = 6.283185307179586;
		const Rect2 plot = _plot_rect();
		const float amplitude = plot.size.y * 0.2f;
		for (int voice = 0; voice < FmodAudioEffectChorus::MAX_VOICES; voice++) {
			curves[voice].clear();
			if (voice >= effect->get_voice_count()) continue;
			curves[voice].resize(points);
			const float pan = (effect->get_voice_pan(voice) + 1.0f) * 0.5f;
			const float center = Math::lerp(plot.position.y + amplitude, plot.get_end().y - amplitude, pan);
			const double delay = effect->get_voice_delay_ms(voice) / 1000.0;
			const double rate = effect->get_voice_rate_hz(voice);
			const float depth = effect->get_voice_depth_ms(voice) / float(FmodAudioEffectChorus::MAX_DEPTH_MS);
			for (int point = 0; point < points; point++) {
				const double time = delay + (1.0 - delay) * double(point) / double(points - 1);
				curves[voice].set(point, Vector2(plot.position.x + float(time) * plot.size.x, center + float(std::sin(tau * rate * time)) * amplitude * depth));
			}
		}
	}

	void FmodAudioEffectChorusPreview::_draw() {
		const Vector2 size = get_size();
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color text = get_theme_color("font_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		draw_rect(Rect2(Vector2(), size), background);
		const float scale = _scale();
		const Rect2 plot = _plot_rect();
		if (effect.is_null() || plot.size.x < 32.0f * scale || plot.size.y < 24.0f * scale) return;
		if (curves_dirty) _rebuild_curves();
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float ascent = font->get_ascent(font_size);
		const float baseline = size.y - 8.0f * scale - font->get_descent(font_size);
		const float label_x = size.x - 8.0f * scale - font->get_string_size("R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
		draw_string(font, Vector2(label_x, 8.0f * scale + ascent), "L", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		draw_string(font, Vector2(label_x, baseline), "R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, muted);
		for (int voice = 0; voice < FmodAudioEffectChorus::MAX_VOICES; voice++) {
			Color color = _voice_color(voice);
			const bool active = voice < effect->get_voice_count();
			if (active) draw_polyline(curves[voice], color, 2.0f * scale, true);
			else color.a *= 0.35f;
			const float x = (8.0f + 36.0f * voice) * scale;
			if (x + 26.0f * scale >= label_x - 8.0f * scale) continue;
			draw_rect(Rect2(Vector2(x, baseline - 8.0f * scale), Vector2(7.0f, 7.0f) * scale), color);
			draw_string(font, Vector2(x + 12.0f * scale, baseline), String::num_int64(voice + 1), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, active ? text : muted);
		}
	}
}
