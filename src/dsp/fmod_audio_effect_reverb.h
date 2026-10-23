#ifndef FMOD_AUDIO_EFFECT_REVERB_H
#define FMOD_AUDIO_EFFECT_REVERB_H

#include "dsp/fmod_audio_effect.h"
#include <atomic>
#include <memory>

namespace godot {
	struct FmodAudioEffectReverbSharedState;

	class FmodAudioEffectReverb : public FmodAudioEffect {
		GDCLASS(FmodAudioEffectReverb, FmodAudioEffect)

	protected:
		static void _bind_methods();

	private:
		std::atomic<float> damping{ 0.5f };
		std::atomic<float> dry{ 1.0f };
		std::atomic<float> hipass{ 0.0f };
		std::atomic<float> predelay_feedback{ 0.4f };
		std::atomic<float> predelay_msec{ 150.0f };
		std::atomic<float> room_size{ 0.8f };
		std::atomic<float> spread{ 1.0f };
		std::atomic<float> wet{ 0.5f };
		std::shared_ptr<FmodAudioEffectReverbSharedState> reverb_state;

	public:
		FmodAudioEffectReverb();
		virtual ~FmodAudioEffectReverb();

		virtual void apply_to(Ref<FmodChannelGroup> p_bus) override;
		virtual Ref<FmodDSP> create_custom_dsp(Ref<FmodSystem> system) override;
		virtual FMOD_DSP_DESCRIPTION* get_dsp_description() override;
		virtual void _on_dsp_create(FMOD_DSP_STATE* dsp_state) override;
		virtual void _on_dsp_process(FMOD_DSP_STATE* dsp_state,
			unsigned int length,
			const FMOD_DSP_BUFFER* inbufferarray,
			const FMOD_DSP_BUFFER* outbufferarray) override;
		virtual void _on_dsp_release(FMOD_DSP_STATE* dsp_state) override;

		void set_damping(float p_damping);
		float get_damping() const;

		void set_dry(float p_dry);
		float get_dry() const;

		void set_hpf(float p_hpf);
		float get_hpf() const;

		void set_predelay_feedback(float p_predelay_feedback);
		float get_predelay_feedback() const;

		void set_predelay_msec(float p_predelay_msec);
		float get_predelay_msec() const;

		void set_room_size(float p_room_size);
		float get_room_size() const;

		void set_spread(float p_spread);
		float get_spread() const;

		void set_wet(float p_wet);
		float get_wet() const;
	};
}

#endif
