#ifndef FMOD_AUDIO_EFFECT_AMPLIFY_PREVIEW_H
#define FMOD_AUDIO_EFFECT_AMPLIFY_PREVIEW_H

#include "dsp/fmod_audio_effect_amplify.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectAmplifyPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectAmplifyPreview, EditorProperty)

		Ref<FmodAudioEffectAmplify> effect;
		bool hovered = false;
		bool dragging = false;
		bool gesture_changed = false;
		Vector2 drag_start_position;
		float drag_start_gain = 0.0f;

		float _scale() const;
		Rect2 _plot_rect() const;
		Vector2 _gain_position(float p_gain) const;
		bool _hit_handle(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _update_cursor();
		void _edit_gain_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectAmplifyPreview();

		void set_effect(const Ref<FmodAudioEffectAmplify>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_AMPLIFY_PREVIEW_H
