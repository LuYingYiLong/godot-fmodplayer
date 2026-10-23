#ifndef FMOD_AUDIO_EFFECT_RECORD_H
#define FMOD_AUDIO_EFFECT_RECORD_H

#include "dsp/fmod_audio_effect.h"
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <memory>

namespace godot {
	struct FmodAudioEffectRecordSharedState;

	class FmodAudioEffectRecord : public FmodAudioEffect {
		GDCLASS(FmodAudioEffectRecord, FmodAudioEffect)

	public:
		enum Format {
			FORMAT_8_BITS = 0,
			FORMAT_16_BITS = 1,
			FORMAT_IMA_ADPCM = 2
		};

	private:
		Format format = FORMAT_16_BITS;
		std::shared_ptr<FmodAudioEffectRecordSharedState> record_state;

		void _init_recording();
		void _finish_recording();
		Ref<AudioStreamWAV> _create_wav_from_buffer() const;

	protected:
		static void _bind_methods();

	public:
		FmodAudioEffectRecord();
		virtual ~FmodAudioEffectRecord();

		virtual void apply_to(Ref<FmodChannelGroup> p_bus) override;

		virtual Ref<FmodDSP> create_custom_dsp(Ref<FmodSystem> system) override;
		virtual FMOD_DSP_DESCRIPTION* get_dsp_description() override;
		virtual void _on_dsp_process(FMOD_DSP_STATE* dsp_state,
			unsigned int length,
			const FMOD_DSP_BUFFER* inbufferarray,
			const FMOD_DSP_BUFFER* outbufferarray) override;

		void set_recording_active(bool p_record);
		bool is_recording_active() const;

		void set_format(Format p_format);
		Format get_format() const;

		Ref<AudioStreamWAV> get_recording() const;
		PackedVector2Array get_waveform_snapshot(int p_width = 512, float p_seconds = 5.0f) const;

		void _add_samples(const float* buffer, unsigned int length, int channels);
	};
}

VARIANT_ENUM_CAST(FmodAudioEffectRecord::Format);

#endif
