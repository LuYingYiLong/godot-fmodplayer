#ifndef FMOD_AUDIO_EFFECT_REVERB_PREVIEW_H
#define FMOD_AUDIO_EFFECT_REVERB_PREVIEW_H

#include "dsp/fmod_audio_effect_reverb.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <vector>

namespace godot {
	class FmodAudioEffectReverbPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectReverbPreview, EditorProperty)

		struct Ring {
			PackedVector2Array back;
			PackedVector2Array front;
			float energy = 1.0f;
		};

		Ref<FmodAudioEffectReverb> effect;
		std::vector<Ring> rings;
		PackedVector2Array source_marker;
		bool geometry_dirty = true;
		int hovered_handle = -1;
		int dragged_handle = -1;
		float drag_start_x = 0.0f;
		float drag_start_spread = 0.0f;
		float drag_max_separation = 0.0f;
		bool gesture_changed = false;

		float _scale() const;
		Rect2 _space_rect() const;
		float _ring_height() const;
		float _ring_radius_x() const;
		float _max_separation() const;
		Vector2 _handle_position(int p_handle) const;
		int _hit_handle(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_geometry();
		void _draw_reflections(bool p_front, float p_brightness);
		void _edit_spread_at(float p_x, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectReverbPreview();

		void set_effect(const Ref<FmodAudioEffectReverb>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_REVERB_PREVIEW_H
