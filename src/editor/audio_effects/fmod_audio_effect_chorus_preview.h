#ifndef FMOD_AUDIO_EFFECT_CHORUS_PREVIEW_H
#define FMOD_AUDIO_EFFECT_CHORUS_PREVIEW_H

#include "dsp/fmod_audio_effect_chorus.h"
#include <godot_cpp/classes/editor_property.hpp>

namespace godot {
	class FmodAudioEffectChorusPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectChorusPreview, EditorProperty)

		Ref<FmodAudioEffectChorus> effect;
		PackedVector2Array curves[FmodAudioEffectChorus::MAX_VOICES];
		bool curves_dirty = true;

		float _scale() const;
		Rect2 _plot_rect() const;
		Color _voice_color(int p_voice) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_curves();

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectChorusPreview();

		void set_effect(const Ref<FmodAudioEffectChorus>& p_effect);
		void _draw() override;
		void _update_property() override;
	};
}

#endif // FMOD_AUDIO_EFFECT_CHORUS_PREVIEW_H
