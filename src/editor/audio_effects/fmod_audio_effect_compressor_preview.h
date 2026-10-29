#ifndef FMOD_AUDIO_EFFECT_COMPRESSOR_PREVIEW_H
#define FMOD_AUDIO_EFFECT_COMPRESSOR_PREVIEW_H

#include "dsp/fmod_audio_effect_compressor.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/input_event.hpp>

namespace godot {
	class FmodAudioEffectCompressorPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectCompressorPreview, EditorProperty)

		enum Handle {
			HANDLE_NONE = -1,
			HANDLE_THRESHOLD,
			HANDLE_RATIO,
			HANDLE_SHARED,
		};

		Ref<FmodAudioEffectCompressor> effect;
		PackedVector2Array curve;
		bool curve_dirty = true;
		bool gesture_changed = false;
		Handle hovered_handle = HANDLE_NONE;
		Handle dragging_handle = HANDLE_NONE;
		Vector2 drag_start_position;
		float drag_start_threshold = 0.0f;
		float drag_start_ratio = 1.0f;

		float _scale() const;
		Rect2 _plot_rect() const;
		Vector2 _point(float p_input, float p_output) const;
		Vector2 _handle_position(Handle p_handle) const;
		Handle _hit_handle(const Vector2& p_position) const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _update_cursor_and_tooltip();
		void _rebuild_curve();
		void _edit_at(const Vector2& p_position, bool p_changing);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectCompressorPreview();

		void set_effect(const Ref<FmodAudioEffectCompressor>& p_effect);
		void _draw() override;
		void _gui_input(const Ref<InputEvent>& p_event) override;
		void _update_property() override;
		void _set_read_only(bool p_read_only) override;
	};
}

#endif // FMOD_AUDIO_EFFECT_COMPRESSOR_PREVIEW_H
