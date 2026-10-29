#ifndef FMOD_AUDIO_EFFECT_SPECTRUM_ANALYZER_PREVIEW_H
#define FMOD_AUDIO_EFFECT_SPECTRUM_ANALYZER_PREVIEW_H

#include "dsp/fmod_audio_effect_spectrum_analyzer.h"
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <array>

namespace godot {
	class FmodAudioEffectSpectrumAnalyzerPreview : public EditorProperty {
		GDCLASS(FmodAudioEffectSpectrumAnalyzerPreview, EditorProperty)
		Ref<FmodAudioEffectSpectrumAnalyzer> effect;
		Timer* refresh_timer = nullptr;
		std::array<Vector2, 64> magnitudes{};
		PackedVector2Array left_curve;
		PackedVector2Array right_curve;
		PackedVector2Array fill_lines;
		bool geometry_dirty = true;
		int last_bins = -1;
		float sample_rate = 48000.0f;

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
		FmodAudioEffectSpectrumAnalyzerPreview();
		void set_effect(const Ref<FmodAudioEffectSpectrumAnalyzer>& p_effect);
		void _draw() override;
		void _update_property() override;
	};
}

#endif
