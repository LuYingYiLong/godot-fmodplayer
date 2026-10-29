#ifndef FMOD_AUDIO_EFFECT_PANNER_PREVIEW_H
#define FMOD_AUDIO_EFFECT_PANNER_PREVIEW_H

#include "dsp/fmod_audio_effect_panner.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectPannerPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectPannerPreview, EditorProperty)

		Ref<FmodAudioEffectPanner> effect;
		bool dragging = false;
		bool gesture_changed = false;
		
		void _on_effect_changed();
		void _connect_effect();
		void _disconnect_effect();
		float _get_plot_inset() const;
		void _edit_pan_at(float p_x, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectPannerPreview();
		void set_effect(const Ref<FmodAudioEffectPanner>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_PANNER_PREVIEW_H
