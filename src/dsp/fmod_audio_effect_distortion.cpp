#include "dsp/fmod_audio_effect_distortion.h"
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
	static constexpr uint32_t DISTORTION_DSP_USERDATA_MAGIC = 0x44535452;
	static constexpr float DISTORTION_TAU = 6.28318530717958647692f;

	struct FmodAudioEffectDistortionSharedState {
		std::atomic<int> mode{ FmodAudioEffectDistortion::MODE_CLIP };
		std::atomic<float> pre_gain{ 0.0f };
		std::atomic<float> drive{ 0.0f };
		std::atomic<float> post_gain{ 0.0f };
		std::atomic<float> keep_hf_hz{ 16000.0f };
	};

	struct DistortionDSPRuntime {
		std::vector<float> lowpass_history;
		int channels = 0;
	};

	struct DistortionDSPUserData {
		uint32_t magic = DISTORTION_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectDistortionSharedState> distortion_state;
		DistortionDSPRuntime runtime;
	};

	static DistortionDSPUserData* _get_distortion_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}

		DistortionDSPUserData* userdata = static_cast<DistortionDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == DISTORTION_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL distortion_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}

		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}

		DistortionDSPUserData* distortion_userdata = static_cast<DistortionDSPUserData*>(userdata);
		if (!distortion_userdata || distortion_userdata->magic != DISTORTION_DSP_USERDATA_MAGIC) {
			dsp_state->plugindata = nullptr;
			return FMOD_OK;
		}

		dsp_state->plugindata = distortion_userdata;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL distortion_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			DistortionDSPUserData* userdata = static_cast<DistortionDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == DISTORTION_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static float _distortion_process_sample(float sample, int mode, float drive, float atan_mult, float atan_div, float lofi_mult) {
		switch (mode) {
			case FmodAudioEffectDistortion::MODE_CLIP: {
				float sign = sample < 0.0f ? -1.0f : 1.0f;
				sample = std::pow(std::abs(sample), 1.0001f - drive) * sign;
				return CLAMP(sample, -1.0f, 1.0f);
			}
			case FmodAudioEffectDistortion::MODE_ATAN:
				return std::atan(sample * atan_mult) * atan_div;
			case FmodAudioEffectDistortion::MODE_LOFI:
			case FmodAudioEffectDistortion::MODE_BITCRUSH:
				return std::floor(sample * lofi_mult + 0.5f) / lofi_mult;
			case FmodAudioEffectDistortion::MODE_OVERDRIVE: {
				double x = static_cast<double>(sample) * 0.686306;
				double z = 1.0 + std::exp(std::sqrt(std::abs(x)) * -0.75);
				return static_cast<float>((std::exp(x) - std::exp(-x * z)) / (std::exp(x) + std::exp(-x)));
			}
			case FmodAudioEffectDistortion::MODE_WAVESHAPE: {
				float k = 2.0f * drive / (1.00001f - drive);
				return (1.0f + k) * sample / (1.0f + k * std::abs(sample));
			}
		}

		return sample;
	}

	static FMOD_RESULT F_CALL distortion_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {

		if (!inbufferarray || !outbufferarray) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (!inbufferarray->buffer || !outbufferarray->buffer) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int input_channels = inbufferarray->numchannels;
		const int output_channels = outbufferarray->numchannels;
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffer);
		float* out_buffer = static_cast<float*>(outbufferarray->buffer);
		if (!in_buffer || !out_buffer || input_channels <= 0 || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		DistortionDSPUserData* userdata = _get_distortion_userdata(dsp_state);
		FmodAudioEffectDistortionSharedState* state = userdata ? userdata->distortion_state.get() : nullptr;
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

		DistortionDSPRuntime& runtime = userdata->runtime;
		if (runtime.channels != output_channels) {
			runtime.channels = output_channels;
			runtime.lowpass_history.assign(static_cast<size_t>(output_channels), 0.0f);
		}

		const int mode = state->mode.load();
		const float drive = CLAMP(state->drive.load(), 0.0f, 1.0f);
		const float pregain = FmodUtils::db_to_linear(state->pre_gain.load());
		const float postgain = FmodUtils::db_to_linear(state->post_gain.load());
		const float keep_hf_hz = MAX(state->keep_hf_hz.load(), 1.0f);
		const float lpf_c = std::exp(-DISTORTION_TAU * keep_hf_hz / static_cast<float>(sample_rate));
		const float lpf_ic = 1.0f - lpf_c;
		const float atan_mult = std::pow(10.0f, drive * drive * 3.0f) - 1.0f + 0.001f;
		const float atan_div = 1.0f / (std::atan(atan_mult) * (1.0f + drive * 8.0f));
		const float lofi_mult = std::pow(2.0f, 2.0f + (1.0f - drive) * 14.0f);

		for (unsigned int frame = 0; frame < length; frame++) {
			for (int channel = 0; channel < output_channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				const float source = in_buffer[frame * input_channels + input_channel];
				float low = source * lpf_ic + lpf_c * runtime.lowpass_history[channel];
				if (std::abs(low) < 1e-20f) {
					low = 0.0f;
				}
				runtime.lowpass_history[channel] = low;
				const float high = source - low;

				float shaped = low * pregain;
				shaped = _distortion_process_sample(shaped, mode, drive, atan_mult, atan_div, lofi_mult);
				out_buffer[frame * output_channels + channel] = shaped * postgain + high;
			}
		}

		return FMOD_OK;
	}

	void FmodAudioEffectDistortion::_bind_methods() {
		BIND_ENUM_CONSTANT(MODE_CLIP);
		BIND_ENUM_CONSTANT(MODE_ATAN);
		BIND_ENUM_CONSTANT(MODE_LOFI);
		BIND_ENUM_CONSTANT(MODE_OVERDRIVE);
		BIND_ENUM_CONSTANT(MODE_WAVESHAPE);
		BIND_ENUM_CONSTANT(MODE_BITCRUSH);

		ClassDB::bind_method(D_METHOD("set_mode", "mode"), &FmodAudioEffectDistortion::set_mode);
		ClassDB::bind_method(D_METHOD("get_mode"), &FmodAudioEffectDistortion::get_mode);

		ClassDB::bind_method(D_METHOD("set_pre_gain", "gain"), &FmodAudioEffectDistortion::set_pre_gain);
		ClassDB::bind_method(D_METHOD("get_pre_gain"), &FmodAudioEffectDistortion::get_pre_gain);

		ClassDB::bind_method(D_METHOD("set_drive", "drive"), &FmodAudioEffectDistortion::set_drive);
		ClassDB::bind_method(D_METHOD("get_drive"), &FmodAudioEffectDistortion::get_drive);

		ClassDB::bind_method(D_METHOD("set_post_gain", "gain"), &FmodAudioEffectDistortion::set_post_gain);
		ClassDB::bind_method(D_METHOD("get_post_gain"), &FmodAudioEffectDistortion::get_post_gain);

		ClassDB::bind_method(D_METHOD("set_keep_hf_hz", "hz"), &FmodAudioEffectDistortion::set_keep_hf_hz);
		ClassDB::bind_method(D_METHOD("get_keep_hf_hz"), &FmodAudioEffectDistortion::get_keep_hf_hz);

		ClassDB::bind_method(D_METHOD("set_oversample", "oversample"), &FmodAudioEffectDistortion::set_oversample);
		ClassDB::bind_method(D_METHOD("get_oversample"), &FmodAudioEffectDistortion::get_oversample);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Clip,ATan,LoFi,Overdrive,Wave Shape,BitCrush"), "set_mode", "get_mode");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pre_gain", PROPERTY_HINT_RANGE, "-60,60,0.01,suffix:dB"), "set_pre_gain", "get_pre_gain");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drive", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_drive", "get_drive");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "post_gain", PROPERTY_HINT_RANGE, "-80,24,0.01,suffix:dB"), "set_post_gain", "get_post_gain");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "keep_hf_hz", PROPERTY_HINT_RANGE, "1,20500,1,suffix:Hz"), "set_keep_hf_hz", "get_keep_hf_hz");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "oversample", PROPERTY_HINT_RANGE, "1,16,1", PROPERTY_USAGE_NO_EDITOR), "set_oversample", "get_oversample");
	}

	FmodAudioEffectDistortion::FmodAudioEffectDistortion() {
		distortion_state = std::make_shared<FmodAudioEffectDistortionSharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectDistortion::~FmodAudioEffectDistortion() {
		remove_from_bus(bus);
		distortion_state.reset();
	}

	void FmodAudioEffectDistortion::_sync_shared_state() {
		if (!distortion_state) {
			return;
		}

		distortion_state->mode.store(mode);
		distortion_state->pre_gain.store(pre_gain);
		distortion_state->drive.store(drive);
		distortion_state->post_gain.store(post_gain);
		distortion_state->keep_hf_hz.store(keep_hf_hz);
	}

	void FmodAudioEffectDistortion::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> dsp = create_custom_dsp(system);
		if (!dsp.is_valid()) {
			ERR_PRINT("FmodAudioEffectDistortion: Failed to create custom DSP");
			return;
		}

		bus->add_dsp(-1, dsp);
		dsp_chain.push_back(dsp);
	}

	Ref<FmodDSP> FmodAudioEffectDistortion::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());

		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, get_dsp_description(), sizeof(FMOD_DSP_DESCRIPTION));
		if (!distortion_state) {
			distortion_state = std::make_shared<FmodAudioEffectDistortionSharedState>();
			_sync_shared_state();
		}

		DistortionDSPUserData* dsp_userdata = new DistortionDSPUserData();
		dsp_userdata->distortion_state = distortion_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectDistortion::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;

		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotDistortionDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = distortion_dsp_create_callback;
			desc.release = distortion_dsp_release_callback;
			desc.process = distortion_dsp_process_callback;
			initialized = true;
		}

		return &desc;
	}

	void FmodAudioEffectDistortion::set_mode(Mode p_mode) {
		mode = p_mode;
		_sync_shared_state();
	}

	FmodAudioEffectDistortion::Mode FmodAudioEffectDistortion::get_mode() const {
		return mode;
	}

	void FmodAudioEffectDistortion::set_pre_gain(float p_gain) {
		pre_gain = CLAMP(p_gain, -60.0f, 60.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectDistortion::get_pre_gain() const {
		return pre_gain;
	}

	void FmodAudioEffectDistortion::set_drive(float p_drive) {
		drive = CLAMP(p_drive, 0.0f, 1.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectDistortion::get_drive() const {
		return drive;
	}

	void FmodAudioEffectDistortion::set_post_gain(float p_gain) {
		post_gain = CLAMP(p_gain, -80.0f, 24.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectDistortion::get_post_gain() const {
		return post_gain;
	}

	void FmodAudioEffectDistortion::set_keep_hf_hz(float p_hz) {
		keep_hf_hz = CLAMP(p_hz, 1.0f, 20500.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectDistortion::get_keep_hf_hz() const {
		return keep_hf_hz;
	}

	void FmodAudioEffectDistortion::set_oversample(int p_oversample) {
		oversample = CLAMP(p_oversample, 1, 16);
	}

	int FmodAudioEffectDistortion::get_oversample() const {
		return oversample;
	}
} // namespace godot
