#ifndef FMOD_AUDIO_EFFECT_EQ_PREVIEW_H
#define FMOD_AUDIO_EFFECT_EQ_PREVIEW_H

#include "dsp/fmod_audio_effect_eq.h"
#include "fmod_eq_preview_response.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectEQPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectEQPreview, EditorProperty)

		Ref<FmodAudioEffectEQ> effect;
		int selected_band = -1;
		int hovered_band = -1;
		int band_count = 0;
		bool dragging = false;
		bool gesture_changed = false;
		bool curves_dirty = true;
		float sample_rate = 48000.0f;
		PackedVector2Array total_curve;
		Vector<PackedVector2Array> band_curves;

		float _scale() const;
		Rect2 _plot_rect() const;
		Vector2 _band_position(int p_band) const;
		int _hit_band(const Vector2& p_position) const;
		bool _uses_gain(int p_band) const;
		void _on_effect_changed();
		void _connect_effect();
		void _disconnect_effect();
		void _rebuild_curves();
		void _edit_band_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectEQPreview();
		void set_effect(const Ref<FmodAudioEffectEQ>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_EQ_PREVIEW_H
