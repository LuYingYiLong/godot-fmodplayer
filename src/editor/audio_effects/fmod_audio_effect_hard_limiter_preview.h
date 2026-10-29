#ifndef FMOD_AUDIO_EFFECT_HARD_LIMITER_PREVIEW_H
#define FMOD_AUDIO_EFFECT_HARD_LIMITER_PREVIEW_H

#include "dsp/fmod_audio_effect_hard_limiter.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectHardLimiterPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectHardLimiterPreview, EditorProperty)
		Ref<FmodAudioEffectHardLimiter> effect;
		PackedVector2Array curve;
		bool curve_dirty = true;
		bool gesture_changed = false;
		int hovered_handle = -1;
		int dragged_handle = -1;
		float drag_start_y = 0.0f;
		float drag_start_value = 0.0f;

		float _scale() const;
		Rect2 _plot_rect() const;
		Vector2 _point(float p_input, float p_output) const;
		Vector2 _handle_position(int p_handle) const;
		int _hit_handle(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_curve();
		void _edit_at(float p_y, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectHardLimiterPreview();
		void set_effect(const Ref<FmodAudioEffectHardLimiter>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif
