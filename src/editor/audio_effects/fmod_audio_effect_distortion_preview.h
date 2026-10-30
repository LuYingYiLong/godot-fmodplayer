#ifndef FMOD_AUDIO_EFFECT_DISTORTION_PREVIEW_H
#define FMOD_AUDIO_EFFECT_DISTORTION_PREVIEW_H

#include "dsp/fmod_audio_effect_distortion.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectDistortionPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectDistortionPreview, EditorProperty)
		Ref<FmodAudioEffectDistortion> effect;
		PackedVector2Array curve;
		bool curve_dirty = true;
		bool dragging = false;
		bool gesture_changed = false;
		StringName drag_property;
		float drag_start_y = 0.0f;
		float drag_start_value = 0.0f;
		float output_span = 1.0f;

		float _scale() const;
		Rect2 _plot_rect() const;
		Vector2 _point(float p_input, float p_output) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_curve();
		void _edit_at(float p_y, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectDistortionPreview();

		void set_effect(const Ref<FmodAudioEffectDistortion>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif
