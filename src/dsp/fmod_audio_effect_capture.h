#ifndef FMOD_AUDIO_EFFECT_CAPTURE_H
#define FMOD_AUDIO_EFFECT_CAPTURE_H

#include "dsp/fmod_audio_effect.h"
#include <atomic>
#include <mutex>
#include <vector>

namespace godot {
	class AudioRingBuffer {
	private:
		std::vector<float> buffer;
		size_t write_pos = 0;
		size_t read_pos = 0;
		size_t size = 0;

	public:
		void resize(size_t p_size) {
			if (p_size < 16) {
				p_size = 16;
			}
			buffer.resize(p_size);
			size = p_size;
			write_pos = 0;
			read_pos = 0;
		}

		void clear() {
			write_pos = 0;
			read_pos = 0;
		}

		size_t space_left() const {
			if (size == 0) return 0;
			if (write_pos >= read_pos) {
				return size - (write_pos - read_pos) - 1;
			}
			return (read_pos - write_pos) - 1;
		}

		size_t data_left() const {
			if (size == 0) return 0;
			if (write_pos >= read_pos) {
				return write_pos - read_pos;
			}
			return size - (read_pos - write_pos);
		}

		size_t write(const float* data, size_t count) {
			if (size == 0 || !data) return 0;
			size_t to_write = MIN(count, space_left());
			for (size_t i = 0; i < to_write; i++) {
				buffer[write_pos] = data[i];
				write_pos = (write_pos + 1) % size;
			}
			return to_write;
		}

		size_t read(float* data, size_t count) {
			if (size == 0 || !data) return 0;
			size_t to_read = MIN(count, data_left());
			for (size_t i = 0; i < to_read; i++) {
				data[i] = buffer[read_pos];
				read_pos = (read_pos + 1) % size;
			}
			return to_read;
		}
	};

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
			const FmodDSPBufferArray* inbufferarray,
			FmodDSPBufferArray* outbufferarray,
			FMOD_BOOL inputsidle,
			FmodDSPProcessOperation op) override;
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
