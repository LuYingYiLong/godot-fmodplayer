#ifndef FMOD_AUDIO_EFFECT_PITCH_SHIFT_PREVIEW_H
#define FMOD_AUDIO_EFFECT_PITCH_SHIFT_PREVIEW_H

#include "dsp/fmod_audio_effect_pitch_shift.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectPitchShiftPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectPitchShiftPreview, EditorProperty)

		Ref<FmodAudioEffectPitchShift> effect;
		bool dragging = false;
		bool gesture_changed = false;
		float drag_range = 24.0f;

		float _scale() const;
		Vector2 _dial_center() const;
		float _dial_radius() const;
		float _semitones() const;
		float _dial_range() const;
		bool _hit_dial(const Vector2& p_position, bool p_ring_only) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _edit_pitch(float p_semitones, bool p_changing);
		void _edit_pitch_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectPitchShiftPreview();

		void set_effect(const Ref<FmodAudioEffectPitchShift>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_PITCH_SHIFT_PREVIEW_H
