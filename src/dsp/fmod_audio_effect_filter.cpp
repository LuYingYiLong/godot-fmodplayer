#include "dsp/fmod_audio_effect_filter.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"

namespace godot {
	static int _filter_db_to_fmod_lowpass(FmodAudioEffectFilter::FilterDB p_db) {
		switch (p_db) {
			case FmodAudioEffectFilter::FILTER_6DB:
				return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_6DB;
			case FmodAudioEffectFilter::FILTER_12DB:
				return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_12DB;
			case FmodAudioEffectFilter::FILTER_18DB:
				return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_24DB;
			case FmodAudioEffectFilter::FILTER_24DB:
				return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_24DB;
		}

		return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_6DB;
	}

	static float _filter_resonance_to_q(float p_resonance) {
		return MAX(p_resonance, 0.0001f);
	}

	void FmodAudioEffectFilter::_bind_methods() {
		BIND_ENUM_CONSTANT(FILTER_6DB);
		BIND_ENUM_CONSTANT(FILTER_12DB);
		BIND_ENUM_CONSTANT(FILTER_18DB);
		BIND_ENUM_CONSTANT(FILTER_24DB);

		ClassDB::bind_method(D_METHOD("set_cutoff_hz", "cutoff_hz"), &FmodAudioEffectFilter::set_cutoff_hz);
		ClassDB::bind_method(D_METHOD("get_cutoff_hz"), &FmodAudioEffectFilter::get_cutoff_hz);

		ClassDB::bind_method(D_METHOD("set_db", "db"), &FmodAudioEffectFilter::set_db);
		ClassDB::bind_method(D_METHOD("get_db"), &FmodAudioEffectFilter::get_db);

		ClassDB::bind_method(D_METHOD("set_gain", "gain"), &FmodAudioEffectFilter::set_gain);
		ClassDB::bind_method(D_METHOD("get_gain"), &FmodAudioEffectFilter::get_gain);

		ClassDB::bind_method(D_METHOD("set_resonance", "resonance"), &FmodAudioEffectFilter::set_resonance);
		ClassDB::bind_method(D_METHOD("get_resonance"), &FmodAudioEffectFilter::get_resonance);
		
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cutoff_hz", PROPERTY_HINT_RANGE, "1,20500,1,suffix:Hz"), "set_cutoff_hz", "get_cutoff_hz");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "db", PROPERTY_HINT_ENUM, "6 dB,12 dB,18 dB,24 dB"), "set_db", "get_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0,4,0.01"), "set_gain", "get_gain");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "resonance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_resonance", "get_resonance");
	}

	FmodAudioEffectFilter::FmodAudioEffectFilter() {
	}

	FmodAudioEffectFilter::~FmodAudioEffectFilter() {
	}

	void FmodAudioEffectFilter::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> eq_dsp = system->create_dsp_by_type(FmodDSP::DSP_TYPE_MULTIBAND_EQ);
		if (!eq_dsp.is_valid()) {
			return;
		}

		for (int i = 4; i < 20; i += 4) {
			eq_dsp->set_parameter_int(i, FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED);
		}

		bus->add_dsp(-1, eq_dsp);
		dsp_chain.push_back(eq_dsp);
		_update_dsp_params();
	}

	void FmodAudioEffectFilter::set_cutoff_hz(float p_cutoff_hz) {
		const float cutoff = CLAMP(p_cutoff_hz, 1.0f, 20500.0f);
		if (cutoff_hz == cutoff) {
			return;
		}
		cutoff_hz = cutoff;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectFilter::get_cutoff_hz() const {
		return cutoff_hz;
	}

	void FmodAudioEffectFilter::set_db(FilterDB p_db) {
		if (db == p_db) {
			return;
		}
		db = p_db;
		_update_dsp_params();
		emit_changed();
	}

	FmodAudioEffectFilter::FilterDB FmodAudioEffectFilter::get_db() const {
		return db;
	}

	void FmodAudioEffectFilter::set_gain(float p_gain) {
		const float value = CLAMP(p_gain, 0.0f, 4.0f);
		if (gain == value) {
			return;
		}
		gain = value;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectFilter::get_gain() const {
		return gain;
	}

	void FmodAudioEffectFilter::set_resonance(float p_resonance) {
		const float value = CLAMP(p_resonance, 0.0f, 1.0f);
		if (resonance == value) {
			return;
		}
		resonance = value;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectFilter::get_resonance() const {
		return resonance;
	}

	void FmodAudioEffectFilter::_update_dsp_params() {
		if (dsp_chain.is_empty() || !dsp_chain[0].is_valid()) {
			return;
		}

		Ref<FmodDSP> eq_dsp = dsp_chain[0];
		eq_dsp->set_parameter_int(0, _filter_db_to_fmod_lowpass(db));
		eq_dsp->set_parameter_float(1, cutoff_hz);
		eq_dsp->set_parameter_float(2, _filter_resonance_to_q(resonance));
		eq_dsp->set_parameter_float(3, FmodUtils::linear_to_db(MAX(gain, 0.001f)));
	}
}
