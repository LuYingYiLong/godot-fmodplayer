#ifndef FMOD_AUDIO_EFFECT_CAPTURE_PREVIEW_H
#define FMOD_AUDIO_EFFECT_CAPTURE_PREVIEW_H

#include "dsp/fmod_audio_effect_capture.h"
#include "fmod_capture_preview_envelope.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/timer.hpp>

namespace godot {
	class FmodAudioEffectCapturePreview : public EditorProperty {
		GDCLASS(FmodAudioEffectCapturePreview, EditorProperty)

		Ref<FmodAudioEffectCapture> effect;
		Ref<FmodAudioEffectCapture> runtime_effect;
		Timer* refresh_timer = nullptr;
		std::vector<Vector2> snapshot;
		std::vector<fmod_capture_preview::PeakRange> ranges;
		size_t last_capacity = 0;
		size_t last_frames = 0;
		int64_t last_written_frames = -1;
		double last_sample_rate = 0.0;
		double duration = 1.0;
		bool snapshot_dirty = true;

		float _scale() const;
		Rect2 _plot_rect() const;
		void _connect_effect();
		void _disconnect_effect();
		void _on_effect_changed();
		void _update_refresh_timer();
		void _refresh_snapshot();

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodAudioEffectCapturePreview();

		void set_effect(const Ref<FmodAudioEffectCapture>& p_effect);
		void set_runtime_effect(const Ref<FmodAudioEffect>& value) { runtime_effect = value; _on_effect_changed(); }
		void _draw() override;
		void _update_property() override;
	};
}

#endif // FMOD_AUDIO_EFFECT_CAPTURE_PREVIEW_H
