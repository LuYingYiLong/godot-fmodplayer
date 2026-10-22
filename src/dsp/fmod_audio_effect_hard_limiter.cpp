#include "dsp/fmod_audio_effect_hard_limiter.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"

#include <fmod_errors.h>
#include <godot_cpp/core/class_db.hpp>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

namespace godot {
	static constexpr uint32_t HARD_LIMITER_DSP_USERDATA_MAGIC = 0x484C494D;
	static constexpr float HARD_LIMITER_ATTACK = 0.002f;
	static constexpr float HARD_LIMITER_SUSTAIN = 0.02f;

	struct FmodAudioEffectHardLimiterSharedState {
		std::atomic<float> ceiling_db{ -0.3f };
		std::atomic<float> pre_gain_db{ 0.0f };
		std::atomic<float> release{ 0.1f };
	};

	struct HardLimiterDSPRuntime {
		std::vector<float> sample_buffer;
		std::vector<float> gain_buckets;
		int sample_cursor = 0;
		int gain_bucket_cursor = 0;
		int gain_bucket_size = 1;
		int gain_samples_to_store = 1;
		float release_factor = 0.0f;
		float attack_factor = 0.0f;
		float gain = 1.0f;
		float gain_target = 1.0f;
		int channels = 0;
		int sample_rate = 0;
	};

	struct HardLimiterDSPUserData {
		uint32_t magic = HARD_LIMITER_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectHardLimiterSharedState> limiter_state;
		HardLimiterDSPRuntime runtime;
	};

	static float _lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	static void _hard_limiter_ensure_runtime(HardLimiterDSPRuntime& runtime, int channels, int sample_rate) {
		channels = MAX(channels, 1);
		sample_rate = MAX(sample_rate, 1);
		if (runtime.channels == channels && runtime.sample_rate == sample_rate) {
			return;
		}

		runtime.channels = channels;
		runtime.sample_rate = sample_rate;
		runtime.sample_cursor = 0;
		runtime.gain_bucket_cursor = 0;
		runtime.release_factor = 0.0f;
		runtime.attack_factor = 0.0f;
		runtime.gain = 1.0f;
		runtime.gain_target = 1.0f;

		const int latency_frames = static_cast<int>(std::ceil(sample_rate * HARD_LIMITER_ATTACK)) + 1;
		runtime.sample_buffer.assign(static_cast<size_t>(latency_frames) * static_cast<size_t>(channels), 0.0f);

		runtime.gain_samples_to_store = static_cast<int>(std::ceil(sample_rate * (HARD_LIMITER_ATTACK + HARD_LIMITER_SUSTAIN) + 1.0f));
		runtime.gain_bucket_size = MAX(static_cast<int>(sample_rate * HARD_LIMITER_ATTACK), 1);
		const int bucket_count = MAX((runtime.gain_samples_to_store + runtime.gain_bucket_size - 1) / runtime.gain_bucket_size, 1);
		runtime.gain_buckets.assign(static_cast<size_t>(bucket_count), 1.0f);
	}

