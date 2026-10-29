#ifndef FMOD_AUDIO_EFFECT_STEREO_ENHANCE_PREVIEW_H
#define FMOD_AUDIO_EFFECT_STEREO_ENHANCE_PREVIEW_H

#include "dsp/fmod_audio_effect_stereo_enhance.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectStereoEnhancePreview : public EditorProperty {
		GDCLASS(FmodAudioEffectStereoEnhancePreview, EditorProperty)

		Ref<FmodAudioEffectStereoEnhance> effect;
		PackedVector2Array sector;
		bool geometry_dirty = true;
		bool hovered = false;
		bool dragging = false;
		bool gesture_changed = false;

		float _scale() const;
		Vector2 _radar_center() const;
		float _radar_radius() const;
		float _half_angle() const;
		bool _hit_radar(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_geometry();
		void _edit_width_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectStereoEnhancePreview();
		void set_effect(const Ref<FmodAudioEffectStereoEnhance>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_STEREO_ENHANCE_PREVIEW_H
