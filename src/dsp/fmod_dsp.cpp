#include "fmod_dsp.h"
#include "fmod_dsp_connection.h"

#include <cstring>
#include <limits>

#include <godot_cpp/classes/os.hpp>

namespace godot {
	static bool _is_main_thread() {
		OS* os = OS::get_singleton();
		return os && os->get_thread_caller_id() == os->get_main_thread_id();
	}

	static String _fmod_fixed_string_to_godot(const char* p_chars, int p_length) {
		if (!p_chars || p_length <= 0) {
			return String();
		}

		int safe_length = 0;
		while (safe_length < p_length && p_chars[safe_length] != '\0') {
			safe_length++;
		}

		return safe_length > 0 ? String::utf8(p_chars, safe_length) : String();
	}

	static void _copy_fmod_valuestr(char* p_valuestr, const String& p_value) {
		if (!p_valuestr) {
			return;
		}

		CharString utf8 = p_value.utf8();
		const int copy_len = MIN(static_cast<int>(utf8.length()), FMOD_DSP_GETPARAM_VALUESTR_LENGTH - 1);
		if (copy_len > 0) {
			memcpy(p_valuestr, utf8.get_data(), copy_len);
		}
		p_valuestr[copy_len] = '\0';
	}

	void FmodDSP::_bind_methods() {
		BIND_ENUM_CONSTANT(DSP_MODE_FILTER);
		BIND_ENUM_CONSTANT(DSP_MODE_GENERATOR);
		BIND_CONSTANT(DSP_COOLDOWN_INFINITE);
		ClassDB::bind_method(D_METHOD("add_input_preallocated", "target_dsp", "connection"), &FmodDSP::add_input_preallocated);
		BIND_ENUM_CONSTANT(DSP_TYPE_UNKNOWN);
		BIND_ENUM_CONSTANT(DSP_TYPE_MIXER);
		BIND_ENUM_CONSTANT(DSP_TYPE_OSCILLATOR);
		BIND_ENUM_CONSTANT(DSP_TYPE_LOWPASS);
		BIND_ENUM_CONSTANT(DSP_TYPE_ITLOWPASS);
		BIND_ENUM_CONSTANT(DSP_TYPE_HIGHPASS);
		BIND_ENUM_CONSTANT(DSP_TYPE_ECHO);
		BIND_ENUM_CONSTANT(DSP_TYPE_FADER);
		BIND_ENUM_CONSTANT(DSP_TYPE_FLANGE);
		BIND_ENUM_CONSTANT(DSP_TYPE_DISTORTION);
		BIND_ENUM_CONSTANT(DSP_TYPE_NORMALIZE);
		BIND_ENUM_CONSTANT(DSP_TYPE_LIMITER);
		BIND_ENUM_CONSTANT(DSP_TYPE_PARAMEQ);
		BIND_ENUM_CONSTANT(DSP_TYPE_PITCHSHIFT);
		BIND_ENUM_CONSTANT(DSP_TYPE_CHORUS);
		BIND_ENUM_CONSTANT(DSP_TYPE_ITECHO);
		BIND_ENUM_CONSTANT(DSP_TYPE_COMPRESSOR);
		BIND_ENUM_CONSTANT(DSP_TYPE_SFXREVERB);
		BIND_ENUM_CONSTANT(DSP_TYPE_LOWPASS_SIMPLE);
		BIND_ENUM_CONSTANT(DSP_TYPE_DELAY);
		BIND_ENUM_CONSTANT(DSP_TYPE_TREMOLO);
		BIND_ENUM_CONSTANT(DSP_TYPE_SEND);
		BIND_ENUM_CONSTANT(DSP_TYPE_RETURN);
		BIND_ENUM_CONSTANT(DSP_TYPE_HIGHPASS_SIMPLE);
		BIND_ENUM_CONSTANT(DSP_TYPE_PAN);
		BIND_ENUM_CONSTANT(DSP_TYPE_THREE_EQ);
		BIND_ENUM_CONSTANT(DSP_TYPE_FFT);
		BIND_ENUM_CONSTANT(DSP_TYPE_LOUDNESS_METER);
		BIND_ENUM_CONSTANT(DSP_TYPE_CONVOLUTIONREVERB);
		BIND_ENUM_CONSTANT(DSP_TYPE_CHANNELMIX);
		BIND_ENUM_CONSTANT(DSP_TYPE_TRANSCEIVER);
		BIND_ENUM_CONSTANT(DSP_TYPE_OBJECTPAN);
		BIND_ENUM_CONSTANT(DSP_TYPE_MULTIBAND_EQ);
		BIND_ENUM_CONSTANT(DSP_TYPE_MULTIBAND_DYNAMICS);
		BIND_ENUM_CONSTANT(DSP_TYPE_MAX);
		BIND_ENUM_CONSTANT(DSP_TYPE_FORCEINT);

		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_USER);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_OVERALLGAIN);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_SIDECHAIN);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_FFT);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES_MULTI);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_ATTENUATION_RANGE);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_DYNAMIC_RESPONSE);
		BIND_ENUM_CONSTANT(DSP_PARAMETER_DATA_TYPE_FINITE_LENGTH);

		ClassDB::bind_method(D_METHOD("add_input", "target_dsp", "type"), &FmodDSP::add_input, DEFVAL(FmodDSPConnection::FmodDSPConnectionType::DSPCONNECTION_TYPE_STANDARD));
		ClassDB::bind_method(D_METHOD("get_input", "index"), &FmodDSP::get_input);
		ClassDB::bind_method(D_METHOD("get_output", "index"), &FmodDSP::get_output);
		ClassDB::bind_method(D_METHOD("get_num_inputs"), &FmodDSP::get_num_inputs);
		ClassDB::bind_method(D_METHOD("get_num_outputs"), &FmodDSP::get_num_outputs);
		ClassDB::bind_method(D_METHOD("disconnect_all", "inputs", "outputs"), &FmodDSP::disconnect_all);
		ClassDB::bind_method(D_METHOD("disconnect_from", "target", "connection"), &FmodDSP::disconnect_from, DEFVAL(Ref<FmodDSPConnection>()));

		ClassDB::bind_method(D_METHOD("set_input_channel_format", "numchannels", "speakermode"), &FmodDSP::set_input_channel_format);
		ClassDB::bind_method(D_METHOD("get_input_channel_format"), &FmodDSP::get_input_channel_format);
		ClassDB::bind_method(D_METHOD("get_output_channel_format", "inchannels", "inspeakermode"), &FmodDSP::get_output_channel_format);

		ClassDB::bind_method(D_METHOD("get_metering_info"), &FmodDSP::get_metering_info);
		ClassDB::bind_method(D_METHOD("set_metering_enabled", "input_enabled", "output_enabled"), &FmodDSP::set_metering_enabled);
		ClassDB::bind_method(D_METHOD("get_metering_enabled"), &FmodDSP::get_metering_enabled);

		ClassDB::bind_method(D_METHOD("get_data_parameter_index", "data_type"), &FmodDSP::get_data_parameter_index);
		ClassDB::bind_method(D_METHOD("get_num_parameters"), &FmodDSP::get_num_parameters);
		ClassDB::bind_method(D_METHOD("set_parameter_bool", "index", "value"), &FmodDSP::set_parameter_bool);
		ClassDB::bind_method(D_METHOD("get_parameter_bool", "index"), &FmodDSP::get_parameter_bool);
		ClassDB::bind_method(D_METHOD("set_parameter_data", "index", "data"), &FmodDSP::set_parameter_data);
		ClassDB::bind_method(D_METHOD("get_parameter_data", "index"), &FmodDSP::get_parameter_data);
		ClassDB::bind_method(D_METHOD("set_parameter_float", "index", "value"), &FmodDSP::set_parameter_float);
		ClassDB::bind_method(D_METHOD("get_parameter_float", "index"), &FmodDSP::get_parameter_float);
		ClassDB::bind_method(D_METHOD("set_parameter_int", "index", "value"), &FmodDSP::set_parameter_int);
		ClassDB::bind_method(D_METHOD("get_parameter_int", "index"), &FmodDSP::get_parameter_int);
		ClassDB::bind_method(D_METHOD("get_parameter_info", "index"), &FmodDSP::get_parameter_info);

		ClassDB::bind_method(D_METHOD("set_active", "active"), &FmodDSP::set_active);
		ClassDB::bind_method(D_METHOD("get_active"), &FmodDSP::get_active);

		ClassDB::bind_method(D_METHOD("set_bypass", "bypass"), &FmodDSP::set_bypass);
		ClassDB::bind_method(D_METHOD("get_bypass"), &FmodDSP::get_bypass);

		ClassDB::bind_method(D_METHOD("set_wet_dry_mix", "prewet", "postwet", "dry"), &FmodDSP::set_wet_dry_mix);
		ClassDB::bind_method(D_METHOD("get_wet_dry_mix"), &FmodDSP::get_wet_dry_mix);
		ClassDB::bind_method(D_METHOD("get_idle"), &FmodDSP::get_idle);

		ClassDB::bind_method(D_METHOD("reset"), &FmodDSP::reset);
		ClassDB::bind_method(D_METHOD("release"), &FmodDSP::release);
		ClassDB::bind_method(D_METHOD("get_type"), &FmodDSP::get_type);
		ClassDB::bind_method(D_METHOD("get_info"), &FmodDSP::get_info);
		ClassDB::bind_method(D_METHOD("get_cpu_usage"), &FmodDSP::get_cpu_usage);
		ClassDB::bind_method(D_METHOD("get_system_object"), &FmodDSP::get_system_object);

		ClassDB::bind_method(D_METHOD("set_create_callback", "callback"), &FmodDSP::set_create_callback);
		ClassDB::bind_method(D_METHOD("get_create_callback"), &FmodDSP::get_create_callback);
		ClassDB::bind_method(D_METHOD("set_release_callback", "callback"), &FmodDSP::set_release_callback);
		ClassDB::bind_method(D_METHOD("get_release_callback"), &FmodDSP::get_release_callback);
		ClassDB::bind_method(D_METHOD("set_reset_callback", "callback"), &FmodDSP::set_reset_callback);
		ClassDB::bind_method(D_METHOD("get_reset_callback"), &FmodDSP::get_reset_callback);
		ClassDB::bind_method(D_METHOD("set_process_callback", "callback"), &FmodDSP::set_process_callback);
		ClassDB::bind_method(D_METHOD("get_process_callback"), &FmodDSP::get_process_callback);
		ClassDB::bind_method(D_METHOD("set_setposition_callback", "callback"), &FmodDSP::set_setposition_callback);
		ClassDB::bind_method(D_METHOD("get_setposition_callback"), &FmodDSP::get_setposition_callback);
		ClassDB::bind_method(D_METHOD("set_setparam_float_callback", "callback"), &FmodDSP::set_setparam_float_callback);
		ClassDB::bind_method(D_METHOD("get_setparam_float_callback"), &FmodDSP::get_setparam_float_callback);
		ClassDB::bind_method(D_METHOD("set_setparam_int_callback", "callback"), &FmodDSP::set_setparam_int_callback);
		ClassDB::bind_method(D_METHOD("get_setparam_int_callback"), &FmodDSP::get_setparam_int_callback);
		ClassDB::bind_method(D_METHOD("set_setparam_bool_callback", "callback"), &FmodDSP::set_setparam_bool_callback);
		ClassDB::bind_method(D_METHOD("get_setparam_bool_callback"), &FmodDSP::get_setparam_bool_callback);
		ClassDB::bind_method(D_METHOD("set_setparam_data_callback", "callback"), &FmodDSP::set_setparam_data_callback);
		ClassDB::bind_method(D_METHOD("get_setparam_data_callback"), &FmodDSP::get_setparam_data_callback);
		ClassDB::bind_method(D_METHOD("set_getparam_float_callback", "callback"), &FmodDSP::set_getparam_float_callback);
		ClassDB::bind_method(D_METHOD("get_getparam_float_callback"), &FmodDSP::get_getparam_float_callback);
		ClassDB::bind_method(D_METHOD("set_getparam_int_callback", "callback"), &FmodDSP::set_getparam_int_callback);
		ClassDB::bind_method(D_METHOD("get_getparam_int_callback"), &FmodDSP::get_getparam_int_callback);
		ClassDB::bind_method(D_METHOD("set_getparam_bool_callback", "callback"), &FmodDSP::set_getparam_bool_callback);
		ClassDB::bind_method(D_METHOD("get_getparam_bool_callback"), &FmodDSP::get_getparam_bool_callback);
		ClassDB::bind_method(D_METHOD("set_getparam_data_callback", "callback"), &FmodDSP::set_getparam_data_callback);
		ClassDB::bind_method(D_METHOD("get_getparam_data_callback"), &FmodDSP::get_getparam_data_callback);

		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "get_active");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bypass"), "set_bypass", "get_bypass");
	}

	FmodDSP::FmodDSP() {}

	FmodDSP::~FmodDSP() {
		if (dsp) {
			_detach_from_dsp();
			if (owns_dsp && dsp->release() != FMOD_OK) {
				ERR_PRINT("Failed to release DSP while destroying FmodDSP");
			}
			else {
				dsp = nullptr;
			}
		}
	}

	void FmodDSP::_detach_from_dsp() {
		if (!dsp) {
			return;
		}

		void* userdata = nullptr;
		if (dsp->getUserData(&userdata) == FMOD_OK && userdata == this) {
			dsp->setUserData(nullptr);
		}
		if (callbacks_set) {
			dsp->setCallback(nullptr);
			callbacks_set = false;
		}
	}

	void FmodDSP::setup(FMOD::DSP* p_dsp, bool p_owns_dsp) {
		ERR_FAIL_COND_MSG(!p_dsp, "DSP pointer is null");
		ERR_FAIL_COND_MSG(dsp != nullptr, "FmodDSP is already bound to an FMOD DSP");

		dsp = p_dsp;
		owns_dsp = p_owns_dsp;

		if (!owns_dsp) {
			return;
		}

		FMOD_ERR_CHECK(dsp->setUserData(this));

		FMOD_DSP_TYPE dsp_type;
		if (dsp->getType(&dsp_type) == FMOD_OK) {
			if (dsp_type == FMOD_DSP_TYPE_UNKNOWN) {
				// 可能是自定义 DSP，需要设置通用回调
				if (dsp->setCallback(GD_FMOD_DSP_CALLBACK) == FMOD_OK) {
					callbacks_set = true;
				}
			}
		}
	}

	Ref<FmodDSPConnection> FmodDSP::add_input(Ref<FmodDSP> target_dsp, unsigned int type) {
		ERR_FAIL_COND_V(!dsp, Ref<FmodDSPConnection>());
		ERR_FAIL_COND_V(target_dsp.is_null() || !target_dsp->dsp, Ref<FmodDSPConnection>());
		FMOD::DSPConnection* dsp_connection_ptr = nullptr;
		FMOD_DSPCONNECTION_TYPE connection_type = static_cast<FMOD_DSPCONNECTION_TYPE>(type);
		FMOD_ERR_CHECK_V(dsp->addInput(target_dsp->dsp, &dsp_connection_ptr, connection_type), Ref<FmodDSPConnection>());
		Ref<FmodDSPConnection> dsp_connection;
		dsp_connection.instantiate();
		dsp_connection->setup(dsp_connection_ptr);
		return dsp_connection;
	}

	Ref<FmodDSPConnection> FmodDSP::add_input_preallocated(Ref<FmodDSP> target_dsp, Ref<FmodDSPConnection> connection) {
		ERR_FAIL_COND_V(!dsp || target_dsp.is_null() || !target_dsp->dsp || connection.is_null() || !connection->dsp_connection, Ref<FmodDSPConnection>());
		FMOD::DSPConnection* native = connection->dsp_connection;
		FMOD_ERR_CHECK_V(dsp->addInputPreallocated(target_dsp->dsp, &native), Ref<FmodDSPConnection>());
		connection->setup(native);
		return connection;
	}

	Dictionary FmodDSP::get_input(const int index) const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		ERR_FAIL_COND_V(index < 0, Dictionary());
		FMOD::DSP* dsp_ptr = nullptr;
		FMOD::DSPConnection* dsp_connection_ptr = nullptr;
		FMOD_ERR_CHECK_V(dsp->getInput(index, &dsp_ptr, &dsp_connection_ptr), Dictionary());

		Ref<FmodDSP> input_dsp;
		if (dsp_ptr) {
			input_dsp.instantiate();
			input_dsp->setup(dsp_ptr, false);
		}
		Ref<FmodDSPConnection> connection;
		if (dsp_connection_ptr) {
			connection.instantiate();
			connection->setup(dsp_connection_ptr);
		}

		Dictionary result;
		result["input"] = input_dsp;
		result["input_connection"] = connection;
		return result;
	}

	Dictionary FmodDSP::get_output(const int index) const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		ERR_FAIL_COND_V(index < 0, Dictionary());
		FMOD::DSP* dsp_ptr = nullptr;
		FMOD::DSPConnection* dsp_connection_ptr = nullptr;
		FMOD_ERR_CHECK_V(dsp->getOutput(index, &dsp_ptr, &dsp_connection_ptr), Dictionary());

		Ref<FmodDSP> output_dsp;
		if (dsp_ptr) {
			output_dsp.instantiate();
			output_dsp->setup(dsp_ptr, false);
		}
		Ref<FmodDSPConnection> connection;
		if (dsp_connection_ptr) {
			connection.instantiate();
			connection->setup(dsp_connection_ptr);
		}

		Dictionary result;
		result["output"] = output_dsp;
		result["output_connection"] = connection;
		return result;
	}

	int FmodDSP::get_num_inputs() const {
		ERR_FAIL_COND_V(!dsp, 0);
		int numinputs = 0;
		FMOD_ERR_CHECK_V(dsp->getNumInputs(&numinputs), 0);
		return numinputs;
	}

	int FmodDSP::get_num_outputs() const {
		ERR_FAIL_COND_V(!dsp, 0);
		int numoutputs = 0;
		FMOD_ERR_CHECK_V(dsp->getNumOutputs(&numoutputs), 0);
		return numoutputs;
	}

	void FmodDSP::disconnect_all(const bool inputs, const bool outputs) {
		ERR_FAIL_COND(!dsp);
		FMOD_ERR_CHECK(dsp->disconnectAll(inputs, outputs));
	}

	void FmodDSP::disconnect_from(Ref<FmodDSP> target, Ref<FmodDSPConnection> connection) {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND(target.is_null() || !target->dsp);
		FMOD::DSPConnection* connection_ptr = connection.is_valid() ? connection->dsp_connection : nullptr;
		FMOD_ERR_CHECK(dsp->disconnectFrom(target->dsp, connection_ptr));
	}

	void FmodDSP::set_input_channel_format(const int numchannels, FmodSystem::FmodSpeakerMode speakermode) {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND(numchannels <= 0);
		FMOD_SPEAKERMODE to = static_cast<FMOD_SPEAKERMODE>(speakermode);
		FMOD_ERR_CHECK(dsp->setInputChannelFormat(numchannels, to));
	}

	Dictionary FmodDSP::get_input_channel_format() const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		int numchannels = 0;
		FMOD_SPEAKERMODE speakermode = FMOD_SPEAKERMODE_DEFAULT;
		FMOD_ERR_CHECK_V(dsp->getInputChannelFormat(&numchannels, &speakermode), Dictionary());
		Dictionary result;
		result["num_channels"] = numchannels;
		result["speaker_mode"] = static_cast<int>(speakermode);
		return result;
	}

	Dictionary FmodDSP::get_output_channel_format(const int inchannels, FmodSystem::FmodSpeakerMode inspeakermode) {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		ERR_FAIL_COND_V(inchannels <= 0, Dictionary());
		FMOD_SPEAKERMODE to = static_cast<FMOD_SPEAKERMODE>(inspeakermode);
		int outchannels = 0;
		FMOD_SPEAKERMODE outspeakermode = FMOD_SPEAKERMODE_DEFAULT;
		FMOD_ERR_CHECK_V(dsp->getOutputChannelFormat(inchannels, to, &outchannels, &outspeakermode), Dictionary());
		Dictionary result;
		result["out_channels"] = outchannels;
		result["out_speaker_mode"] = static_cast<int>(outspeakermode);
		return result;
	}

	Dictionary FmodDSP::get_metering_info() const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		FMOD_DSP_METERING_INFO input_info = {};
		FMOD_DSP_METERING_INFO output_info = {};
		FMOD_ERR_CHECK_V(dsp->getMeteringInfo(&input_info, &output_info), Dictionary());

		// 辅助 lambda：将 FMOD_DSP_METERING_INFO 转换为 Dictionary
		auto metering_info_to_dict = [](const FMOD_DSP_METERING_INFO& metering) -> Dictionary {
			Dictionary dict;

			// 基本采样信息
			dict["num_samples"] = metering.numsamples;
			dict["num_channels"] = metering.numchannels;

			// 峰值电平 (线性值) - 每个通道一个值，最多32通道
			Array peak_levels;
			for (int i = 0; i < metering.numchannels && i < 32; i++) {
				peak_levels.push_back(metering.peaklevel[i]);
			}
			dict["peak_level"] = peak_levels;

			// 均方根电平/RMS电平 (线性值) - 每个通道一个值，最多32通道
			Array rms_levels;
			for (int i = 0; i < metering.numchannels && i < 32; i++) {
				rms_levels.push_back(metering.rmslevel[i]);
			}
			dict["rms_level"] = rms_levels;

			// 计算平均 RMS 和峰值（单声道混合）
			if (metering.numchannels > 0) {
				float avg_rms = 0.0f;
				float max_peak = 0.0f;
				for (int i = 0; i < metering.numchannels && i < 32; i++) {
					avg_rms += metering.rmslevel[i];
					max_peak = MAX(max_peak, metering.peaklevel[i]);
				}
				avg_rms /= metering.numchannels;

				dict["average_rms"] = avg_rms;
				dict["max_peak"] = max_peak;

				dict["average_rms_db"] = FmodUtils::linear_to_db(avg_rms);
				dict["max_peak_db"] = FmodUtils::linear_to_db(max_peak);
			}
			return dict;
			};

		Dictionary info;
		info["input"] = metering_info_to_dict(input_info);
		info["output"] = metering_info_to_dict(output_info);
		return info;
	}

	void FmodDSP::set_metering_enabled(const bool input_enabled, const bool output_enabled) {
		ERR_FAIL_COND(!dsp);
		FMOD_ERR_CHECK(dsp->setMeteringEnabled(input_enabled, output_enabled));
	}

	Dictionary FmodDSP::get_metering_enabled() const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		bool inputenabled = false, outputenabled = false;
		FMOD_ERR_CHECK_V(dsp->getMeteringEnabled(&inputenabled, &outputenabled), Dictionary());
		Dictionary result;
		result["input_enabled"] = inputenabled;
		result["output_enabled"] = outputenabled;
		return result;
	}

	int FmodDSP::get_data_parameter_index(FmodDSPParameterDataType data_type) const {
		ERR_FAIL_COND_V(!dsp, -1);
		int index = -1;
		FMOD_DSP_PARAMETER_DATA_TYPE fmod_data_type = static_cast<FMOD_DSP_PARAMETER_DATA_TYPE>(data_type);
		FMOD_ERR_CHECK_V(dsp->getDataParameterIndex(fmod_data_type, &index), -1);
		return index;
	}

	int FmodDSP::get_num_parameters() const {
		ERR_FAIL_COND_V(!dsp, 0);
		int numparams = 0;
		FMOD_ERR_CHECK_V(dsp->getNumParameters(&numparams), 0);
		return numparams;
	}

	void FmodDSP::set_parameter_bool(const int32_t index, const bool value) {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND(index < 0);
		FMOD_ERR_CHECK(dsp->setParameterBool(static_cast<int>(index), value));
	}

	bool FmodDSP::get_parameter_bool(const int32_t index) const {
		ERR_FAIL_COND_V(!dsp, false);
		ERR_FAIL_COND_V(index < 0, false);
		bool value = false;
		FMOD_ERR_CHECK_V(dsp->getParameterBool(index, &value, nullptr, 0), false);
		return value;
	}

	void FmodDSP::set_parameter_data(const int32_t index, const PackedByteArray& data) {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND(index < 0);
		ERR_FAIL_COND_MSG(data.size() > static_cast<int64_t>(std::numeric_limits<unsigned int>::max()), "DSP parameter data is too large for FMOD.");

		// 如果数据为空，FMOD 有时允许传 nullptr 和 0
		if (data.is_empty()) {
			FMOD_ERR_CHECK(dsp->setParameterData(index, nullptr, 0));
			return;
		}

		PackedByteArray fmod_data = data;
		FMOD_ERR_CHECK(dsp->setParameterData(
			static_cast<int>(index),
			fmod_data.ptrw(),
			static_cast<unsigned int>(fmod_data.size())
		));
	}

	PackedByteArray FmodDSP::get_parameter_data(const int32_t index) const {
		ERR_FAIL_COND_V(!dsp, PackedByteArray());
		ERR_FAIL_COND_V(index < 0, PackedByteArray());

		unsigned int size = 0;
		void* internal_fmod_ptr = nullptr; // 用于接收 FMOD 内部数据的指针

		// FMOD 返回的是内部数据指针，必须立即拷贝到 Godot 管理的数组。
		FMOD_ERR_CHECK_V(dsp->getParameterData(index, &internal_fmod_ptr, &size, nullptr, 0), PackedByteArray());

		if (size == 0 || internal_fmod_ptr == nullptr) {
			return PackedByteArray();
		}

		PackedByteArray data;
		data.resize(size);
		uint8_t* dst = data.ptrw();
		memcpy(dst, internal_fmod_ptr, size);

		return data;
	}

	void FmodDSP::set_parameter_float(int index, float value) {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND(index < 0);
		FMOD_ERR_CHECK(dsp->setParameterFloat(index, value));
	}

	float FmodDSP::get_parameter_float(int index) const {
		ERR_FAIL_COND_V(!dsp, 0.0f);
		ERR_FAIL_COND_V(index < 0, 0.0f);
		float value = 0.0f;
		FMOD_ERR_CHECK(dsp->getParameterFloat(index, &value, nullptr, 0));
		return value;
	}

	void FmodDSP::set_parameter_int(int index, int value) {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND(index < 0);
		FMOD_ERR_CHECK(dsp->setParameterInt(index, value));
	}

	int FmodDSP::get_parameter_int(int index) const {
		ERR_FAIL_COND_V(!dsp, 0);
		ERR_FAIL_COND_V(index < 0, 0);
		int value = 0;
		FMOD_ERR_CHECK(dsp->getParameterInt(index, &value, nullptr, 0));
		return value;
	}

	Dictionary FmodDSP::get_parameter_info(const int index) const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		ERR_FAIL_COND_V(index < 0, Dictionary());
		int numparams = 0;
		FMOD_ERR_CHECK_V(dsp->getNumParameters(&numparams), Dictionary());
		ERR_FAIL_INDEX_V(index, numparams, Dictionary());

		FMOD_DSP_PARAMETER_DESC* desc = nullptr;
		FMOD_RESULT result = dsp->getParameterInfo(index, &desc);
		if (result != FMOD_OK || desc == nullptr) {
			return Dictionary();
		}
		Dictionary info;
		info["type"] = static_cast<int>(desc->type);
		info["name"] = _fmod_fixed_string_to_godot(desc->name, sizeof(desc->name));
		info["label"] = _fmod_fixed_string_to_godot(desc->label, sizeof(desc->label));
		info["description"] = desc->description ? String::utf8(desc->description) : String();

		// 根据参数类型添加特定字段
		// 注意: FMOD_DSP_PARAMETER_DESC 使用联合体，必须根据 type 访问正确的成员
		switch (desc->type) {
		case FMOD_DSP_PARAMETER_TYPE_FLOAT: {
			info["min"] = desc->floatdesc.min;
			info["max"] = desc->floatdesc.max;
			info["default"] = desc->floatdesc.defaultval;

			Dictionary mapping;
			const auto mapping_type = desc->floatdesc.mapping_type;
			mapping["type"] = static_cast<int>(mapping_type);

			// 只有当 mapping type 是 PIECEWISE_LINEAR 时才访问 piecewiselinearmapping
			// 否则里面的指针可能是未初始化的垃圾值
			if (mapping_type == FMOD_DSP_PARAMETER_FLOAT_MAPPING_TYPE_PIECEWISE_LINEAR) {
				Dictionary piecewiselinearmapping;
				const int numpoints = desc->floatdesc.mapping_piecewiselinear_numpoints;
				const float* pointparamvalues = desc->floatdesc.mapping_piecewiselinear_pointparamvalues;
				const float* pointpositions = desc->floatdesc.mapping_piecewiselinear_pointpositions;
				piecewiselinearmapping["numpoints"] = numpoints;

				// 安全地访问指针，只有当 numpoints > 0 且指针不为 nullptr 时才访问
				// 添加上限检查防止垃圾值
				if (numpoints > 0 && numpoints <= 256) {
					if (pointparamvalues != nullptr && pointpositions != nullptr) {
						Array point_values;
						Array point_positions;
						for (int i = 0; i < numpoints; i++) {
							point_values.append(pointparamvalues[i]);
							point_positions.append(pointpositions[i]);
						}
						piecewiselinearmapping["point_values"] = point_values;
						piecewiselinearmapping["point_positions"] = point_positions;
					}
				}
				mapping["piecewiselinearmapping"] = piecewiselinearmapping;
			}

			info["mapping"] = mapping;
			break;
		}

		case FMOD_DSP_PARAMETER_TYPE_INT: {
			info["min"] = desc->intdesc.min;
			info["max"] = desc->intdesc.max;
			info["defaultval"] = desc->intdesc.defaultval;
			info["goes_to_infinity"] = desc->intdesc.goestoinf != 0;
			// 注意：某些 FMOD DSP 的 valuenames 指针可能是未初始化的垃圾值
			// 即使指针不为 nullptr，访问它也可能导致崩溃
			// 因此暂时禁用 valuenames 的导出以保证稳定性
			// 如果需要枚举值名称，建议查阅 FMOD 文档直接使用已知值
			break;
		}

		case FMOD_DSP_PARAMETER_TYPE_BOOL: {
			info["defaultval"] = desc->booldesc.defaultval != 0;
			// 注意：某些 FMOD DSP 的 valuenames 指针可能是未初始化的垃圾值
			// 即使指针不为 nullptr，访问它也可能导致崩溃
			// 因此暂时禁用 valuenames 的导出以保证稳定性
			break;
		}

		case FMOD_DSP_PARAMETER_TYPE_DATA: {
			info["datatype"] = desc->datadesc.datatype;
			break;
		}

		default: {
			// 未知的参数类型，不添加额外字段
			info["warning"] = "Unknown parameter type";
			break;
		}
		}
		return info;
	}

	void FmodDSP::set_active(const bool active) {
		ERR_FAIL_COND(!dsp);
		FMOD_ERR_CHECK(dsp->setActive(active));
	}

	bool FmodDSP::get_active() const {
		if (!dsp) return false;
		bool active = false;
		FMOD_ERR_CHECK_V(dsp->getActive(&active), false);
		return active;
	}

	void FmodDSP::set_bypass(const bool bypass) {
		ERR_FAIL_COND(!dsp);
		FMOD_ERR_CHECK(dsp->setBypass(bypass));
	}

	bool FmodDSP::get_bypass() const {
		if (!dsp) return false;
		bool bypass = false;
		FMOD_ERR_CHECK_V(dsp->getBypass(&bypass), false);
		return bypass;
	}

	void FmodDSP::set_wet_dry_mix(const float prewet, const float postwet, const float dry) {
		ERR_FAIL_COND(!dsp);
		FMOD_ERR_CHECK(dsp->setWetDryMix(prewet, postwet, dry));
	}

	Dictionary FmodDSP::get_wet_dry_mix() const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		float prewet = 0.0f, postwet = 0.0f, dry = 0.0f;
		FMOD_ERR_CHECK_V(dsp->getWetDryMix(&prewet, &postwet, &dry), Dictionary());
		Dictionary result;
		result["prewet"] = prewet;
		result["postwet"] = postwet;
		result["dry"] = dry;
		return result;
	}

	bool FmodDSP::get_idle() const {
		ERR_FAIL_COND_V(!dsp, false);
		bool idle = false;
		FMOD_ERR_CHECK_V(dsp->getIdle(&idle), false);
		return idle;
	}

	void FmodDSP::reset() {
		ERR_FAIL_COND(!dsp);
		FMOD_ERR_CHECK(dsp->reset());
	}

	void FmodDSP::release() {
		ERR_FAIL_COND(!dsp);
		ERR_FAIL_COND_MSG(!owns_dsp, "Cannot release a borrowed FMOD DSP");
		const bool had_callbacks = callbacks_set;
		_detach_from_dsp();
		FMOD_RESULT result = dsp->release();
		if (result != FMOD_OK) {
			ERR_PRINT("Failed to release DSP");
			FMOD_ERR_CHECK(dsp->setUserData(this));
			if (had_callbacks && dsp->setCallback(GD_FMOD_DSP_CALLBACK) == FMOD_OK) {
				callbacks_set = true;
			}
			return;
		}
		dsp = nullptr;
		owns_dsp = false;
		std::lock_guard<std::mutex> lock(getparam_data_mutex);
		getparam_data_storage.clear();
	}

	FmodDSP::FmodDSPType FmodDSP::get_type() const {
		ERR_FAIL_COND_V(!dsp, DSP_TYPE_UNKNOWN);
		FMOD_DSP_TYPE dsp_type = FMOD_DSP_TYPE_UNKNOWN;
		FMOD_ERR_CHECK_V(dsp->getType(&dsp_type), DSP_TYPE_UNKNOWN);
		FmodDSPType to = static_cast<FmodDSPType>(dsp_type);
		return to;
	}

	Dictionary FmodDSP::get_info() const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		char name[32] = { 0 };
		unsigned int version = 0;
		int channels = 0;
		FMOD_DSP_MODE mode = FMOD_DSP_MODE_FILTER;
		FMOD_ERR_CHECK_V(dsp->getInfo(name, &version, &channels, &mode), Dictionary());
		Dictionary info;
		info["channels"] = channels;
		info["mode"] = static_cast<int>(mode);
		info["name"] = String::utf8(name);
		info["version"] = static_cast<uint32_t>(version);
		return info;
	}

	Dictionary FmodDSP::get_cpu_usage() const {
		ERR_FAIL_COND_V(!dsp, Dictionary());
		unsigned int exclusive = 0, inclusive = 0;
		FMOD_ERR_CHECK_V(dsp->getCPUUsage(&exclusive, &inclusive), Dictionary());
		Dictionary usage;
		usage["exclusive"] = static_cast<uint32_t>(exclusive);
		usage["inclusive"] = static_cast<uint32_t>(inclusive);
		return usage;
	}

	Ref<FmodSystem> FmodDSP::get_system_object() const {
		ERR_FAIL_COND_V(!dsp, Ref<FmodSystem>());
		FMOD::System* system_ptr = nullptr;
		FMOD_ERR_CHECK_V(dsp->getSystemObject(&system_ptr), Ref<FmodSystem>());
		ERR_FAIL_NULL_V(system_ptr, Ref<FmodSystem>());

		Ref<FmodSystem> system;
		system.instantiate();
		system->setup_borrowed(system_ptr);
		return system;
	}

	// GDScript 回调设置方法实现
	void FmodDSP::set_create_callback(const Callable& p_callback) {
		_create_callback = p_callback;
	}

	Callable FmodDSP::get_create_callback() const {
		return _create_callback;
	}

	void FmodDSP::set_release_callback(const Callable& p_callback) {
		_release_callback = p_callback;
	}

	Callable FmodDSP::get_release_callback() const {
		return _release_callback;
	}

	void FmodDSP::set_reset_callback(const Callable& p_callback) {
		_reset_callback = p_callback;
	}

	Callable FmodDSP::get_reset_callback() const {
		return _reset_callback;
	}

	void FmodDSP::set_process_callback(const Callable& p_callback) {
		_process_callback = p_callback;
	}

	Callable FmodDSP::get_process_callback() const {
		return _process_callback;
	}

	void FmodDSP::set_setposition_callback(const Callable& p_callback) {
		_setposition_callback = p_callback;
	}

	Callable FmodDSP::get_setposition_callback() const {
		return _setposition_callback;
	}

	void FmodDSP::set_setparam_float_callback(const Callable& p_callback) {
		_setparam_float_callback = p_callback;
	}

	Callable FmodDSP::get_setparam_float_callback() const {
		return _setparam_float_callback;
	}

	void FmodDSP::set_setparam_int_callback(const Callable& p_callback) {
		_setparam_int_callback = p_callback;
	}

	Callable FmodDSP::get_setparam_int_callback() const {
		return _setparam_int_callback;
	}

	void FmodDSP::set_setparam_bool_callback(const Callable& p_callback) {
		_setparam_bool_callback = p_callback;
	}

	Callable FmodDSP::get_setparam_bool_callback() const {
		return _setparam_bool_callback;
	}

	void FmodDSP::set_setparam_data_callback(const Callable& p_callback) {
		_setparam_data_callback = p_callback;
	}

	Callable FmodDSP::get_setparam_data_callback() const {
		return _setparam_data_callback;
	}

	void FmodDSP::set_getparam_float_callback(const Callable& p_callback) {
		_getparam_float_callback = p_callback;
	}

	Callable FmodDSP::get_getparam_float_callback() const {
		return _getparam_float_callback;
	}

	void FmodDSP::set_getparam_int_callback(const Callable& p_callback) {
		_getparam_int_callback = p_callback;
	}

	Callable FmodDSP::get_getparam_int_callback() const {
		return _getparam_int_callback;
	}

	void FmodDSP::set_getparam_bool_callback(const Callable& p_callback) {
		_getparam_bool_callback = p_callback;
	}

	Callable FmodDSP::get_getparam_bool_callback() const {
		return _getparam_bool_callback;
	}

	void FmodDSP::set_getparam_data_callback(const Callable& p_callback) {
		_getparam_data_callback = p_callback;
	}

	Callable FmodDSP::get_getparam_data_callback() const {
		return _getparam_data_callback;
	}

	// 回调处理函数实现
	FMOD_RESULT FmodDSP::_handle_create(FMOD_DSP_STATE* dsp_state) {
		// 分配状态内存
		MyDSPState* state = static_cast<MyDSPState*>(FMOD_DSP_ALLOC(dsp_state, sizeof(MyDSPState)));
		if (!state) return FMOD_ERR_MEMORY;

		// 初始化默认值
		state->gain = 1.0f;
		state->bypass = false;
		state->sample_rate = 0;

		// 存储状态指针
		dsp_state->plugindata = state;

		if (_create_callback.is_valid()) {
			if (!_is_main_thread()) {
				_create_callback.call_deferred();
				return FMOD_OK;
			}
			const Variant ret = _create_callback.call();
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_release(FMOD_DSP_STATE* dsp_state) {
		FMOD_RESULT callback_result = FMOD_OK;
		if (_release_callback.is_valid()) {
			if (!_is_main_thread()) {
				_release_callback.call_deferred();
			}
			else {
				const Variant ret = _release_callback.call();
				if (ret.get_type() == Variant::INT) {
					callback_result = static_cast<FMOD_RESULT>(int(ret));
				}
			}
		}
		if (dsp_state && dsp_state->plugindata) {
			FMOD_DSP_FREE(dsp_state, dsp_state->plugindata);
			dsp_state->plugindata = nullptr;
		}
		return callback_result;
	}

	FMOD_RESULT FmodDSP::_handle_reset(FMOD_DSP_STATE* dsp_state) {
		if (_reset_callback.is_valid()) {
			if (!_is_main_thread()) {
				_reset_callback.call_deferred();
				return FMOD_OK;
			}
			const Variant ret = _reset_callback.call();
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_process(FMOD_DSP_STATE* state, unsigned int frames, const FMOD_DSP_BUFFER* input, const FMOD_DSP_BUFFER* output) {
		FMOD_RESULT status = fmod_dsp_passthrough(frames, input, output);
		if (status != FMOD_OK || !_process_callback.is_valid() || !_is_main_thread()) {
			return status;
		}
		const int input_channels = input ? input->numchannels : 0;
		PackedFloat32Array samples;
		if (input && input->buffer && input_channels > 0) {
			samples.resize(static_cast<int64_t>(frames) * input_channels);
			memcpy(samples.ptrw(), input->buffer, samples.size() * sizeof(float));
		}
		const Variant returned = _process_callback.call(samples, frames, input_channels, output->numchannels);
		if (returned.get_type() != Variant::DICTIONARY) {
			return FMOD_OK;
		}
		const Dictionary result = returned;
		if (result.has("output")) {
			ERR_FAIL_COND_V(result["output"].get_type() != Variant::PACKED_FLOAT32_ARRAY, FMOD_ERR_INVALID_PARAM);
			const PackedFloat32Array processed = result["output"];
			const int64_t capacity = static_cast<int64_t>(frames) * output->numchannels;
			const int64_t count = MIN(processed.size(), capacity);
			if (count > 0) {
				memcpy(output->buffer, processed.ptr(), count * sizeof(float));
			}
			memset(output->buffer + count, 0, (capacity - count) * sizeof(float));
		}
		if (result.has("cooldown")) {
			const int64_t cooldown = result["cooldown"];
			ERR_FAIL_COND_V(cooldown < 0 || cooldown > FMOD_DSP_COOLDOWN_INFINITE, FMOD_ERR_INVALID_PARAM);
			status = FMOD_DSP_SETCOOLDOWN(state, static_cast<unsigned int>(cooldown));
			if (status != FMOD_OK) return status;
		}
		if (result.has("bypass")) {
			status = FMOD_DSP_SETBYPASS(state, bool(result["bypass"]));
			if (status != FMOD_OK) return status;
		}
		if (result.has("silence")) {
			status = FMOD_DSP_SETSILENCE(state, bool(result["silence"]));
		}
		return status;
	}

	FMOD_RESULT FmodDSP::_handle_setposition(FMOD_DSP_STATE* dsp_state, unsigned int pos) {
		if (_setposition_callback.is_valid()) {
			if (!_is_main_thread()) {
				_setposition_callback.call_deferred(pos);
				return FMOD_OK;
			}
			const Variant ret = _setposition_callback.call(pos);
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_setparam_float(FMOD_DSP_STATE* dsp_state, int index, float value) {
		if (_setparam_float_callback.is_valid()) {
			if (!_is_main_thread()) {
				_setparam_float_callback.call_deferred(index, value);
				return FMOD_OK;
			}
			const Variant ret = _setparam_float_callback.call(index, value);
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_setparam_int(FMOD_DSP_STATE* dsp_state, int index, int value) {
		if (_setparam_int_callback.is_valid()) {
			if (!_is_main_thread()) {
				_setparam_int_callback.call_deferred(index, value);
				return FMOD_OK;
			}
			const Variant ret = _setparam_int_callback.call(index, value);
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_setparam_bool(FMOD_DSP_STATE* dsp_state, int index, FMOD_BOOL value) {
		if (_setparam_bool_callback.is_valid()) {
			if (!_is_main_thread()) {
				_setparam_bool_callback.call_deferred(index, value != 0);
				return FMOD_OK;
			}
			const Variant ret = _setparam_bool_callback.call(index, value != 0);
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_setparam_data(FMOD_DSP_STATE* dsp_state, int index, void* data, unsigned int length) {
		if (_setparam_data_callback.is_valid()) {
			PackedByteArray arr;
			if (length > 0) {
				ERR_FAIL_COND_V(!data, FMOD_ERR_INVALID_PARAM);
				arr.resize(length);
				memcpy(arr.ptrw(), data, length);
			}
			if (!_is_main_thread()) {
				_setparam_data_callback.call_deferred(index, arr);
				return FMOD_OK;
			}
			const Variant ret = _setparam_data_callback.call(index, arr);
			if (ret.get_type() == Variant::INT) {
				return static_cast<FMOD_RESULT>(int(ret));
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_getparam_float(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valuestr) {
		if (_getparam_float_callback.is_valid()) {
			if (!_is_main_thread()) {
				if (value) {
					*value = 0.0f;
				}
				if (valuestr) {
					valuestr[0] = '\0';
				}
				_getparam_float_callback.call_deferred(index);
				return FMOD_OK;
			}
			const Variant ret = _getparam_float_callback.call(index);
			if (ret.get_type() == Variant::DICTIONARY) {
				Dictionary result = ret;
				if (result.has("value")) {
					*value = result["value"];
				}
				if (result.has("valuestr")) {
					String str = result["valuestr"];
					_copy_fmod_valuestr(valuestr, str);
				}
				if (result.has("result")) {
					return static_cast<FMOD_RESULT>(int(result["result"]));
				}
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_getparam_int(FMOD_DSP_STATE* dsp_state, int index, int* value, char* valuestr) {
		if (_getparam_int_callback.is_valid()) {
			if (!_is_main_thread()) {
				if (value) {
					*value = 0;
				}
				if (valuestr) {
					valuestr[0] = '\0';
				}
				_getparam_int_callback.call_deferred(index);
				return FMOD_OK;
			}
			const Variant ret = _getparam_int_callback.call(index);
			if (ret.get_type() == Variant::DICTIONARY) {
				Dictionary result = ret;
				if (result.has("value")) {
					*value = result["value"];
				}
				if (result.has("valuestr")) {
					String str = result["valuestr"];
					_copy_fmod_valuestr(valuestr, str);
				}
				if (result.has("result")) {
					return static_cast<FMOD_RESULT>(int(result["result"]));
				}
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_getparam_bool(FMOD_DSP_STATE* dsp_state, int index, FMOD_BOOL* value, char* valuestr) {
		if (_getparam_bool_callback.is_valid()) {
			if (!_is_main_thread()) {
				if (value) {
					*value = false;
				}
				if (valuestr) {
					valuestr[0] = '\0';
				}
				_getparam_bool_callback.call_deferred(index);
				return FMOD_OK;
			}
			const Variant ret = _getparam_bool_callback.call(index);
			if (ret.get_type() == Variant::DICTIONARY) {
				Dictionary result = ret;
				if (result.has("value")) {
					*value = bool(result["value"]) ? 1 : 0;
				}
				if (result.has("valuestr")) {
					String str = result["valuestr"];
					_copy_fmod_valuestr(valuestr, str);
				}
				if (result.has("result")) {
					return static_cast<FMOD_RESULT>(int(result["result"]));
				}
			}
		}
		return FMOD_OK;
	}

	FMOD_RESULT FmodDSP::_handle_getparam_data(FMOD_DSP_STATE* dsp_state, int index, void** data, unsigned int* length, char* valuestr) {
		if (_getparam_data_callback.is_valid()) {
			if (!_is_main_thread()) {
				if (data) {
					*data = nullptr;
				}
				if (length) {
					*length = 0;
				}
				if (valuestr) {
					valuestr[0] = '\0';
				}
				_getparam_data_callback.call_deferred(index);
				return FMOD_OK;
			}
			const Variant ret = _getparam_data_callback.call(index);
			if (ret.get_type() == Variant::DICTIONARY) {
				Dictionary result = ret;
				if (result.has("data")) {
					PackedByteArray arr = result["data"];
					void* data_ptr = arr.is_empty() ? nullptr : arr.ptrw();
					if (data_ptr) {
						std::lock_guard<std::mutex> lock(getparam_data_mutex);
						getparam_data_storage[data_ptr] = arr;
					}
					if (length) {
						*length = static_cast<unsigned int>(arr.size());
					}
					// FMOD 会在 DATAPARAMETERRELEASE 回调中归还该指针
					if (data) {
						*data = data_ptr;
					}
				}
				if (result.has("valuestr")) {
					String str = result["valuestr"];
					_copy_fmod_valuestr(valuestr, str);
				}
				if (result.has("result")) {
					return static_cast<FMOD_RESULT>(int(result["result"]));
				}
			}
		}
		return FMOD_OK;
	}

	void FmodDSP::_release_getparam_data(void* p_data) {
		if (!p_data) {
			return;
		}

		std::lock_guard<std::mutex> lock(getparam_data_mutex);
		getparam_data_storage.erase(p_data);
	}

}

// Helper function to get FmodDSP from DSP state
godot::FmodDSP* get_dsp_from_state(FMOD_DSP_STATE* dsp_state) {
	if (!dsp_state) return nullptr;
	void* user_data = nullptr;
	FMOD_RESULT result = FMOD_DSP_GETUSERDATA(dsp_state, &user_data);
	if (result != FMOD_OK || !user_data) return nullptr;
	return static_cast<godot::FmodDSP*>(user_data);
}

FMOD_RESULT F_CALL GD_FMOD_DSP_CALLBACK(
	FMOD_DSP* dsp,
	FMOD_DSP_CALLBACK_TYPE type,
	void* data
) {
	// 通用回调分发
	if (!dsp) return FMOD_ERR_INVALID_PARAM;

	void* user_data = nullptr;
	FMOD::DSP* dsp_cpp = reinterpret_cast<FMOD::DSP*>(dsp);
	FMOD_RESULT result = dsp_cpp->getUserData(&user_data);
	if (result != FMOD_OK || !user_data) return FMOD_OK;

	godot::FmodDSP* fmod_dsp = static_cast<godot::FmodDSP*>(user_data);

	// FMOD_DSP_CALLBACK_TYPE 只包含 DATAPARAMETERRELEASE 类型
	// CREATE/RELEASE 等回调是通过 DSP 描述符中的独立函数指针设置的
	switch (type) {
	case FMOD_DSP_CALLBACK_DATAPARAMETERRELEASE:
		fmod_dsp->_release_getparam_data(data);
		return FMOD_OK;
	default:
		return FMOD_OK;
	}
}

FMOD_RESULT F_CALL GD_FMOD_DSP_CREATE_CALLBACK(FMOD_DSP_STATE* dsp_state) {
	// CREATE 回调在 DSP 创建时调用，此时 UserData 还未设置
	// 所以使用默认实现
	MyDSPState* state = static_cast<MyDSPState*>(FMOD_DSP_ALLOC(dsp_state, sizeof(MyDSPState)));
	if (!state) return FMOD_ERR_MEMORY;

	state->gain = 1.0f;
	state->bypass = false;
	state->sample_rate = 0;

	dsp_state->plugindata = state;
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_RELEASE_CALLBACK(FMOD_DSP_STATE* dsp_state) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_release(dsp_state);
	}
	// 默认清理
	if (dsp_state && dsp_state->plugindata) {
		FMOD_DSP_FREE(dsp_state, dsp_state->plugindata);
		dsp_state->plugindata = nullptr;
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_RESET_CALLBACK(
	FMOD_DSP_STATE* dsp_state
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_reset(dsp_state);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_PROCESS_CALLBACK(FMOD_DSP_STATE* state, unsigned int frames, const FMOD_DSP_BUFFER* input, const FMOD_DSP_BUFFER* output) {
	godot::FmodDSP* dsp = get_dsp_from_state(state);
	return dsp ? dsp->_handle_process(state, frames, input, output) : fmod_dsp_passthrough(frames, input, output);
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SETPOSITION_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	unsigned int pos
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_setposition(dsp_state, pos);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_FLOAT_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	float value
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_setparam_float(dsp_state, index, value);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_INT_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	int value
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_setparam_int(dsp_state, index, value);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_BOOL_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	FMOD_BOOL value
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_setparam_bool(dsp_state, index, value);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_DATA_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	void* data,
	unsigned int length
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_setparam_data(dsp_state, index, data, length);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_FLOAT_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	float* value,
	char* valuestr
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_getparam_float(dsp_state, index, value, valuestr);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_INT_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	int* value,
	char* valuestr
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_getparam_int(dsp_state, index, value, valuestr);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_BOOL_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	FMOD_BOOL* value,
	char* valuestr
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_getparam_bool(dsp_state, index, value, valuestr);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_DATA_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int index,
	void** data,
	unsigned int* length,
	char* valuestr
) {
	godot::FmodDSP* dsp = get_dsp_from_state(dsp_state);
	if (dsp) {
		return dsp->_handle_getparam_data(dsp_state, index, data, length, valuestr);
	}
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SYSTEM_REGISTER_CALLBACK(
	FMOD_DSP_STATE* dsp_state
) {
	// 系统级回调，不通过 FmodDSP 实例处理
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SYSTEM_DEREGISTER_CALLBACK(
	FMOD_DSP_STATE* dsp_state
) {
	// 系统级回调，不通过 FmodDSP 实例处理
	return FMOD_OK;
}

FMOD_RESULT F_CALL GD_FMOD_DSP_SYSTEM_MIX_CALLBACK(
	FMOD_DSP_STATE* dsp_state,
	int stage
) {
	// 系统级混音回调，不通过 FmodDSP 实例处理
	return FMOD_OK;
}
