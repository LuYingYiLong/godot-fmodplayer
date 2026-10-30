#ifndef FMOD_AUDIO_EFFECT_DELAY_PREVIEW_H
#define FMOD_AUDIO_EFFECT_DELAY_PREVIEW_H

#include "dsp/fmod_audio_effect_delay.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <array>
#include <vector>

namespace godot {
	class FmodAudioEffectDelayPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectDelayPreview, EditorProperty)

		struct Tap {
			bool active;
			float time;
			float level;
			float pan;
		};
		struct Echo {
			PackedVector2Array arc;
			float gain;
		};

		Ref<FmodAudioEffectDelay> effect;
		std::array<PackedVector2Array, 12> grid_arcs;
		std::array<PackedVector2Array, 2> tap_arcs;
		std::vector<Echo> echoes;
		bool geometry_dirty = true;
		bool dragging = false;
		bool gesture_changed = false;
		int selected_tap = 0;
		int hovered_tap = -1;
		float drag_range = 1000.0f;

		float _scale() const;
		Vector2 _center() const;
		float _radius() const;
		float _inner_radius() const;
		float _time_range() const;
		float _time_radius(float p_time) const;
		Tap _tap(int p_index) const;
		Vector2 _tap_position(int p_index) const;
		int _hit_tap(const Vector2& p_position, bool p_alternate = false) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_geometry();
		void _edit_tap_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectDelayPreview();

		void set_effect(const Ref<FmodAudioEffectDelay>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_DELAY_PREVIEW_H
