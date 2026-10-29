#ifndef FMOD_AUDIO_EFFECT_REVERB_PREVIEW_H
#define FMOD_AUDIO_EFFECT_REVERB_PREVIEW_H

#include "dsp/fmod_audio_effect_reverb.h"
#include <godot_cpp/classes/editor_property.hpp>
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

		float _scale() const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_geometry();

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectReverbPreview();
		void set_effect(const Ref<FmodAudioEffectReverb>& p_effect);
		void _draw() override;
		void _update_property() override;
	};
}

#endif // FMOD_AUDIO_EFFECT_REVERB_PREVIEW_H
