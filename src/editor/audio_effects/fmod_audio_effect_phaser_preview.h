#ifndef FMOD_AUDIO_EFFECT_PHASER_PREVIEW_H
#define FMOD_AUDIO_EFFECT_PHASER_PREVIEW_H

#include "dsp/fmod_audio_effect_phaser.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <vector>

namespace godot {
	class FmodAudioEffectPhaserPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectPhaserPreview, EditorProperty)

		Ref<FmodAudioEffectPhaser> effect;
		PackedVector2Array gain_curve;
		std::vector<PackedVector2Array> phase_curves;
		bool curve_dirty = true;
		bool gesture_changed = false;
		int hovered_range = -1;
		int dragged_range = -1;
		float drag_start_x = 0.0f;
		float drag_start_frequency = 0.0f;

		float _scale() const;
		Rect2 _plot_rect() const;
		float _frequency_x(float p_frequency) const;
		int _hit_range(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _rebuild_curve();
		void _edit_range_at(float p_x, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectPhaserPreview();

		void set_effect(const Ref<FmodAudioEffectPhaser>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_PHASER_PREVIEW_H
