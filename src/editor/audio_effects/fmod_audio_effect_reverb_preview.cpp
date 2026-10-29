#include "fmod_audio_effect_reverb_preview.h"

#include <godot_cpp/classes/editor_interface.hpp>
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
		set_tooltip_text(tr("Reverb space illustration. Room Size and Spread shape the rings; Damping fades reflections, Wet controls their brightness, and Dry controls the central source."));
	}

	float FmodAudioEffectReverbPreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	void FmodAudioEffectReverbPreview::set_effect(const Ref<FmodAudioEffectReverb>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
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

	void FmodAudioEffectReverbPreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectReverbPreview::_rebuild_geometry() {
		geometry_dirty = false;
		constexpr int segments = 64;
		constexpr double pi = 3.141592653589793;
		const float scale = _scale();
		const Vector2 available = get_size() - Vector2(20.0f, 20.0f) * scale;
		const Vector2 center = get_size() * 0.5f;
		const float room = effect->get_room_size();
		const float feedback = effect->get_predelay_feedback();
		const float height = available.y * (0.62f + 0.30f * room + 0.08f * feedback);
		const float radius_x = MIN(available.x * 0.44f, height * (0.34f + 0.16f * effect->get_spread()));
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

	void FmodAudioEffectReverbPreview::_draw() {
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		draw_rect(Rect2(Vector2(), get_size()), background);
		const float scale = _scale();
		if (effect.is_null() || get_size().x < 48.0f * scale || get_size().y < 48.0f * scale) return;
		if (geometry_dirty) _rebuild_geometry();
		const float brightness = std::sqrt(effect->get_wet());
		if (brightness > 0.0f) {
			for (int i = int(rings.size()) - 1; i >= 0; i--) {
				Color color = accent;
				color.a *= brightness * (0.06f + 0.84f * rings[i].energy);
				draw_polyline(rings[i].back, color, 2.0f * scale, true);
			}
		}
		if (effect->get_dry() > 0.0f) {
			Color color = accent;
			color.a *= effect->get_dry();
			draw_colored_polygon(source_marker, color);
		}
		if (brightness > 0.0f) {
			for (int i = int(rings.size()) - 1; i >= 0; i--) {
				Color color = accent;
				color.a *= brightness * (0.04f + 0.96f * rings[i].energy);
				draw_polyline(rings[i].front, color, 2.0f * scale, true);
			}
		}
	}
}
