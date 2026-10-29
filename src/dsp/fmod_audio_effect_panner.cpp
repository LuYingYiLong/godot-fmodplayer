#include "dsp/fmod_audio_effect_panner.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"

#include <fmod_errors.h>
#include <godot_cpp/core/class_db.hpp>
#include <atomic>
#include <cstring>

namespace godot {
	static constexpr uint32_t PANNER_DSP_USERDATA_MAGIC = 0x50414E52;

	struct FmodAudioEffectPannerSharedState {
		std::atomic<float> pan{ 0.0f };
	};

	struct PannerDSPUserData {
		uint32_t magic = PANNER_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectPannerSharedState> panner_state;
	};

	static PannerDSPUserData* _get_panner_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}

		PannerDSPUserData* userdata = static_cast<PannerDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == PANNER_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL panner_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}

		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}

		PannerDSPUserData* panner_userdata = static_cast<PannerDSPUserData*>(userdata);
		dsp_state->plugindata = (panner_userdata && panner_userdata->magic == PANNER_DSP_USERDATA_MAGIC) ? panner_userdata : nullptr;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL panner_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			PannerDSPUserData* userdata = static_cast<PannerDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == PANNER_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL panner_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
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

		PannerDSPUserData* userdata = _get_panner_userdata(dsp_state);
		const float pan = userdata && userdata->panner_state ? CLAMP(userdata->panner_state->pan.load(), -1.0f, 1.0f) : 0.0f;
		const float lvol = CLAMP(1.0f - pan, 0.0f, 1.0f);
		const float rvol = CLAMP(1.0f + pan, 0.0f, 1.0f);

		for (unsigned int frame = 0; frame < length; frame++) {
			if (output_channels >= 2) {
				const float left = in_buffer[frame * input_channels];
				const float right = in_buffer[frame * input_channels + MIN(1, input_channels - 1)];
				out_buffer[frame * output_channels] = left * lvol + right * (1.0f - rvol);
				out_buffer[frame * output_channels + 1] = right * rvol + left * (1.0f - lvol);
				for (int channel = 2; channel < output_channels; channel++) {
					const int input_channel = MIN(channel, input_channels - 1);
					out_buffer[frame * output_channels + channel] = in_buffer[frame * input_channels + input_channel];
				}
			} else {
				out_buffer[frame] = in_buffer[frame * input_channels];
			}
		}

		return FMOD_OK;
	}

	void FmodAudioEffectPanner::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_pan", "pan"), &FmodAudioEffectPanner::set_pan);
		ClassDB::bind_method(D_METHOD("get_pan"), &FmodAudioEffectPanner::get_pan);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_pan", "get_pan");
	}

	FmodAudioEffectPanner::FmodAudioEffectPanner() {
		panner_state = std::make_shared<FmodAudioEffectPannerSharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectPanner::~FmodAudioEffectPanner() {
		remove_from_bus(bus);
		panner_state.reset();
	}

	void FmodAudioEffectPanner::_sync_shared_state() {
		if (panner_state) {
			panner_state->pan.store(pan);
		}
	}

	void FmodAudioEffectPanner::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> panner_dsp = create_custom_dsp(system);
		if (!panner_dsp.is_valid()) {
			return;
		}

		bus->add_dsp(-1, panner_dsp);
		dsp_chain.push_back(panner_dsp);
	}

	Ref<FmodDSP> FmodAudioEffectPanner::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());
		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, get_dsp_description(), sizeof(FMOD_DSP_DESCRIPTION));
		PannerDSPUserData* dsp_userdata = new PannerDSPUserData();
		dsp_userdata->panner_state = panner_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectPanner::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;
		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotPannerDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = panner_dsp_create_callback;
			desc.release = panner_dsp_release_callback;
			desc.process = panner_dsp_process_callback;
			initialized = true;
		}
		return &desc;
	}

	void FmodAudioEffectPanner::set_pan(float p_pan) {
		const float new_pan = CLAMP(p_pan, -1.0f, 1.0f);
		if (pan == new_pan) {
			return;
		}
		pan = new_pan;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectPanner::get_pan() const {
		return pan;
	}
} // namespace godot