	static HardLimiterDSPUserData* _get_hard_limiter_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}
		HardLimiterDSPUserData* userdata = static_cast<HardLimiterDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == HARD_LIMITER_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL hard_limiter_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}
		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}
		HardLimiterDSPUserData* limiter_userdata = static_cast<HardLimiterDSPUserData*>(userdata);
		dsp_state->plugindata = (limiter_userdata && limiter_userdata->magic == HARD_LIMITER_DSP_USERDATA_MAGIC) ? limiter_userdata : nullptr;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL hard_limiter_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			HardLimiterDSPUserData* userdata = static_cast<HardLimiterDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == HARD_LIMITER_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL hard_limiter_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FmodDSPBufferArray* inbufferarray,
		FmodDSPBufferArray* outbufferarray,
		FMOD_BOOL inputsidle,
		FmodDSPProcessOperation op) {

		static_cast<void>(inputsidle);

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

		const int input_channels = inbufferarray->buffernumchannels[0];
		const int output_channels = outbufferarray->buffernumchannels[0];
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffers[0]);
		float* out_buffer = static_cast<float*>(outbufferarray->buffers[0]);
		if (!in_buffer || !out_buffer || input_channels <= 0 || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		HardLimiterDSPUserData* userdata = _get_hard_limiter_userdata(dsp_state);
		FmodAudioEffectHardLimiterSharedState* state = userdata ? userdata->limiter_state.get() : nullptr;
		if (!state) {
			for (unsigned int frame = 0; frame < length; frame++) {
				for (int channel = 0; channel < output_channels; channel++) {
					const int input_channel = MIN(channel, input_channels - 1);
					out_buffer[frame * output_channels + channel] = in_buffer[frame * input_channels + input_channel];
				}
			}
			return FMOD_OK;
		}

		int sample_rate = 48000;
		if (dsp_state && dsp_state->functions && dsp_state->functions->getsamplerate) {
			int current_rate = 0;
			if (dsp_state->functions->getsamplerate(dsp_state, &current_rate) == FMOD_OK && current_rate > 0) {
				sample_rate = current_rate;
			}
		}

		HardLimiterDSPRuntime& runtime = userdata->runtime;
		_hard_limiter_ensure_runtime(runtime, output_channels, sample_rate);

		const float ceiling = FmodUtils::db_to_linear(state->ceiling_db.load());
		const float pre_gain = FmodUtils::db_to_linear(state->pre_gain_db.load());
		const float release = MAX(state->release.load(), 0.01f);

		for (unsigned int frame = 0; frame < length; frame++) {
			float largest_sample = 0.0f;
			for (int channel = 0; channel < output_channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				const float sample = in_buffer[frame * input_channels + input_channel] * pre_gain;
				largest_sample = MAX(largest_sample, std::abs(sample));
			}

			runtime.release_factor = MAX(0.0f, runtime.release_factor - 1.0f / static_cast<float>(sample_rate));
			runtime.release_factor = MIN(runtime.release_factor, release);

			if (runtime.release_factor > 0.0f) {
				runtime.gain = _lerp(runtime.gain_target, 1.0f, 1.0f - runtime.release_factor / release);
			}

			if (largest_sample > 0.0f && largest_sample * runtime.gain > ceiling) {
				runtime.gain_target = ceiling / largest_sample;
				runtime.release_factor = release;
				runtime.attack_factor = HARD_LIMITER_ATTACK;
			}

			runtime.attack_factor = MAX(0.0f, runtime.attack_factor - 1.0f / static_cast<float>(sample_rate));
			if (runtime.attack_factor > 0.0f) {
				runtime.gain = _lerp(runtime.gain_target, runtime.gain, 1.0f - runtime.attack_factor / HARD_LIMITER_ATTACK);
			}

			const int bucket_id = runtime.gain_bucket_cursor / runtime.gain_bucket_size;
			if (runtime.gain_bucket_cursor % runtime.gain_bucket_size == 0) {
				runtime.gain_buckets[bucket_id] = 1.0f;
			}
			runtime.gain_buckets[bucket_id] = MIN(runtime.gain_buckets[bucket_id], runtime.gain);
			runtime.gain_bucket_cursor = (runtime.gain_bucket_cursor + 1) % runtime.gain_samples_to_store;

			for (float bucket_gain : runtime.gain_buckets) {
				runtime.gain = MIN(runtime.gain, bucket_gain);
			}

			const int delay_frame = runtime.sample_cursor;
			for (int channel = 0; channel < output_channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				const size_t index = static_cast<size_t>(delay_frame) * output_channels + channel;
				const float delayed = runtime.sample_buffer[index];
				runtime.sample_buffer[index] = in_buffer[frame * input_channels + input_channel] * pre_gain;
				out_buffer[frame * output_channels + channel] = delayed * runtime.gain;
			}

			runtime.sample_cursor = (runtime.sample_cursor + 1) % static_cast<int>(runtime.sample_buffer.size() / output_channels);
		}

		return FMOD_OK;
	}

	void FmodAudioEffectHardLimiter::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_ceiling_db", "ceiling_db"), &FmodAudioEffectHardLimiter::set_ceiling_db);
		ClassDB::bind_method(D_METHOD("get_ceiling_db"), &FmodAudioEffectHardLimiter::get_ceiling_db);
		ClassDB::bind_method(D_METHOD("set_pre_gain_db", "pre_gain_db"), &FmodAudioEffectHardLimiter::set_pre_gain_db);
		ClassDB::bind_method(D_METHOD("get_pre_gain_db"), &FmodAudioEffectHardLimiter::get_pre_gain_db);
		ClassDB::bind_method(D_METHOD("set_release", "release"), &FmodAudioEffectHardLimiter::set_release);
		ClassDB::bind_method(D_METHOD("get_release"), &FmodAudioEffectHardLimiter::get_release);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ceiling_db", PROPERTY_HINT_RANGE, "-24,0,0.01,suffix:dB"), "set_ceiling_db", "get_ceiling_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pre_gain_db", PROPERTY_HINT_RANGE, "-24,24,0.01,suffix:dB"), "set_pre_gain_db", "get_pre_gain_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "release", PROPERTY_HINT_RANGE, "0.01,3,0.01"), "set_release", "get_release");
	}

	FmodAudioEffectHardLimiter::FmodAudioEffectHardLimiter() {
		limiter_state = std::make_shared<FmodAudioEffectHardLimiterSharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectHardLimiter::~FmodAudioEffectHardLimiter() {
		remove_from_bus(bus);
		limiter_state.reset();
	}

	void FmodAudioEffectHardLimiter::_sync_shared_state() {
		if (!limiter_state) {
			return;
		}
		limiter_state->ceiling_db.store(ceiling_db);
		limiter_state->pre_gain_db.store(pre_gain_db);
		limiter_state->release.store(release);
	}

	void FmodAudioEffectHardLimiter::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");
		if (bus.is_valid()) {
			remove_from_bus(bus);
		}
		bus = p_bus;
		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");
		Ref<FmodDSP> dsp = create_custom_dsp(system);
		if (!dsp.is_valid()) {
			return;
		}
		bus->add_dsp(-1, dsp);
		dsp_chain.push_back(dsp);
	}

	Ref<FmodDSP> FmodAudioEffectHardLimiter::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());
		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());
		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, get_dsp_description(), sizeof(FMOD_DSP_DESCRIPTION));
		HardLimiterDSPUserData* dsp_userdata = new HardLimiterDSPUserData();
		dsp_userdata->limiter_state = limiter_state;
		desc.userdata = dsp_userdata;
		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectHardLimiter::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;
		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotHardLimiterDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = hard_limiter_dsp_create_callback;
			desc.release = hard_limiter_dsp_release_callback;
			desc.process = fmod_dsp_process_callback<hard_limiter_dsp_process_callback>();
			initialized = true;
		}
		return &desc;
	}

	void FmodAudioEffectHardLimiter::set_ceiling_db(float p_ceiling_db) {
		ceiling_db = CLAMP(p_ceiling_db, -24.0f, 0.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectHardLimiter::get_ceiling_db() const {
		return ceiling_db;
	}

	void FmodAudioEffectHardLimiter::set_pre_gain_db(float p_pre_gain_db) {
		pre_gain_db = CLAMP(p_pre_gain_db, -24.0f, 24.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectHardLimiter::get_pre_gain_db() const {
		return pre_gain_db;
	}

	void FmodAudioEffectHardLimiter::set_release(float p_release) {
		release = CLAMP(p_release, 0.01f, 3.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectHardLimiter::get_release() const {
		return release;
	}
} // namespace godot
