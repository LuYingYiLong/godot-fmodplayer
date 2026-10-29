#include "dsp/fmod_audio_effect_amplify.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"

namespace godot {
	void FmodAudioEffectAmplify::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_volume_db", "volume_db"), &FmodAudioEffectAmplify::set_volume_db);
		ClassDB::bind_method(D_METHOD("get_volume_db"), &FmodAudioEffectAmplify::get_volume_db);
		ClassDB::bind_method(D_METHOD("set_volume_linear", "volume_linear"), &FmodAudioEffectAmplify::set_volume_linear);
		ClassDB::bind_method(D_METHOD("get_volume_linear"), &FmodAudioEffectAmplify::get_volume_linear);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.01,suffix:dB"), "set_volume_db", "get_volume_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_linear", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_volume_linear", "get_volume_linear");
	}

	FmodAudioEffectAmplify::FmodAudioEffectAmplify() {
	}

	FmodAudioEffectAmplify::~FmodAudioEffectAmplify() {
	}

	void FmodAudioEffectAmplify::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> fader = system->create_dsp_by_type(FmodDSP::DSP_TYPE_FADER);
		if (!fader.is_valid()) {
			return;
		}

		// FMOD Fader gain is linear, while Godot Amplify stores dB.
		fader->set_parameter_float(0, FmodUtils::db_to_linear(volume_db));
		bus->add_dsp(-1, fader);
		dsp_chain.push_back(fader);
	}

	void FmodAudioEffectAmplify::set_volume_db(float p_volume_db) {
		const float value = CLAMP(p_volume_db, -80.0f, 24.0f);
		if (volume_db == value) return;
		volume_db = value;
		_update_dsp_gains();
		emit_changed();
	}

	float FmodAudioEffectAmplify::get_volume_db() const {
		return volume_db;
	}

	void FmodAudioEffectAmplify::set_volume_linear(float p_volume_linear) {
		set_volume_db(FmodUtils::linear_to_db(p_volume_linear));
	}

	float FmodAudioEffectAmplify::get_volume_linear() const {
		return FmodUtils::db_to_linear(volume_db);
	}

	void FmodAudioEffectAmplify::_update_dsp_gains() {
		if (dsp_chain.is_empty() || !dsp_chain[0].is_valid()) {
			return;
		}

		dsp_chain[0]->set_parameter_float(0, FmodUtils::db_to_linear(volume_db));
	}
}
