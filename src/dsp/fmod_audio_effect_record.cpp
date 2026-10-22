#include "dsp/fmod_audio_effect_record.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <atomic>
#include <cstring>
#include <mutex>
#include <new>
#include <vector>

namespace godot {
	static constexpr uint32_t RECORD_DSP_USERDATA_MAGIC = 0x52454344;

	struct FmodAudioEffectRecordSharedState {
		std::atomic<bool> recording_active{ false };
		std::vector<float> recording_buffer;
		mutable std::mutex recording_mutex;
		int buffer_size = 0;
		int max_buffer_size = 0;
		std::atomic<int> sample_rate{ 48000 };
		int recording_channels = 2;
	};

	struct RecordDSPUserData {
		uint32_t magic = RECORD_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectRecordSharedState> record_state;
	};

	static FmodAudioEffectRecordSharedState* _get_record_state_from_dsp(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}

		RecordDSPUserData* userdata = static_cast<RecordDSPUserData*>(dsp_state->plugindata);
		if (userdata->magic != RECORD_DSP_USERDATA_MAGIC) {
			return nullptr;
		}

		return userdata->record_state.get();
	}

	static FMOD_RESULT _record_passthrough(float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels) {
		if (!outbuffer || !outchannels || *outchannels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int output_channels = *outchannels;
		const size_t output_sample_count = static_cast<size_t>(length) * static_cast<size_t>(output_channels);

		if (inbuffer && inchannels > 0) {
			if (inchannels == output_channels) {
				memcpy(outbuffer, inbuffer, output_sample_count * sizeof(float));
			} else {
				for (unsigned int frame = 0; frame < length; frame++) {
					for (int channel = 0; channel < output_channels; channel++) {
						const int input_channel = MIN(channel, inchannels - 1);
						outbuffer[frame * output_channels + channel] = inbuffer[frame * inchannels + input_channel];
					}
				}
			}
		} else {
			memset(outbuffer, 0, output_sample_count * sizeof(float));
		}

		return FMOD_OK;
	}

	static void _record_add_samples(FmodAudioEffectRecordSharedState* p_state, const float* buffer, unsigned int length, int channels) {
		if (!p_state || !buffer || !p_state->recording_active.load()) {
			return;
		}

		std::lock_guard<std::mutex> lock(p_state->recording_mutex);

		if (p_state->recording_buffer.empty() || p_state->buffer_size >= p_state->max_buffer_size) {
			return;
		}

		if (p_state->buffer_size == 0) {
			p_state->recording_channels = channels >= 2 ? 2 : 1;
		}

		if (channels == 1) {
			for (unsigned int i = 0; i < length && p_state->buffer_size < p_state->max_buffer_size; i++) {
				p_state->recording_buffer[static_cast<size_t>(p_state->buffer_size++)] = buffer[i];
			}
		} else if (channels >= 2) {
			for (unsigned int i = 0; i < length && p_state->buffer_size + 1 < p_state->max_buffer_size; i++) {
				p_state->recording_buffer[static_cast<size_t>(p_state->buffer_size++)] = buffer[i * channels];
				p_state->recording_buffer[static_cast<size_t>(p_state->buffer_size++)] = buffer[i * channels + 1];
			}
		}
	}

	static FMOD_RESULT _record_dsp_read(FmodAudioEffectRecordSharedState* p_state, FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels) {
		FMOD_RESULT result = _record_passthrough(inbuffer, outbuffer, length, inchannels, outchannels);
		if (result != FMOD_OK) {
			return result;
		}

		if (p_state && p_state->recording_active.load() && inbuffer && inchannels > 0) {
			if (dsp_state && dsp_state->functions && dsp_state->functions->getsamplerate) {
				int current_rate = 0;
				if (dsp_state->functions->getsamplerate(dsp_state, &current_rate) == FMOD_OK && current_rate > 0) {
					p_state->sample_rate.store(current_rate);
				}
			}
			_record_add_samples(p_state, inbuffer, length, inchannels);
		}

		return FMOD_OK;
	}

	static FMOD_RESULT _record_dsp_process(FmodAudioEffectRecordSharedState* p_state,
		FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FmodDSPBufferArray* inbufferarray,
		FmodDSPBufferArray* outbufferarray,
		FmodDSPProcessOperation op) {

		if (!inbufferarray || !outbufferarray ||
			inbufferarray->numbuffers <= 0 || outbufferarray->numbuffers <= 0 ||
			!inbufferarray->buffernumchannels || !outbufferarray->buffernumchannels ||
			!inbufferarray->bufferchannelmask || !outbufferarray->bufferchannelmask) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (op == FMOD_PLAYER_DSP_PROCESS_QUERY) {
			outbufferarray->buffernumchannels[0] = inbufferarray->buffernumchannels[0];
			outbufferarray->bufferchannelmask[0] = inbufferarray->bufferchannelmask[0];
			outbufferarray->speakermode = inbufferarray->speakermode;
			return FMOD_OK;
		}

		if (op != FMOD_PLAYER_DSP_PROCESS_PERFORM) {
			return FMOD_OK;
		}

		if (!inbufferarray->buffers || !outbufferarray->buffers) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int input_channels = inbufferarray->buffernumchannels[0];
		int output_channels = outbufferarray->buffernumchannels[0];
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffers[0]);
		float* out_buffer = static_cast<float*>(outbufferarray->buffers[0]);

		if (!out_buffer || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (input_channels > 0 && in_buffer) {
			if (input_channels == output_channels) {
				memcpy(out_buffer, in_buffer, static_cast<size_t>(length) * static_cast<size_t>(output_channels) * sizeof(float));
			} else {
				for (unsigned int frame = 0; frame < length; frame++) {
					for (int channel = 0; channel < output_channels; channel++) {
						const int input_channel = MIN(channel, input_channels - 1);
						out_buffer[frame * output_channels + channel] = in_buffer[frame * input_channels + input_channel];
					}
				}
			}
		} else {
			memset(out_buffer, 0, static_cast<size_t>(length) * static_cast<size_t>(output_channels) * sizeof(float));
			return FMOD_OK;
		}

		if (p_state && p_state->recording_active.load()) {
			if (dsp_state && dsp_state->functions && dsp_state->functions->getsamplerate) {
				int current_rate = 0;
				if (dsp_state->functions->getsamplerate(dsp_state, &current_rate) == FMOD_OK && current_rate > 0) {
					p_state->sample_rate.store(current_rate);
				}
			}
			_record_add_samples(p_state, in_buffer, length, input_channels);
		}

		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL record_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}

		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}

		RecordDSPUserData* record_userdata = static_cast<RecordDSPUserData*>(userdata);
		if (!record_userdata || record_userdata->magic != RECORD_DSP_USERDATA_MAGIC) {
			dsp_state->plugindata = nullptr;
			return FMOD_OK;
		}

		dsp_state->plugindata = record_userdata;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL record_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			RecordDSPUserData* userdata = static_cast<RecordDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == RECORD_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL record_dsp_read_callback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels) {
		return _record_dsp_read(_get_record_state_from_dsp(dsp_state), dsp_state, inbuffer, outbuffer, length, inchannels, outchannels);
	}

	static FMOD_RESULT F_CALL record_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FmodDSPBufferArray* inbufferarray,
		FmodDSPBufferArray* outbufferarray,
		FMOD_BOOL inputsidle,
		FmodDSPProcessOperation op) {

		static_cast<void>(inputsidle);
		return _record_dsp_process(_get_record_state_from_dsp(dsp_state), dsp_state, length, inbufferarray, outbufferarray, op);
	}

	void FmodAudioEffectRecord::_bind_methods() {
		BIND_ENUM_CONSTANT(FORMAT_8_BITS);
		BIND_ENUM_CONSTANT(FORMAT_16_BITS);
		BIND_ENUM_CONSTANT(FORMAT_IMA_ADPCM);

		ClassDB::bind_method(D_METHOD("set_recording_active", "record"), &FmodAudioEffectRecord::set_recording_active);
		ClassDB::bind_method(D_METHOD("is_recording_active"), &FmodAudioEffectRecord::is_recording_active);
		ClassDB::bind_method(D_METHOD("set_format", "format"), &FmodAudioEffectRecord::set_format);
		ClassDB::bind_method(D_METHOD("get_format"), &FmodAudioEffectRecord::get_format);
		ClassDB::bind_method(D_METHOD("get_recording"), &FmodAudioEffectRecord::get_recording);
		ClassDB::bind_method(D_METHOD("get_waveform_snapshot", "width", "seconds"), &FmodAudioEffectRecord::get_waveform_snapshot, DEFVAL(512), DEFVAL(5.0f));

		ADD_PROPERTY(PropertyInfo(Variant::INT, "format", PROPERTY_HINT_ENUM, "8-bit,16-bit,IMA ADPCM"), "set_format", "get_format");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "recording_active"), "set_recording_active", "is_recording_active");
	}

	FmodAudioEffectRecord::FmodAudioEffectRecord() {
		record_state = std::make_shared<FmodAudioEffectRecordSharedState>();
	}

	FmodAudioEffectRecord::~FmodAudioEffectRecord() {
		if (record_state) {
			record_state->recording_active.store(false);
		}
		remove_from_bus(bus);
		record_state.reset();
	}

	void FmodAudioEffectRecord::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> record_dsp = create_custom_dsp(system);
		if (!record_dsp.is_valid()) {
			ERR_PRINT("FmodAudioEffectRecord: Failed to create custom DSP");
			return;
		}

		dsp_chain.push_back(record_dsp);
		bus->add_dsp(-1, record_dsp);
	}

	Ref<FmodDSP> FmodAudioEffectRecord::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());

		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION* base_desc = get_dsp_description();
		ERR_FAIL_COND_V(!base_desc, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, base_desc, sizeof(FMOD_DSP_DESCRIPTION));
		if (!record_state) {
			record_state = std::make_shared<FmodAudioEffectRecordSharedState>();
		}

		RecordDSPUserData* dsp_userdata = new RecordDSPUserData();
		dsp_userdata->record_state = record_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectRecord::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;

		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "RecordDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = record_dsp_create_callback;
			desc.release = record_dsp_release_callback;
			desc.process = fmod_dsp_process_callback<record_dsp_process_callback>();
			initialized = true;
		}

		return &desc;
	}

	void FmodAudioEffectRecord::_on_dsp_process(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FmodDSPBufferArray* inbufferarray,
		FmodDSPBufferArray* outbufferarray,
		FMOD_BOOL inputsidle,
		FmodDSPProcessOperation op) {

		if (!inbufferarray || !outbufferarray || inbufferarray->numbuffers <= 0 || outbufferarray->numbuffers <= 0 ||
			!inbufferarray->buffernumchannels || !outbufferarray->buffernumchannels ||
			!inbufferarray->bufferchannelmask || !outbufferarray->bufferchannelmask ||
			!inbufferarray->buffers || !outbufferarray->buffers) {
			return;
		}

		if (op == FMOD_PLAYER_DSP_PROCESS_QUERY) {
			outbufferarray->buffernumchannels[0] = inbufferarray->buffernumchannels[0];
			outbufferarray->bufferchannelmask[0] = inbufferarray->bufferchannelmask[0];
			outbufferarray->speakermode = inbufferarray->speakermode;
			return;
		}

		if (op != FMOD_PLAYER_DSP_PROCESS_PERFORM) {
			return;
		}

		int num_channels = inbufferarray->buffernumchannels[0];
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffers[0]);
		float* out_buffer = static_cast<float*>(outbufferarray->buffers[0]);

		if (num_channels <= 0 || !out_buffer) {
			return;
		}

		size_t sample_count = static_cast<size_t>(length) * static_cast<size_t>(num_channels);
		if (in_buffer) {
			memcpy(out_buffer, in_buffer, sample_count * sizeof(float));
		} else {
			memset(out_buffer, 0, sample_count * sizeof(float));
			return;
		}

		std::shared_ptr<FmodAudioEffectRecordSharedState> state = record_state;
		if (state && state->recording_active.load()) {
			if (dsp_state && dsp_state->functions && dsp_state->functions->getsamplerate) {
				int current_rate = 0;
				if (dsp_state->functions->getsamplerate(dsp_state, &current_rate) == FMOD_OK && current_rate > 0) {
					state->sample_rate.store(current_rate);
				}
			}
			_record_add_samples(state.get(), in_buffer, length, num_channels);
		}
	}

	FMOD_RESULT FmodAudioEffectRecord::_on_dsp_read(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, unsigned int length, int inchannels, int* outchannels) {
		return _record_dsp_read(record_state.get(), dsp_state, inbuffer, outbuffer, length, inchannels, outchannels);
	}

	void FmodAudioEffectRecord::_init_recording() {
		if (!record_state) {
			record_state = std::make_shared<FmodAudioEffectRecordSharedState>();
		}

		std::lock_guard<std::mutex> lock(record_state->recording_mutex);

		record_state->sample_rate.store(48000);
		record_state->recording_channels = 2;
		record_state->max_buffer_size = record_state->sample_rate.load() * 60 * 5 * record_state->recording_channels;
		record_state->buffer_size = 0;
		record_state->recording_buffer.clear();
		record_state->recording_buffer.resize(static_cast<size_t>(record_state->max_buffer_size));
	}

	void FmodAudioEffectRecord::_finish_recording() {
		// Keep the buffer available for get_recording().
	}

	void FmodAudioEffectRecord::_add_samples(const float* buffer, unsigned int length, int channels) {
		_record_add_samples(record_state.get(), buffer, length, channels);
	}

	void FmodAudioEffectRecord::set_recording_active(bool p_record) {
		if (!record_state) {
			record_state = std::make_shared<FmodAudioEffectRecordSharedState>();
		}

		if (record_state->recording_active.load() == p_record) {
			return;
		}

		if (p_record) {
			_init_recording();
			record_state->recording_active.store(true);
		} else {
			record_state->recording_active.store(false);
			_finish_recording();
		}
	}

	bool FmodAudioEffectRecord::is_recording_active() const {
		return record_state && record_state->recording_active.load();
	}

	void FmodAudioEffectRecord::set_format(Format p_format) {
		format = p_format;
	}

	FmodAudioEffectRecord::Format FmodAudioEffectRecord::get_format() const {
		return format;
	}

	Ref<AudioStreamWAV> FmodAudioEffectRecord::get_recording() const {
		return _create_wav_from_buffer();
	}

	PackedVector2Array FmodAudioEffectRecord::get_waveform_snapshot(int p_width, float p_seconds) const {
		PackedVector2Array snapshot;
		if (!record_state || p_width <= 0) {
			return snapshot;
		}

		std::lock_guard<std::mutex> lock(record_state->recording_mutex);
		if (record_state->recording_buffer.empty() || record_state->buffer_size <= 0) {
			return snapshot;
		}

		const int channels = MAX(record_state->recording_channels, 1);
		const int frame_count = record_state->buffer_size / channels;
		if (frame_count <= 0) {
			return snapshot;
		}

		int visible_frames = frame_count;
		const int rate = record_state->sample_rate.load();
		if (p_seconds > 0.0f && rate > 0) {
			visible_frames = MIN(frame_count, MAX(1, static_cast<int>(p_seconds * static_cast<float>(rate))));
		}

		const int start_frame = MAX(0, frame_count - visible_frames);
		snapshot.resize(p_width * 2);
		Vector2* snapshot_write = snapshot.ptrw();

		for (int x = 0; x < p_width; x++) {
			const int bucket_start = start_frame + (visible_frames * x) / p_width;
			int bucket_end = start_frame + (visible_frames * (x + 1)) / p_width;
			bucket_end = MAX(bucket_end, bucket_start + 1);
			bucket_end = MIN(bucket_end, frame_count);

			float min_sample = 0.0f;
			float max_sample = 0.0f;

			for (int frame = bucket_start; frame < bucket_end; frame++) {
				const int sample_index = frame * channels;
				float mixed_sample = 0.0f;

				for (int channel = 0; channel < channels; channel++) {
					mixed_sample += record_state->recording_buffer[static_cast<size_t>(sample_index + channel)];
				}
				mixed_sample /= static_cast<float>(channels);
				mixed_sample = CLAMP(mixed_sample, -1.0f, 1.0f);

				min_sample = MIN(min_sample, mixed_sample);
				max_sample = MAX(max_sample, mixed_sample);
			}

			const float x_position = p_width > 1 ? static_cast<float>(x) / static_cast<float>(p_width - 1) : 0.0f;
			snapshot_write[x * 2] = Vector2(x_position, min_sample);
			snapshot_write[x * 2 + 1] = Vector2(x_position, max_sample);
		}

		return snapshot;
	}

	Ref<AudioStreamWAV> FmodAudioEffectRecord::_create_wav_from_buffer() const {
		std::vector<float> samples;
		int sample_count = 0;
		int channels = 2;
		int rate = 48000;

		{
			if (!record_state) {
				return Ref<AudioStreamWAV>();
			}

			std::lock_guard<std::mutex> lock(record_state->recording_mutex);
			if (record_state->recording_buffer.empty() || record_state->buffer_size <= 0) {
				return Ref<AudioStreamWAV>();
			}

			sample_count = record_state->buffer_size;
			channels = record_state->recording_channels;
			rate = record_state->sample_rate.load();
			samples.assign(record_state->recording_buffer.begin(), record_state->recording_buffer.begin() + sample_count);
		}

		Ref<AudioStreamWAV> wav;
		wav.instantiate();

		wav->set_mix_rate(rate);
		wav->set_loop_mode(AudioStreamWAV::LOOP_DISABLED);
		wav->set_stereo(channels > 1);

		PackedByteArray data;

		switch (format) {
			case FORMAT_8_BITS: {
				wav->set_format(AudioStreamWAV::FORMAT_8_BITS);
				data.resize(sample_count);
				for (int i = 0; i < sample_count; i++) {
					int32_t sample = static_cast<int32_t>(CLAMP(samples[static_cast<size_t>(i)] * 127.0f + 128.0f, 0.0f, 255.0f));
					data[i] = static_cast<uint8_t>(sample);
				}
				break;
			}
			case FORMAT_16_BITS:
			case FORMAT_IMA_ADPCM: {
				// The captured buffer is PCM float data, so expose IMA requests as valid 16-bit WAV data.
				wav->set_format(AudioStreamWAV::FORMAT_16_BITS);
				data.resize(sample_count * 2);
				for (int i = 0; i < sample_count; i++) {
					int16_t sample = static_cast<int16_t>(CLAMP(samples[static_cast<size_t>(i)] * 32767.0f, -32768.0f, 32767.0f));
					data[i * 2] = sample & 0xFF;
					data[i * 2 + 1] = (sample >> 8) & 0xFF;
				}
				break;
			}
		}

		wav->set_data(data);
		return wav;
	}

}
