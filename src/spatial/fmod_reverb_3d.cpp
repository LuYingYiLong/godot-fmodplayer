#include "fmod_reverb_3d.h"
#include <cfloat>
#include <godot_cpp/core/math.hpp>

namespace godot {
	static float _clamp_finite_or(float p_value, float p_fallback, float p_min, float p_max) {
		if (!Math::is_finite(p_value)) {
			return p_fallback;
		}
		return CLAMP(p_value, p_min, p_max);
	}

	static bool _dictionary_get_float(const Dictionary& p_dictionary, const StringName& p_key, float p_min, float p_max, float& r_value) {
		if (!p_dictionary.has(p_key)) {
			return false;
		}

		const Variant value = p_dictionary[p_key];
		const Variant::Type value_type = value.get_type();
		if (value_type != Variant::INT && value_type != Variant::FLOAT) {
			ERR_PRINT(vformat("FmodReverb3D property '%s' expects a numeric value.", String(p_key)));
			return false;
		}

		const float float_value = static_cast<float>(value);
		if (!Math::is_finite(float_value)) {
			ERR_PRINT(vformat("FmodReverb3D property '%s' must be finite.", String(p_key)));
			return false;
		}

		r_value = CLAMP(float_value, p_min, p_max);
		return true;
	}

	void FmodReverb3D::_bind_methods() {
		ClassDB::bind_method(D_METHOD("reverb_3d_is_valid"), &FmodReverb3D::reverb_3d_is_valid);
		ClassDB::bind_method(D_METHOD("reverb_3d_is_null"), &FmodReverb3D::reverb_3d_is_null);

		ClassDB::bind_method(D_METHOD("set_3d_attributes", "position", "min_distance", "max_distance"), &FmodReverb3D::set_3d_attributes);
		ClassDB::bind_method(D_METHOD("get_3d_attributes"), &FmodReverb3D::get_3d_attributes);
		ClassDB::bind_method(D_METHOD("set_properties", "properties"), &FmodReverb3D::set_properties);
		ClassDB::bind_method(D_METHOD("get_properties"), &FmodReverb3D::get_properties);

		ClassDB::bind_method(D_METHOD("set_active", "active"), &FmodReverb3D::set_active);
		ClassDB::bind_method(D_METHOD("get_active"), &FmodReverb3D::get_active);

		ClassDB::bind_method(D_METHOD("release"), &FmodReverb3D::release);

		ClassDB::bind_method(D_METHOD("set_decay_time", "decay_time"), &FmodReverb3D::set_decay_time);
		ClassDB::bind_method(D_METHOD("get_decay_time"), &FmodReverb3D::get_decay_time);

		ClassDB::bind_method(D_METHOD("set_early_delay", "early_delay"), &FmodReverb3D::set_early_delay);
		ClassDB::bind_method(D_METHOD("get_early_delay"), &FmodReverb3D::get_early_delay);

		ClassDB::bind_method(D_METHOD("set_late_delay", "late_delay"), &FmodReverb3D::set_late_delay);
		ClassDB::bind_method(D_METHOD("get_late_delay"), &FmodReverb3D::get_late_delay);

		ClassDB::bind_method(D_METHOD("set_hf_reference", "hf_reference"), &FmodReverb3D::set_hf_reference);
		ClassDB::bind_method(D_METHOD("get_hf_reference"), &FmodReverb3D::get_hf_reference);

		ClassDB::bind_method(D_METHOD("set_hf_decay_ratio", "hf_decay_ratio"), &FmodReverb3D::set_hf_decay_ratio);
		ClassDB::bind_method(D_METHOD("get_hf_decay_ratio"), &FmodReverb3D::get_hf_decay_ratio);

		ClassDB::bind_method(D_METHOD("set_diffusion", "diffusion"), &FmodReverb3D::set_diffusion);
		ClassDB::bind_method(D_METHOD("get_diffusion"), &FmodReverb3D::get_diffusion);

		ClassDB::bind_method(D_METHOD("set_density", "density"), &FmodReverb3D::set_density);
		ClassDB::bind_method(D_METHOD("get_density"), &FmodReverb3D::get_density);

		ClassDB::bind_method(D_METHOD("set_low_shelf_frequency", "low_shelf_frequency"), &FmodReverb3D::set_low_shelf_frequency);
		ClassDB::bind_method(D_METHOD("get_low_shelf_frequency"), &FmodReverb3D::get_low_shelf_frequency);

		ClassDB::bind_method(D_METHOD("set_low_shelf_gain", "low_shelf_gain"), &FmodReverb3D::set_low_shelf_gain);
		ClassDB::bind_method(D_METHOD("get_low_shelf_gain"), &FmodReverb3D::get_low_shelf_gain);

		ClassDB::bind_method(D_METHOD("set_high_cut", "high_cut"), &FmodReverb3D::set_high_cut);
		ClassDB::bind_method(D_METHOD("get_high_cut"), &FmodReverb3D::get_high_cut);

		ClassDB::bind_method(D_METHOD("set_early_late_mix", "early_late_mix"), &FmodReverb3D::set_early_late_mix);
		ClassDB::bind_method(D_METHOD("get_early_late_mix"), &FmodReverb3D::get_early_late_mix);

		ClassDB::bind_method(D_METHOD("set_wet_level", "wet_level"), &FmodReverb3D::set_wet_level);
		ClassDB::bind_method(D_METHOD("get_wet_level"), &FmodReverb3D::get_wet_level);
		
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "get_active");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "decay_time", PROPERTY_HINT_RANGE, "100,20000,0.1,suffix:ms"), "set_decay_time", "get_decay_time");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "early_delay", PROPERTY_HINT_RANGE, "0,300,0.1,suffix:ms"), "set_early_delay", "get_early_delay");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "late_delay", PROPERTY_HINT_RANGE, "0,100,0.1,suffix:ms"), "set_late_delay", "get_late_delay");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hf_reference", PROPERTY_HINT_RANGE, "20,20000,0.1,suffix:Hz"), "set_hf_reference", "get_hf_reference");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hf_decay_ratio", PROPERTY_HINT_RANGE, "10,200,0.1"), "set_hf_decay_ratio", "get_hf_decay_ratio");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "diffusion", PROPERTY_HINT_RANGE, "0,100,0.1"), "set_diffusion", "get_diffusion");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density", PROPERTY_HINT_RANGE, "0,100,0.1"), "set_density", "get_density");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_shelf_frequency", PROPERTY_HINT_RANGE, "20,1000,0.1,suffix:Hz"), "set_low_shelf_frequency", "get_low_shelf_frequency");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "low_shelf_gain", PROPERTY_HINT_RANGE, "-36,12,0.1,suffix:dB"), "set_low_shelf_gain", "get_low_shelf_gain");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "high_cut", PROPERTY_HINT_RANGE, "20,20000,0.1,suffix:Hz"), "set_high_cut", "get_high_cut");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "early_late_mix", PROPERTY_HINT_RANGE, "0,100,0.1"), "set_early_late_mix", "get_early_late_mix");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wet_level", PROPERTY_HINT_RANGE, "-80,20,0.1,suffix:dB"), "set_wet_level", "get_wet_level");
	}

	FmodReverb3D::FmodReverb3D() {

	}

	void FmodReverb3D::_apply_properties() {
		if (!reverb_3d) return;
		FMOD_ERR_CHECK(reverb_3d->setProperties(&properties));
	}

	void FmodReverb3D::_apply_3d_attributes() {
		if (!reverb_3d) return;
		FMOD_VECTOR fmod_position = {
			static_cast<float>(position.x),
			static_cast<float>(position.y),
			static_cast<float>(position.z)
		};
		FMOD_ERR_CHECK(reverb_3d->set3DAttributes(&fmod_position, min_distance, max_distance));
	}

	void FmodReverb3D::_apply_active() {
		if (!reverb_3d) return;
		FMOD_ERR_CHECK(reverb_3d->setActive(active));
	}

	FmodReverb3D::~FmodReverb3D() {
		release();
	}

	bool FmodReverb3D::reverb_3d_is_valid() const {
		return reverb_3d != nullptr;
	}

	bool FmodReverb3D::reverb_3d_is_null() const {
		return reverb_3d == nullptr;
	}

	void FmodReverb3D::setup(FMOD::Reverb3D* p_reverb_3d) {
		ERR_FAIL_COND_MSG(!p_reverb_3d, "Reverb3D pointer is null");

		if (reverb_3d == p_reverb_3d) {
			_apply_properties();
			_apply_3d_attributes();
			_apply_active();
			return;
		}

		if (reverb_3d) {
			release();
			ERR_FAIL_COND_MSG(reverb_3d != nullptr, "Failed to release previous Reverb3D before setup.");
		}

		reverb_3d = p_reverb_3d;
		reverb_3d->setUserData(this);
		_apply_properties();
		_apply_3d_attributes();
		_apply_active();
	}

	void FmodReverb3D::set_properties(const Dictionary& p_properties) {
		float value = 0.0f;
		if (_dictionary_get_float(p_properties, "decay_time", 100.0f, 20000.0f, value)) properties.DecayTime = value;
		if (_dictionary_get_float(p_properties, "early_delay", 0.0f, 300.0f, value)) properties.EarlyDelay = value;
		if (_dictionary_get_float(p_properties, "late_delay", 0.0f, 100.0f, value)) properties.LateDelay = value;
		if (_dictionary_get_float(p_properties, "hf_reference", 20.0f, 20000.0f, value)) properties.HFReference = value;
		if (_dictionary_get_float(p_properties, "hf_decay_ratio", 10.0f, 200.0f, value)) properties.HFDecayRatio = value;
		if (_dictionary_get_float(p_properties, "diffusion", 0.0f, 100.0f, value)) properties.Diffusion = value;
		if (_dictionary_get_float(p_properties, "density", 0.0f, 100.0f, value)) properties.Density = value;
		if (_dictionary_get_float(p_properties, "low_shelf_frequency", 20.0f, 1000.0f, value)) properties.LowShelfFrequency = value;
		if (_dictionary_get_float(p_properties, "low_shelf_gain", -36.0f, 12.0f, value)) properties.LowShelfGain = value;
		if (_dictionary_get_float(p_properties, "high_cut", 20.0f, 20000.0f, value)) properties.HighCut = value;
		if (_dictionary_get_float(p_properties, "early_late_mix", 0.0f, 100.0f, value)) properties.EarlyLateMix = value;
		if (_dictionary_get_float(p_properties, "wet_level", -80.0f, 20.0f, value)) properties.WetLevel = value;
		_apply_properties();
	}

	Dictionary FmodReverb3D::get_properties() const {
		Dictionary result;
		result["decay_time"] = properties.DecayTime;
		result["early_delay"] = properties.EarlyDelay;
		result["late_delay"] = properties.LateDelay;
		result["hf_reference"] = properties.HFReference;
		result["hf_decay_ratio"] = properties.HFDecayRatio;
		result["diffusion"] = properties.Diffusion;
		result["density"] = properties.Density;
		result["low_shelf_frequency"] = properties.LowShelfFrequency;
		result["low_shelf_gain"] = properties.LowShelfGain;
		result["high_cut"] = properties.HighCut;
		result["early_late_mix"] = properties.EarlyLateMix;
		result["wet_level"] = properties.WetLevel;
		return result;
	}

	void FmodReverb3D::set_3d_attributes(const Vector3 position, const float min_distance, const float max_distance) {
		if (position.is_finite()) {
			this->position = position;
		}
		else {
			ERR_PRINT("FmodReverb3D position must be finite.");
		}

		this->min_distance = MAX(_clamp_finite_or(min_distance, this->min_distance, 0.0f, FLT_MAX), 0.0f);
		this->max_distance = MAX(_clamp_finite_or(max_distance, this->max_distance, 0.0f, FLT_MAX), this->min_distance + 0.001f);
		_apply_3d_attributes();
	}

	Dictionary FmodReverb3D::get_3d_attributes() const {
		Dictionary result;
		result["position"] = position;
		result["min_distance"] = min_distance;
		result["max_distance"] = max_distance;
		return result;
	}

	void FmodReverb3D::set_active(const bool active) {
		this->active = active;
		_apply_active();
	}

	bool FmodReverb3D::get_active() const {
		return active;
	}

	void FmodReverb3D::release() {
		if (!reverb_3d) return;
		reverb_3d->setUserData(nullptr);
		FMOD_RESULT result = reverb_3d->release();
		if (result != FMOD_OK) {
			reverb_3d->setUserData(this);
			ERR_PRINT("Failed to release Reverb3D");
			return;
		}
		reverb_3d = nullptr;
	}

	void FmodReverb3D::set_decay_time(const float decay_time) {
		properties.DecayTime = _clamp_finite_or(decay_time, properties.DecayTime, 100.0f, 20000.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_decay_time() const {
		return properties.DecayTime;
	}

	void FmodReverb3D::set_early_delay(const float early_delay) {
		properties.EarlyDelay = _clamp_finite_or(early_delay, properties.EarlyDelay, 0.0f, 300.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_early_delay() const {
		return properties.EarlyDelay;
	}

	void FmodReverb3D::set_late_delay(const float late_delay) {
		properties.LateDelay = _clamp_finite_or(late_delay, properties.LateDelay, 0.0f, 100.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_late_delay() const {
		return properties.LateDelay;
	}

	void FmodReverb3D::set_hf_reference(const float hf_reference) {
		properties.HFReference = _clamp_finite_or(hf_reference, properties.HFReference, 20.0f, 20000.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_hf_reference() const {
		return properties.HFReference;
	}

	void FmodReverb3D::set_hf_decay_ratio(const float hf_decay_ratio) {
		properties.HFDecayRatio = _clamp_finite_or(hf_decay_ratio, properties.HFDecayRatio, 10.0f, 200.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_hf_decay_ratio() const {
		return properties.HFDecayRatio;
	}

	void FmodReverb3D::set_diffusion(const float diffusion) {
		properties.Diffusion = _clamp_finite_or(diffusion, properties.Diffusion, 0.0f, 100.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_diffusion() const {
		return properties.Diffusion;
	}

	void FmodReverb3D::set_density(const float density) {
		properties.Density = _clamp_finite_or(density, properties.Density, 0.0f, 100.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_density() const {
		return properties.Density;
	}

	void FmodReverb3D::set_low_shelf_frequency(const float low_shelf_frequency) {
		properties.LowShelfFrequency = _clamp_finite_or(low_shelf_frequency, properties.LowShelfFrequency, 20.0f, 1000.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_low_shelf_frequency() const {
		return properties.LowShelfFrequency;
	}

	void FmodReverb3D::set_low_shelf_gain(const float low_shelf_gain) {
		properties.LowShelfGain = _clamp_finite_or(low_shelf_gain, properties.LowShelfGain, -36.0f, 12.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_low_shelf_gain() const {
		return properties.LowShelfGain;
	}

	void FmodReverb3D::set_high_cut(const float high_cut) {
		properties.HighCut = _clamp_finite_or(high_cut, properties.HighCut, 20.0f, 20000.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_high_cut() const {
		return properties.HighCut;
	}

	void FmodReverb3D::set_early_late_mix(const float early_late_mix) {
		properties.EarlyLateMix = _clamp_finite_or(early_late_mix, properties.EarlyLateMix, 0.0f, 100.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_early_late_mix() const {
		return properties.EarlyLateMix;
	}

	void FmodReverb3D::set_wet_level(const float wet_level) {
		properties.WetLevel = _clamp_finite_or(wet_level, properties.WetLevel, -80.0f, 20.0f);
		_apply_properties();
	}

	float FmodReverb3D::get_wet_level() const {
		return properties.WetLevel;
	}
}
