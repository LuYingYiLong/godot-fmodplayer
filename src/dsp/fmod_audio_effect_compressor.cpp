#include "fmod_audio_effect_compressor.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {
	void FmodAudioEffectCompressor::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_threshold", "threshold_db"), &FmodAudioEffectCompressor::set_threshold);
		ClassDB::bind_method(D_METHOD("get_threshold"), &FmodAudioEffectCompressor::get_threshold);

		ClassDB::bind_method(D_METHOD("set_ratio", "ratio"), &FmodAudioEffectCompressor::set_ratio);
		ClassDB::bind_method(D_METHOD("get_ratio"), &FmodAudioEffectCompressor::get_ratio);

		ClassDB::bind_method(D_METHOD("set_gain", "gain_db"), &FmodAudioEffectCompressor::set_gain);
		ClassDB::bind_method(D_METHOD("get_gain"), &FmodAudioEffectCompressor::get_gain);

		ClassDB::bind_method(D_METHOD("set_attack_us", "attack_us"), &FmodAudioEffectCompressor::set_attack_us);
		ClassDB::bind_method(D_METHOD("get_attack_us"), &FmodAudioEffectCompressor::get_attack_us);

		ClassDB::bind_method(D_METHOD("set_release_ms", "release_ms"), &FmodAudioEffectCompressor::set_release_ms);
		ClassDB::bind_method(D_METHOD("get_release_ms"), &FmodAudioEffectCompressor::get_release_ms);

		ClassDB::bind_method(D_METHOD("set_mix", "mix"), &FmodAudioEffectCompressor::set_mix);
		ClassDB::bind_method(D_METHOD("get_mix"), &FmodAudioEffectCompressor::get_mix);

		ClassDB::bind_method(D_METHOD("set_sidechain", "sidechain"), &FmodAudioEffectCompressor::set_sidechain);
		ClassDB::bind_method(D_METHOD("get_sidechain"), &FmodAudioEffectCompressor::get_sidechain);
		
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "threshold_db", PROPERTY_HINT_RANGE, "-60,0,0.1,suffix:dB"), "set_threshold", "get_threshold");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ratio", PROPERTY_HINT_RANGE, "1,48,0.1"), "set_ratio", "get_ratio");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain_db", PROPERTY_HINT_RANGE, "-20,20,0.1,suffix:dB"), "set_gain", "get_gain");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attack_us", PROPERTY_HINT_RANGE, "20,2000,1,suffix:us"), "set_attack_us", "get_attack_us");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "release_ms", PROPERTY_HINT_RANGE, "20,2000,1,suffix:ms"), "set_release_ms", "get_release_ms");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mix", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_mix", "get_mix");
		ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "sidechain", PROPERTY_HINT_ENUM), "set_sidechain", "get_sidechain");
	}

	void FmodAudioEffectCompressor::_validate_property(PropertyInfo& p_property) const {
		if (!Engine::get_singleton()->is_editor_hint()) {
			return;
		}
		if (p_property.name == StringName("sidechain")) {
			String buses;
			for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
				if (i > 0) {
					buses += ",";
				}
				buses += AudioServer::get_singleton()->get_bus_name(i);
			}

			p_property.hint_string = buses;
		}
	}

	FmodAudioEffectCompressor::FmodAudioEffectCompressor() {
		threshold = 0.0f;
		ratio = 4.0f;
		gain = 0.0f;
		attack_us = 20.0f;
		release_ms = 250.0f;
		mix = 1.0f;
	}

	FmodAudioEffectCompressor::~FmodAudioEffectCompressor() {
	}

	void FmodAudioEffectCompressor::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> compressor_dsp = system->create_dsp_by_type(FmodDSP::DSP_TYPE_COMPRESSOR);
		ERR_FAIL_COND_MSG(!compressor_dsp.is_valid(), "Failed to create compressor DSP");

		bus->add_dsp(-1, compressor_dsp);
		dsp_chain.push_back(compressor_dsp);
		_update_dsp_params();

		if (sidechain != StringName()) {
			WARN_PRINT_ONCE("FmodAudioEffectCompressor: sidechain is not supported by the FMOD bus mirror yet.");
		}
	}

	void FmodAudioEffectCompressor::_update_dsp_params() {
		if (dsp_chain.is_empty() || !dsp_chain[0].is_valid()) {
			return;
		}

		Ref<FmodDSP> compressor_dsp = dsp_chain[0];
		compressor_dsp->set_parameter_float(0, CLAMP(threshold, -60.0f, 0.0f));
		compressor_dsp->set_parameter_float(1, CLAMP(ratio, 1.0f, 48.0f));
		compressor_dsp->set_parameter_float(2, CLAMP(attack_us / 1000.0f, 0.1f, 500.0f));
		compressor_dsp->set_parameter_float(3, CLAMP(release_ms, 20.0f, 2000.0f));
		compressor_dsp->set_parameter_float(4, CLAMP(gain, -20.0f, 20.0f));
		compressor_dsp->set_parameter_bool(6, true);
		compressor_dsp->set_wet_dry_mix(1.0f, mix, 1.0f - mix);
	}

	void FmodAudioEffectCompressor::set_threshold(float p_threshold) {
		const float value = CLAMP(p_threshold, -60.0f, 0.0f);
		if (threshold == value) {
			return;
		}
		threshold = value;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectCompressor::get_threshold() const {
		return threshold;
	}

	void FmodAudioEffectCompressor::set_ratio(float p_ratio) {
		const float value = CLAMP(p_ratio, 1.0f, 48.0f);
		if (ratio == value) {
			return;
		}
		ratio = value;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectCompressor::get_ratio() const {
		return ratio;
	}

	void FmodAudioEffectCompressor::set_gain(float p_gain) {
		const float value = CLAMP(p_gain, -20.0f, 20.0f);
		if (gain == value) {
			return;
		}
		gain = value;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectCompressor::get_gain() const {
		return gain;
	}

	void FmodAudioEffectCompressor::set_attack_us(float p_attack_us) {
		attack_us = CLAMP(p_attack_us, 20.0f, 2000.0f);
		_update_dsp_params();
	}

	float FmodAudioEffectCompressor::get_attack_us() const {
		return attack_us;
	}

	void FmodAudioEffectCompressor::set_release_ms(float p_release_ms) {
		release_ms = CLAMP(p_release_ms, 20.0f, 2000.0f);
		_update_dsp_params();
	}

	float FmodAudioEffectCompressor::get_release_ms() const {
		return release_ms;
	}

	void FmodAudioEffectCompressor::set_mix(float p_mix) {
		const float value = CLAMP(p_mix, 0.0f, 1.0f);
		if (mix == value) {
			return;
		}
		mix = value;
		_update_dsp_params();
		emit_changed();
	}

	float FmodAudioEffectCompressor::get_mix() const {
		return mix;
	}

	void FmodAudioEffectCompressor::set_sidechain(const StringName& p_sidechain) {
		sidechain = p_sidechain;
	}

	StringName FmodAudioEffectCompressor::get_sidechain() const {
		return sidechain;
	}
}
