#include "dsp/fmod_audio_effect_eq.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

	// FMOD MULTIBAND_EQ 每个 DSP 支持的频段数
	static const int BANDS_PER_DSP = 5;
	// FMOD MULTIBAND_EQ 频段参数偏移量
	static const int PARAMS_PER_BAND = 4;  // FILTER, FREQUENCY, Q, GAIN

	// 辅助函数：将 FilterType 映射到 FMOD DSP 滤波器类型
	static int _filter_type_to_fmod(FmodAudioEffectEQ::FilterType type) {
		switch (type) {
			case FmodAudioEffectEQ::FILTER_DISABLED:      return FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED;
			case FmodAudioEffectEQ::FILTER_LOWPASS_12DB:  return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_12DB;
			case FmodAudioEffectEQ::FILTER_LOWPASS_24DB:  return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_24DB;
			case FmodAudioEffectEQ::FILTER_LOWPASS_48DB:  return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_48DB;
			case FmodAudioEffectEQ::FILTER_HIGHPASS_12DB: return FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_12DB;
			case FmodAudioEffectEQ::FILTER_HIGHPASS_24DB: return FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_24DB;
			case FmodAudioEffectEQ::FILTER_HIGHPASS_48DB: return FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_48DB;
			case FmodAudioEffectEQ::FILTER_LOWSHELF:      return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWSHELF;
			case FmodAudioEffectEQ::FILTER_HIGHSHELF:     return FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHSHELF;
			case FmodAudioEffectEQ::FILTER_PEAKING:       return FMOD_DSP_MULTIBAND_EQ_FILTER_PEAKING;
			case FmodAudioEffectEQ::FILTER_BANDPASS:      return FMOD_DSP_MULTIBAND_EQ_FILTER_BANDPASS;
			case FmodAudioEffectEQ::FILTER_NOTCH:         return FMOD_DSP_MULTIBAND_EQ_FILTER_NOTCH;
			case FmodAudioEffectEQ::FILTER_ALLPASS:       return FMOD_DSP_MULTIBAND_EQ_FILTER_ALLPASS;
			case FmodAudioEffectEQ::FILTER_LOWPASS_6DB:   return FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_6DB;
			case FmodAudioEffectEQ::FILTER_HIGHPASS_6DB:  return FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_6DB;
		}
		return FMOD_DSP_MULTIBAND_EQ_FILTER_PEAKING;  // 默认
	}

	void FmodAudioEffectEQ::_bind_methods() {
		BIND_ENUM_CONSTANT(PRESET_6_BANDS);
		BIND_ENUM_CONSTANT(PRESET_10_BANDS);
		BIND_ENUM_CONSTANT(PRESET_21_BANDS);

		BIND_ENUM_CONSTANT(FILTER_DISABLED);
		BIND_ENUM_CONSTANT(FILTER_LOWPASS_12DB);
		BIND_ENUM_CONSTANT(FILTER_LOWPASS_24DB);
		BIND_ENUM_CONSTANT(FILTER_LOWPASS_48DB);
		BIND_ENUM_CONSTANT(FILTER_HIGHPASS_12DB);
		BIND_ENUM_CONSTANT(FILTER_HIGHPASS_24DB);
		BIND_ENUM_CONSTANT(FILTER_HIGHPASS_48DB);
		BIND_ENUM_CONSTANT(FILTER_LOWSHELF);
		BIND_ENUM_CONSTANT(FILTER_HIGHSHELF);
		BIND_ENUM_CONSTANT(FILTER_PEAKING);
		BIND_ENUM_CONSTANT(FILTER_BANDPASS);
		BIND_ENUM_CONSTANT(FILTER_NOTCH);
		BIND_ENUM_CONSTANT(FILTER_ALLPASS);
		BIND_ENUM_CONSTANT(FILTER_LOWPASS_6DB);
		BIND_ENUM_CONSTANT(FILTER_HIGHPASS_6DB);

		ClassDB::bind_method(D_METHOD("set_preset", "preset"), &FmodAudioEffectEQ::set_preset);
		ClassDB::bind_method(D_METHOD("get_preset"), &FmodAudioEffectEQ::get_preset);

		ClassDB::bind_method(D_METHOD("get_band_count"), &FmodAudioEffectEQ::get_band_count);
		ClassDB::bind_method(D_METHOD("set_band_gain_db", "band_idx", "gain_db"), &FmodAudioEffectEQ::set_band_gain_db);
		ClassDB::bind_method(D_METHOD("get_band_gain_db", "band_idx"), &FmodAudioEffectEQ::get_band_gain_db);
		ClassDB::bind_method(D_METHOD("set_band_frequency", "band_idx", "frequency"), &FmodAudioEffectEQ::set_band_frequency);
		ClassDB::bind_method(D_METHOD("get_band_frequency", "band_idx"), &FmodAudioEffectEQ::get_band_frequency);
		ClassDB::bind_method(D_METHOD("set_band_filter_type", "band_idx", "filter_type"), &FmodAudioEffectEQ::set_band_filter_type);
		ClassDB::bind_method(D_METHOD("get_band_filter_type", "band_idx"), &FmodAudioEffectEQ::get_band_filter_type);
		ClassDB::bind_method(D_METHOD("set_band_q", "band_idx", "q"), &FmodAudioEffectEQ::set_band_q);
		ClassDB::bind_method(D_METHOD("get_band_q", "band_idx"), &FmodAudioEffectEQ::get_band_q);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "preset", PROPERTY_HINT_ENUM, "6 Bands,10 Bands,21 Bands"), "set_preset", "get_preset");
	}

	FmodAudioEffectEQ::FmodAudioEffectEQ() {
		_init_bands();
	}

	FmodAudioEffectEQ::FmodAudioEffectEQ(Preset p_preset) {
		preset = p_preset;
		_init_bands();
	}

	FmodAudioEffectEQ::~FmodAudioEffectEQ() {
	}

	void FmodAudioEffectEQ::_init_bands() {
		bands.clear();
		band_names.clear();

		Vector<float> frequencies;
		_get_preset_frequencies(preset, frequencies);

		int band_count = frequencies.size();
		bands.resize(band_count);
		band_names.resize(band_count);

		for (int i = 0; i < band_count; i++) {
			bands.write[i].frequency = frequencies[i];
			bands.write[i].gain_db = 0.0f;
			bands.write[i].q = 0.707f;
			bands.write[i].filter_type = FILTER_PEAKING;
			band_names.write[i] = String::num_int64(i);
		}
	}

	void FmodAudioEffectEQ::_get_preset_frequencies(Preset p_preset, Vector<float>& r_frequencies) const {
		r_frequencies.clear();
		switch (p_preset) {
			case PRESET_6_BANDS:
				// Godot AudioEffectEQ6 的频段: 32Hz, 100Hz, 320Hz, 1000Hz, 3200Hz, 10000Hz
				r_frequencies.push_back(32.0f);
				r_frequencies.push_back(100.0f);
				r_frequencies.push_back(320.0f);
				r_frequencies.push_back(1000.0f);
				r_frequencies.push_back(3200.0f);
				r_frequencies.push_back(10000.0f);
				break;
			case PRESET_10_BANDS:
				// Godot AudioEffectEQ10 的频段: 31Hz, 62Hz, 125Hz, 250Hz, 500Hz, 1000Hz, 2000Hz, 4000Hz, 8000Hz, 16000Hz
				r_frequencies.push_back(31.25f);
				r_frequencies.push_back(62.5f);
				r_frequencies.push_back(125.0f);
				r_frequencies.push_back(250.0f);
				r_frequencies.push_back(500.0f);
				r_frequencies.push_back(1000.0f);
				r_frequencies.push_back(2000.0f);
				r_frequencies.push_back(4000.0f);
				r_frequencies.push_back(8000.0f);
				r_frequencies.push_back(16000.0f);
				break;
			case PRESET_21_BANDS:
				// Godot AudioEffectEQ21 的频段: 22Hz 到 22000Hz
				r_frequencies.push_back(22.0f);
				r_frequencies.push_back(32.0f);
				r_frequencies.push_back(44.0f);
				r_frequencies.push_back(63.0f);
				r_frequencies.push_back(90.0f);
				r_frequencies.push_back(125.0f);
				r_frequencies.push_back(175.0f);
				r_frequencies.push_back(250.0f);
				r_frequencies.push_back(350.0f);
				r_frequencies.push_back(500.0f);
				r_frequencies.push_back(700.0f);
				r_frequencies.push_back(1000.0f);
				r_frequencies.push_back(1400.0f);
				r_frequencies.push_back(2000.0f);
				r_frequencies.push_back(2800.0f);
				r_frequencies.push_back(4000.0f);
				r_frequencies.push_back(5600.0f);
				r_frequencies.push_back(8000.0f);
				r_frequencies.push_back(11000.0f);
				r_frequencies.push_back(16000.0f);
				r_frequencies.push_back(22000.0f);
				break;
		}
	}

	int FmodAudioEffectEQ::_get_required_dsp_count() const {
		int band_count = bands.size();
		return (band_count + BANDS_PER_DSP - 1) / BANDS_PER_DSP;  // 向上取整
	}

	void FmodAudioEffectEQ::_get_dsp_band_indices(int p_band_idx, int& r_dsp_idx, int& r_band_letter_idx) const {
		r_dsp_idx = p_band_idx / BANDS_PER_DSP;
		r_band_letter_idx = p_band_idx % BANDS_PER_DSP;  // 0-4 对应 A-E
	}

	void FmodAudioEffectEQ::_update_band_dsp(int p_band_idx) {
		// 如果已经应用到了 bus，更新对应的 DSP 参数
		if (bus.is_null() || dsp_chain.is_empty()) {
			return;
		}

		int dsp_idx, band_letter_idx;
		_get_dsp_band_indices(p_band_idx, dsp_idx, band_letter_idx);

		if (dsp_idx >= dsp_chain.size()) {
			return;
		}

		Ref<FmodDSP> dsp = dsp_chain[dsp_idx];
		if (!dsp.is_valid()) {
			return;
		}

		const BandInfo& band = bands[p_band_idx];
		int param_base = band_letter_idx * PARAMS_PER_BAND;

		// 设置滤波器类型
		dsp->set_parameter_int(param_base + 0, _filter_type_to_fmod(band.filter_type));
		// 设置频率
		dsp->set_parameter_float(param_base + 1, band.frequency);
		// 设置 Q 值
		dsp->set_parameter_float(param_base + 2, band.q);
		// 设置增益 (dB)
		dsp->set_parameter_float(param_base + 3, band.gain_db);
	}

	void FmodAudioEffectEQ::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		// 如果已经应用到了同一个 bus，先移除旧的
		if (bus == p_bus && !dsp_chain.is_empty()) {
			remove_from_bus(p_bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		// 计算需要的 DSP 数量
		int dsp_count = _get_required_dsp_count();

		for (int dsp_idx = 0; dsp_idx < dsp_count; dsp_idx++) {
			// 创建 MULTIBAND_EQ DSP
			Ref<FmodDSP> eq_dsp = system->create_dsp_by_type(FmodDSP::DSP_TYPE_MULTIBAND_EQ);
			if (!eq_dsp.is_valid()) {
				ERR_PRINT("Failed to create MULTIBAND_EQ DSP");
				continue;
			}

			// 配置该 DSP 的 5 个频段（或剩余频段）
			for (int band_letter_idx = 0; band_letter_idx < BANDS_PER_DSP; band_letter_idx++) {
				int band_idx = dsp_idx * BANDS_PER_DSP + band_letter_idx;
				if (band_idx >= bands.size()) {
					// 禁用未使用的频段
					int param_base = band_letter_idx * PARAMS_PER_BAND;
					eq_dsp->set_parameter_int(param_base + 0, FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED);
					continue;
				}

				const BandInfo& band = bands[band_idx];
				int param_base = band_letter_idx * PARAMS_PER_BAND;

				// 将 FilterType 映射到 FMOD_DSP_MULTIBAND_EQ_FILTER_TYPE
				int fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_PEAKING;
				switch (band.filter_type) {
					case FILTER_DISABLED:      fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED; break;
					case FILTER_LOWPASS_12DB:  fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_12DB; break;
					case FILTER_LOWPASS_24DB:  fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_24DB; break;
					case FILTER_LOWPASS_48DB:  fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_48DB; break;
					case FILTER_HIGHPASS_12DB: fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_12DB; break;
					case FILTER_HIGHPASS_24DB: fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_24DB; break;
					case FILTER_HIGHPASS_48DB: fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_48DB; break;
					case FILTER_LOWSHELF:      fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_LOWSHELF; break;
					case FILTER_HIGHSHELF:     fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHSHELF; break;
					case FILTER_PEAKING:       fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_PEAKING; break;
					case FILTER_BANDPASS:      fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_BANDPASS; break;
					case FILTER_NOTCH:         fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_NOTCH; break;
					case FILTER_ALLPASS:       fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_ALLPASS; break;
					case FILTER_LOWPASS_6DB:   fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_LOWPASS_6DB; break;
					case FILTER_HIGHPASS_6DB:  fmod_filter_type = FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHPASS_6DB; break;
				}

				// 设置滤波器类型
				eq_dsp->set_parameter_int(param_base + 0, fmod_filter_type);
				// 设置频率
				eq_dsp->set_parameter_float(param_base + 1, band.frequency);
				// 设置 Q 值
				eq_dsp->set_parameter_float(param_base + 2, band.q);
				// 设置增益 (dB)
				eq_dsp->set_parameter_float(param_base + 3, band.gain_db);
			}

			// 添加到 bus
			bus->add_dsp(-1, eq_dsp);
			dsp_chain.push_back(eq_dsp);
		}
	}

	void FmodAudioEffectEQ::set_preset(Preset p_preset) {
		ERR_FAIL_COND(p_preset < PRESET_6_BANDS || p_preset > PRESET_21_BANDS);
		if (preset == p_preset) {
			return;
		}
		preset = p_preset;

		// 重新初始化频段
		_init_bands();

		// 如果已经应用到了 bus，需要重新应用
		if (bus.is_valid() && !dsp_chain.is_empty()) {
			Ref<FmodChannelGroup> old_bus = bus;
			remove_from_bus(old_bus);
			apply_to(old_bus);
		}
		notify_property_list_changed();
		emit_changed();
	}

	FmodAudioEffectEQ::Preset FmodAudioEffectEQ::get_preset() const {
		return preset;
	}

	int FmodAudioEffectEQ::get_band_count() const {
		return bands.size();
	}

	void FmodAudioEffectEQ::set_band_gain_db(int p_band_idx, float p_gain_db) {
		ERR_FAIL_INDEX(p_band_idx, bands.size());
		const float gain = CLAMP(p_gain_db, -60.0f, 24.0f);
		if (bands[p_band_idx].gain_db == gain) {
			return;
		}
		bands.write[p_band_idx].gain_db = gain;
		_update_band_dsp(p_band_idx);
		emit_changed();
	}

	float FmodAudioEffectEQ::get_band_gain_db(int p_band_idx) const {
		ERR_FAIL_INDEX_V(p_band_idx, bands.size(), 0.0f);
		return bands[p_band_idx].gain_db;
	}

	void FmodAudioEffectEQ::set_band_frequency(int p_band_idx, float p_frequency) {
		ERR_FAIL_INDEX(p_band_idx, bands.size());
		// FMOD 的有效频率范围通常是 10Hz - 22000Hz
		const float frequency = CLAMP(p_frequency, 10.0f, 22000.0f);
		if (bands[p_band_idx].frequency == frequency) {
			return;
		}
		bands.write[p_band_idx].frequency = frequency;
		_update_band_dsp(p_band_idx);
		emit_changed();
	}

	float FmodAudioEffectEQ::get_band_frequency(int p_band_idx) const {
		ERR_FAIL_INDEX_V(p_band_idx, bands.size(), 1000.0f);
		return bands[p_band_idx].frequency;
	}

	void FmodAudioEffectEQ::set_band_filter_type(int p_band_idx, FilterType p_type) {
		ERR_FAIL_INDEX(p_band_idx, bands.size());
		if (bands[p_band_idx].filter_type == p_type) {
			return;
		}
		bands.write[p_band_idx].filter_type = p_type;
		_update_band_dsp(p_band_idx);
		emit_changed();
	}

	FmodAudioEffectEQ::FilterType FmodAudioEffectEQ::get_band_filter_type(int p_band_idx) const {
		ERR_FAIL_INDEX_V(p_band_idx, bands.size(), FILTER_PEAKING);
		return bands[p_band_idx].filter_type;
	}

	void FmodAudioEffectEQ::set_band_q(int p_band_idx, float p_q) {
		ERR_FAIL_INDEX(p_band_idx, bands.size());
		// Q 值范围: 0.01 ~ 10.0 是合理范围
		const float q = CLAMP(p_q, 0.01f, 10.0f);
		if (bands[p_band_idx].q == q) {
			return;
		}
		bands.write[p_band_idx].q = q;
		_update_band_dsp(p_band_idx);
		emit_changed();
	}

	float FmodAudioEffectEQ::get_band_q(int p_band_idx) const {
		ERR_FAIL_INDEX_V(p_band_idx, bands.size(), 1.0f);
		return bands[p_band_idx].q;
	}

	bool FmodAudioEffectEQ::_set(const StringName& p_name, const Variant& p_value) {
		String name = p_name;
		if (name.begins_with("band_")) {
			// 解析 band_XX_property 格式
			String rest = name.substr(5);  // 去掉 "band_"
			int underscore_pos = rest.find("_");
			if (underscore_pos == -1) {
				return false;
			}

			String band_str = rest.substr(0, underscore_pos);
			String prop = rest.substr(underscore_pos + 1);

			int band_idx = band_str.to_int();
			if (band_idx < 0 || band_idx >= bands.size()) {
				return false;
			}

			if (prop == "gain") {
				set_band_gain_db(band_idx, p_value);
				return true;
			} else if (prop == "frequency") {
				set_band_frequency(band_idx, p_value);
				return true;
			} else if (prop == "q") {
				set_band_q(band_idx, p_value);
				return true;
			} else if (prop == "filter_type") {
				set_band_filter_type(band_idx, static_cast<FilterType>(int(p_value)));
				return true;
			}
		}
		return false;
	}

	bool FmodAudioEffectEQ::_get(const StringName& p_name, Variant& r_ret) const {
		String name = p_name;
		if (name.begins_with("band_")) {
			// 解析 band_XX_property 格式
			String rest = name.substr(5);  // 去掉 "band_"
			int underscore_pos = rest.find("_");
			if (underscore_pos == -1) {
				return false;
			}

			String band_str = rest.substr(0, underscore_pos);
			String prop = rest.substr(underscore_pos + 1);

			int band_idx = band_str.to_int();
			if (band_idx < 0 || band_idx >= bands.size()) {
				return false;
			}

			if (prop == "gain") {
				r_ret = get_band_gain_db(band_idx);
				return true;
			} else if (prop == "frequency") {
				r_ret = get_band_frequency(band_idx);
				return true;
			} else if (prop == "q") {
				r_ret = get_band_q(band_idx);
				return true;
			} else if (prop == "filter_type") {
				r_ret = static_cast<int>(get_band_filter_type(band_idx));
				return true;
			}
		}
		return false;
	}

	void FmodAudioEffectEQ::_get_property_list(List<PropertyInfo>* p_list) const {
		// 为每个频段添加属性
		for (int i = 0; i < bands.size(); i++) {
			String band_prefix = "band_" + String::num_int64(i) + "_";
			String band_name = band_names[i];

			// 增益 (dB) - 这是 Godot AudioEffectEQ 的主要属性
			p_list->push_back(PropertyInfo(
				Variant::FLOAT,
				band_prefix + "gain",
				PROPERTY_HINT_RANGE,
				"-60,24,0.1,suffix:dB",
				PROPERTY_USAGE_DEFAULT,
				band_name + " Gain"
			));

			// 频率 (Hz)
			p_list->push_back(PropertyInfo(
				Variant::FLOAT,
				band_prefix + "frequency",
				PROPERTY_HINT_RANGE,
				"10,22000,1,suffix:Hz",
				PROPERTY_USAGE_DEFAULT,
				band_name + " Frequency"
			));

			// Q 值
			p_list->push_back(PropertyInfo(
				Variant::FLOAT,
				band_prefix + "q",
				PROPERTY_HINT_RANGE,
				"0.01,10,0.01",
				PROPERTY_USAGE_DEFAULT,
				band_name + " Q"
			));

			// 滤波器类型
			p_list->push_back(PropertyInfo(
				Variant::INT,
				band_prefix + "filter_type",
				PROPERTY_HINT_ENUM,
				"Disabled,LowPass 12dB,LowPass 24dB,LowPass 48dB,"
				"HighPass 12dB,HighPass 24dB,HighPass 48dB,"
				"LowShelf,HighShelf,Peaking,BandPass,Notch,AllPass,LowPass 6dB,HighPass 6dB",
				PROPERTY_USAGE_DEFAULT,
				band_name + " Filter Type"
			));
		}
	}

} // namespace godot
