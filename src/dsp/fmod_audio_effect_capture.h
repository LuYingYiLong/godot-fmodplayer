#ifndef FMOD_AUDIO_EFFECT_CAPTURE_H
#define FMOD_AUDIO_EFFECT_CAPTURE_H

#include "dsp/fmod_audio_effect.h"
#include "dsp/fmod_capture_ring_buffer.h"
#include <atomic>
#include <mutex>

namespace godot {
	struct CaptureDSPState {
		AudioRingBuffer* ring_buffer = nullptr;
		std::mutex* buffer_mutex = nullptr;
		std::atomic<int64_t>* pushed_frames = nullptr;
		std::atomic<int64_t>* discarded_frames = nullptr;
		int channels = 2;
	};

	class FmodAudioEffectCapture : public FmodAudioEffect {
		GDCLASS(FmodAudioEffectCapture, FmodAudioEffect)

	protected:
		static void _bind_methods();

		virtual void _on_dsp_create(FMOD_DSP_STATE* dsp_state) override;
		virtual void _on_dsp_process(FMOD_DSP_STATE* dsp_state,
			unsigned int length,
			const FMOD_DSP_BUFFER* inbufferarray,
			const FMOD_DSP_BUFFER* outbufferarray) override;
		virtual void _on_dsp_release(FMOD_DSP_STATE* dsp_state) override;

	private:
		double buffer_length = 1.0;
		int buffer_size = 0;
		AudioRingBuffer ring_buffer;
		mutable std::mutex buffer_mutex;
		std::atomic<int64_t> pushed_frames{ 0 };
		std::atomic<int64_t> discarded_frames{ 0 };
		bool dsp_created = false;

		void _update_buffer_size();

	public:
		FmodAudioEffectCapture();
		virtual ~FmodAudioEffectCapture();

		virtual void apply_to(Ref<FmodChannelGroup> p_bus) override;
		virtual void remove_from_bus(Ref<FmodChannelGroup> p_bus) override;
		virtual Ref<FmodDSP> create_custom_dsp(Ref<FmodSystem> system) override;
		virtual FMOD_DSP_DESCRIPTION* get_dsp_description() override;

		bool can_get_buffer(const int64_t frames) const;
		void clear_buffer();
		PackedVector2Array get_buffer(const int64_t frames);
		int64_t get_discarded_frames() const;
		int64_t get_frames_available() const;
		int64_t get_pushed_frames() const;

		void set_buffer_length(double p_buffer_length);
		double get_buffer_length() const;
	};
}

#endif // !FMOD_AUDIO_EFFECT_CAPTURE_H
