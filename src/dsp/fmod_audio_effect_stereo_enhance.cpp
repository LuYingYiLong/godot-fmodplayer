#include "fmod_audio_effect_stereo_enhance.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"

#include <fmod_errors.h>
#include <godot_cpp/core/class_db.hpp>
#include <atomic>
#include <cstring>
#include <vector>

namespace godot {
	static constexpr uint32_t STEREO_ENHANCE_DSP_USERDATA_MAGIC = 0x53544552;
	static constexpr float STEREO_ENHANCE_MAX_DELAY_MS = 52.0f;

	struct FmodAudioEffectStereoEnhanceSharedState {
		std::atomic<float> pan_pullout{ 1.0f };
		std::atomic<float> time_pullout{ 0.0f };
		std::atomic<float> surround{ 0.0f };
	};

	struct StereoEnhanceDSPRuntime {
		std::vector<float> delay_ring_buffer;
		unsigned int ring_buffer_pos = 0;
		unsigned int ring_buffer_mask = 0;
		int sample_rate = 48000;
	};

	struct StereoEnhanceDSPUserData {
		uint32_t magic = STEREO_ENHANCE_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectStereoEnhanceSharedState> stereo_state;
		StereoEnhanceDSPRuntime runtime;
	};

	static unsigned int _next_power_of_two(unsigned int p_value) {
		if (p_value <= 1) {
			return 1;
		}
		p_value--;
		p_value |= p_value >> 1;
		p_value |= p_value >> 2;
		p_value |= p_value >> 4;
		p_value |= p_value >> 8;
		p_value |= p_value >> 16;
		return p_value + 1;
	}

	static void _stereo_ensure_runtime(StereoEnhanceDSPRuntime& runtime, int sample_rate) {
		sample_rate = MAX(sample_rate, 1);
		const unsigned int desired_frames = _next_power_of_two(static_cast<unsigned int>((STEREO_ENHANCE_MAX_DELAY_MS / 1000.0f) * static_cast<float>(sample_rate)) + 1);
		if (runtime.sample_rate == sample_rate && runtime.ring_buffer_mask + 1 == desired_frames) {
			return;
		}
		runtime.sample_rate = sample_rate;
		runtime.ring_buffer_mask = desired_frames - 1;
		runtime.ring_buffer_pos = 0;
		runtime.delay_ring_buffer.assign(desired_frames, 0.0f);
	}

