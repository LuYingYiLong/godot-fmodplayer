#ifndef FMOD_AUDIO_EFFECT_FILTER_PREVIEW_H
#define FMOD_AUDIO_EFFECT_FILTER_PREVIEW_H

#include "dsp/fmod_audio_effect_filter.h"
#include "fmod_filter_preview_response.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectFilterPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectFilterPreview, EditorProperty)

		Ref<FmodAudioEffectFilter> effect;
		PackedVector2Array curve;
		bool curve_dirty = true;
		bool hovered = false;
		bool dragging = false;
		bool gesture_changed = false;
		float sample_rate = 48000.0f;

		float _scale() const;
		Rect2 _plot_rect() const;
		Vector2 _handle_position() const;
		bool _hit_handle(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_curve();
		void _edit_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectFilterPreview();

		void set_effect(const Ref<FmodAudioEffectFilter>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_FILTER_PREVIEW_H