	static StereoEnhanceDSPUserData* _get_stereo_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}
		StereoEnhanceDSPUserData* userdata = static_cast<StereoEnhanceDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == STEREO_ENHANCE_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL stereo_enhance_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}
		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}
		StereoEnhanceDSPUserData* stereo_userdata = static_cast<StereoEnhanceDSPUserData*>(userdata);
		dsp_state->plugindata = (stereo_userdata && stereo_userdata->magic == STEREO_ENHANCE_DSP_USERDATA_MAGIC) ? stereo_userdata : nullptr;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL stereo_enhance_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			StereoEnhanceDSPUserData* userdata = static_cast<StereoEnhanceDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == STEREO_ENHANCE_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL stereo_enhance_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {

		if (!inbufferarray || !outbufferarray) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int input_channels = inbufferarray->numchannels;
		const int output_channels = outbufferarray->numchannels;
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffer);
		float* out_buffer = static_cast<float*>(outbufferarray->buffer);
		if (!in_buffer || !out_buffer || input_channels <= 0 || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		StereoEnhanceDSPUserData* userdata = _get_stereo_userdata(dsp_state);
		FmodAudioEffectStereoEnhanceSharedState* state = userdata ? userdata->stereo_state.get() : nullptr;
		if (!state || output_channels < 2) {
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
		StereoEnhanceDSPRuntime& runtime = userdata->runtime;
		_stereo_ensure_runtime(runtime, sample_rate);

		const float intensity = state->pan_pullout.load();
		const float surround = CLAMP(state->surround.load(), 0.0f, 1.0f);
		const bool surround_mode = surround > 0.0f;
		unsigned int delay_frames = static_cast<unsigned int>((CLAMP(state->time_pullout.load(), 0.0f, 50.0f) / 1000.0f) * static_cast<float>(sample_rate));
		delay_frames = MIN(delay_frames, runtime.ring_buffer_mask);

		for (unsigned int frame = 0; frame < length; frame++) {
			float left = in_buffer[frame * input_channels];
			float right = in_buffer[frame * input_channels + MIN(1, input_channels - 1)];
			const float center = (left + right) * 0.5f;

			left = center + (left - center) * intensity;
			right = center + (right - center) * intensity;

			if (surround_mode) {
				const float val = (left + right) * 0.5f;
				runtime.delay_ring_buffer[runtime.ring_buffer_pos & runtime.ring_buffer_mask] = val;
				const float delayed = runtime.delay_ring_buffer[(runtime.ring_buffer_pos - delay_frames) & runtime.ring_buffer_mask] * surround;
				left += delayed;
				right -= delayed;
			} else {
				runtime.delay_ring_buffer[runtime.ring_buffer_pos & runtime.ring_buffer_mask] = right;
				right = runtime.delay_ring_buffer[(runtime.ring_buffer_pos - delay_frames) & runtime.ring_buffer_mask];
			}

			out_buffer[frame * output_channels] = left;
			out_buffer[frame * output_channels + 1] = right;
			for (int channel = 2; channel < output_channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				out_buffer[frame * output_channels + channel] = in_buffer[frame * input_channels + input_channel];
			}
			runtime.ring_buffer_pos++;
		}

		return FMOD_OK;
	}

	void FmodAudioEffectStereoEnhance::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_pan_pullout", "amount"), &FmodAudioEffectStereoEnhance::set_pan_pullout);
		ClassDB::bind_method(D_METHOD("get_pan_pullout"), &FmodAudioEffectStereoEnhance::get_pan_pullout);

		ClassDB::bind_method(D_METHOD("set_time_pullout", "amount"), &FmodAudioEffectStereoEnhance::set_time_pullout);
		ClassDB::bind_method(D_METHOD("get_time_pullout"), &FmodAudioEffectStereoEnhance::get_time_pullout);

		ClassDB::bind_method(D_METHOD("set_surround", "amount"), &FmodAudioEffectStereoEnhance::set_surround);
		ClassDB::bind_method(D_METHOD("get_surround"), &FmodAudioEffectStereoEnhance::get_surround);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pan_pullout", PROPERTY_HINT_RANGE, "0,4,0.01"), "set_pan_pullout", "get_pan_pullout");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_pullout_ms", PROPERTY_HINT_RANGE, "0,50,0.01,suffix:ms"), "set_time_pullout", "get_time_pullout");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surround", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_surround", "get_surround");
	}

	FmodAudioEffectStereoEnhance::FmodAudioEffectStereoEnhance() {
		stereo_state = std::make_shared<FmodAudioEffectStereoEnhanceSharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectStereoEnhance::~FmodAudioEffectStereoEnhance() {
		remove_from_bus(bus);
		stereo_state.reset();
	}

	void FmodAudioEffectStereoEnhance::_sync_shared_state() {
		if (!stereo_state) {
			return;
		}
		stereo_state->pan_pullout.store(pan_pullout);
		stereo_state->time_pullout.store(time_pullout);
		stereo_state->surround.store(surround);
	}

	void FmodAudioEffectStereoEnhance::apply_to(Ref<FmodChannelGroup> p_bus) {
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

	Ref<FmodDSP> FmodAudioEffectStereoEnhance::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());
		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, get_dsp_description(), sizeof(FMOD_DSP_DESCRIPTION));
		StereoEnhanceDSPUserData* dsp_userdata = new StereoEnhanceDSPUserData();
		dsp_userdata->stereo_state = stereo_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectStereoEnhance::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;
		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotStereoEnhanceDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = stereo_enhance_dsp_create_callback;
			desc.release = stereo_enhance_dsp_release_callback;
			desc.process = stereo_enhance_dsp_process_callback;
			initialized = true;
		}
		return &desc;
	}

	void FmodAudioEffectStereoEnhance::set_pan_pullout(float p_amount) {
		pan_pullout = CLAMP(p_amount, 0.0f, 4.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectStereoEnhance::get_pan_pullout() const {
		return pan_pullout;
	}

	void FmodAudioEffectStereoEnhance::set_time_pullout(float p_amount) {
		time_pullout = CLAMP(p_amount, 0.0f, 50.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectStereoEnhance::get_time_pullout() const {
		return time_pullout;
	}

	void FmodAudioEffectStereoEnhance::set_surround(float p_amount) {
		surround = CLAMP(p_amount, 0.0f, 1.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectStereoEnhance::get_surround() const {
		return surround;
	}
}
