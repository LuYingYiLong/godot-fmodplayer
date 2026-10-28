#include "fmod_channel_control.h"

namespace godot {
	void FmodChannelControl::_bind_methods() {
		ClassDB::bind_method(D_METHOD("channel_control_is_valid"), &FmodChannelControl::channel_control_is_valid);
		ClassDB::bind_method(D_METHOD("channel_control_is_null"), &FmodChannelControl::channel_control_is_null);

		ClassDB::bind_method(D_METHOD("is_playing"), &FmodChannelControl::is_playing);
		ClassDB::bind_method(D_METHOD("stop"), &FmodChannelControl::stop);

		ClassDB::bind_method(D_METHOD("set_paused", "paused"), &FmodChannelControl::set_paused);
		ClassDB::bind_method(D_METHOD("get_paused"), &FmodChannelControl::get_paused);

		ClassDB::bind_method(D_METHOD("set_mode", "mode"), &FmodChannelControl::set_mode);
		ClassDB::bind_method(D_METHOD("get_mode"), &FmodChannelControl::get_mode);

		ClassDB::bind_method(D_METHOD("set_pitch", "pitch"), &FmodChannelControl::set_pitch);
		ClassDB::bind_method(D_METHOD("get_pitch"), &FmodChannelControl::get_pitch);

		ClassDB::bind_method(D_METHOD("get_audibility"), &FmodChannelControl::get_audibility);

		ClassDB::bind_method(D_METHOD("set_volume_db", "volume"), &FmodChannelControl::set_volume_db);
		ClassDB::bind_method(D_METHOD("get_volume_db"), &FmodChannelControl::get_volume_db);

		ClassDB::bind_method(D_METHOD("set_volume_ramp", "volume_ramp"), &FmodChannelControl::set_volume_ramp);
		ClassDB::bind_method(D_METHOD("get_volume_ramp"), &FmodChannelControl::get_volume_ramp);

		ClassDB::bind_method(D_METHOD("set_mute", "mute"), &FmodChannelControl::set_mute);
		ClassDB::bind_method(D_METHOD("get_mute"), &FmodChannelControl::get_mute);

		ClassDB::bind_method(D_METHOD("set_3d_attributes", "pos", "vel"), &FmodChannelControl::set_3d_attributes);
		ClassDB::bind_method(D_METHOD("get_3d_attributes"), &FmodChannelControl::get_3d_attributes);
		ClassDB::bind_method(D_METHOD("set_3d_cone_orientation", "orientation"), &FmodChannelControl::set_3d_cone_orientation);
		ClassDB::bind_method(D_METHOD("get_3d_cone_orientation"), &FmodChannelControl::get_3d_cone_orientation);
		ClassDB::bind_method(D_METHOD("set_3d_cone_settings", "inside_cone_angle", "outside_cone_angle", "outside_volume_db"), &FmodChannelControl::set_3d_cone_settings);
		ClassDB::bind_method(D_METHOD("get_3d_cone_settings"), &FmodChannelControl::get_3d_cone_settings);

		ClassDB::bind_method(D_METHOD("set_3d_custom_rolloff", "3d_custom_rolloff"), &FmodChannelControl::set_3d_custom_rolloff);
		ClassDB::bind_method(D_METHOD("get_3d_custom_rolloff"), &FmodChannelControl::get_3d_custom_rolloff);

		ClassDB::bind_method(D_METHOD("set_3d_distance_filter", "custom", "custom_level", "center_freq"), &FmodChannelControl::set_3d_distance_filter);
		ClassDB::bind_method(D_METHOD("get_3d_distance_filter"), &FmodChannelControl::get_3d_distance_filter);

		ClassDB::bind_method(D_METHOD("set_3d_doppler_level", "level"), &FmodChannelControl::set_3d_doppler_level);
		ClassDB::bind_method(D_METHOD("get_3d_doppler_level"), &FmodChannelControl::get_3d_doppler_level);

		ClassDB::bind_method(D_METHOD("set_3d_level", "level"), &FmodChannelControl::set_3d_level);
		ClassDB::bind_method(D_METHOD("get_3d_level"), &FmodChannelControl::get_3d_level);
		
		ClassDB::bind_method(D_METHOD("set_3d_min_distance", "min_distance"), &FmodChannelControl::set_3d_min_distance);
		ClassDB::bind_method(D_METHOD("get_3d_min_distance"), &FmodChannelControl::get_3d_min_distance);
		
		ClassDB::bind_method(D_METHOD("set_3d_max_distance", "max_distance"), &FmodChannelControl::set_3d_max_distance);
		ClassDB::bind_method(D_METHOD("get_3d_max_distance"), &FmodChannelControl::get_3d_max_distance);
		
		ClassDB::bind_method(D_METHOD("set_3d_occlusion", "direct_occlusion", "reverb_occlusion"), &FmodChannelControl::set_3d_occlusion);
		ClassDB::bind_method(D_METHOD("get_3d_occlusion"), &FmodChannelControl::get_3d_occlusion);

		ClassDB::bind_method(D_METHOD("set_3d_spread", "angle"), &FmodChannelControl::set_3d_spread);
		ClassDB::bind_method(D_METHOD("get_3d_spread"), &FmodChannelControl::get_3d_spread);

		ClassDB::bind_method(D_METHOD("set_pan", "pan"), &FmodChannelControl::set_pan);
		ClassDB::bind_method(D_METHOD("set_mix_levels_input", "levels"), &FmodChannelControl::set_mix_levels_input);
		ClassDB::bind_method(D_METHOD("set_mix_levels_output", "front_left", "front_right", "center", "lfe", "surround_left", "surround_right", "back_left", "back_right"), &FmodChannelControl::set_mix_levels_output);
		ClassDB::bind_method(D_METHOD("set_mix_matrix", "matrix", "outchannels", "inchannels", "inchannel_hop"), &FmodChannelControl::set_mix_matrix, DEFVAL(0));
		ClassDB::bind_method(D_METHOD("get_mix_matrix"), &FmodChannelControl::get_mix_matrix);

		ClassDB::bind_method(D_METHOD("set_reverb_properties", "instance", "wet"), &FmodChannelControl::set_reverb_properties);
		ClassDB::bind_method(D_METHOD("get_reverb_properties", "instance"), &FmodChannelControl::get_reverb_properties);
		ClassDB::bind_method(D_METHOD("set_low_pass_gain", "gain"), &FmodChannelControl::set_low_pass_gain);
		ClassDB::bind_method(D_METHOD("get_low_pass_gain"), &FmodChannelControl::get_low_pass_gain);

		ClassDB::bind_method(D_METHOD("add_dsp", "index", "dsp"), &FmodChannelControl::add_dsp);
		ClassDB::bind_method(D_METHOD("remove_dsp", "dsp"), &FmodChannelControl::remove_dsp);
		ClassDB::bind_method(D_METHOD("get_num_dsps"), &FmodChannelControl::get_num_dsps);
		ClassDB::bind_method(D_METHOD("get_dsp", "index"), &FmodChannelControl::get_dsp);
		ClassDB::bind_method(D_METHOD("set_dsp_index", "dsp", "index"), &FmodChannelControl::set_dsp_index);
		ClassDB::bind_method(D_METHOD("get_dsp_index", "dsp"), &FmodChannelControl::get_dsp_index);

		ClassDB::bind_method(D_METHOD("get_dsp_clock"), &FmodChannelControl::get_dsp_clock);
		ClassDB::bind_method(D_METHOD("set_delay", "start", "end", "stop_channels"), &FmodChannelControl::set_delay, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("get_delay"), &FmodChannelControl::get_delay);
		ClassDB::bind_method(D_METHOD("add_fade_point", "dspclock", "volume"), &FmodChannelControl::add_fade_point);
		ClassDB::bind_method(D_METHOD("set_fade_point_ramp", "dspclock", "volume"), &FmodChannelControl::set_fade_point_ramp);
		ClassDB::bind_method(D_METHOD("remove_fade_points", "start", "end"), &FmodChannelControl::remove_fade_points);
		ClassDB::bind_method(D_METHOD("get_fade_points"), &FmodChannelControl::get_fade_points);

		ClassDB::bind_method(D_METHOD("set_callback"), &FmodChannelControl::set_callback);
		ClassDB::bind_method(D_METHOD("clear_callback"), &FmodChannelControl::clear_callback);

		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused"), "set_paused", "get_paused");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch"), "set_pitch", "get_pitch");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.1,suffix:dB"), "set_volume_db", "get_volume_db");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "volume_ramp"), "set_volume_ramp", "get_volume_ramp");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mute"), "set_mute", "get_mute");
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "3d_custom_rolloff"), "set_3d_custom_rolloff", "get_3d_custom_rolloff");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "3d_doppler_level", PROPERTY_HINT_RANGE, "0.00,5.00,0.01"), "set_3d_doppler_level", "get_3d_doppler_level");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "3d_level", PROPERTY_HINT_RANGE, "0.0,1.0,0.1"), "set_3d_level", "get_3d_level");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "3d_min_distance", PROPERTY_HINT_RANGE, "1,100000,1"), "set_3d_min_distance", "get_3d_min_distance");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "3d_max_distance", PROPERTY_HINT_RANGE, "1,100000,1"), "set_3d_max_distance", "get_3d_max_distance");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "3d_spread", PROPERTY_HINT_RANGE, "0,360,0.1"), "set_3d_spread", "get_3d_spread");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0.00,1.00,0.01"), "set_low_pass_gain", "get_low_pass_gain");

		ADD_SIGNAL(MethodInfo("callback_received", PropertyInfo(Variant::INT, "type")));
	}

	FmodChannelControl::FmodChannelControl() {
		channel_control = nullptr;
	}

	FmodChannelControl::~FmodChannelControl() {
		if (channel_control) {
			channel_control->setCallback(nullptr);
			channel_control->setUserData(nullptr);
			channel_control = nullptr;
		}
	}

	void FmodChannelControl::_setup_control(FMOD::ChannelControl* control) {
		if (channel_control && channel_control != control) {
			channel_control->setCallback(nullptr);
			channel_control->setUserData(nullptr);
		}

		channel_control = control;
		if (channel_control) {
			channel_control->setUserData(this);
		}
	}

	bool FmodChannelControl::channel_control_is_valid() const {
		return channel_control != nullptr;
	}

	bool FmodChannelControl::channel_control_is_null() const {
		return channel_control == nullptr;
	}

	bool FmodChannelControl::is_playing() const {
		if (!channel_control) return false;
		bool isplaying = false;
		FMOD_RESULT result = channel_control->isPlaying(&isplaying);
		// FMOD_ERR_INVALID_HANDLE / FMOD_ERR_CHANNEL_STOLEN 表示 channel 已停止、失效或被 FMOD 复用，这是正常情况
		if (result == FMOD_ERR_INVALID_HANDLE || result == FMOD_ERR_CHANNEL_STOLEN) {
			return false;
		}
		FMOD_ERR_CHECK(result);
		return isplaying;
	}

	void FmodChannelControl::stop() {
		if (!channel_control) return;
		FMOD_RESULT result = channel_control->stop();
		if (result == FMOD_ERR_INVALID_HANDLE || result == FMOD_ERR_CHANNEL_STOLEN) {
			return;
		}
		FMOD_ERR_CHECK(result);
	}

	void FmodChannelControl::set_paused(bool paused) {
		if (!channel_control) return;
		FMOD_ERR_CHECK(channel_control->setPaused(paused));
	}

	bool FmodChannelControl::get_paused() const {
		if (!channel_control) return true;
		bool paused = true;
		FMOD_ERR_CHECK(channel_control->getPaused(&paused));
		return paused;
	}

	void FmodChannelControl::set_mode(FmodSystem::FmodMode mode) {
		ERR_FAIL_COND(!channel_control);
		FMOD_MODE fmod_mode = static_cast<FMOD_MODE>(mode);
		FMOD_ERR_CHECK(channel_control->setMode(fmod_mode));
	}

	FmodSystem::FmodMode FmodChannelControl::get_mode() const {
		if (!channel_control) return FmodSystem::FMOD_MODE_DEFAULT;
		FMOD_MODE fmod_mode;
		FMOD_ERR_CHECK(channel_control->getMode(&fmod_mode));
		FmodSystem::FmodMode mode = static_cast<FmodSystem::FmodMode>(fmod_mode);
		return mode;
	}

	void FmodChannelControl::set_pitch(float pitch) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setPitch(pitch));
	}

	float FmodChannelControl::get_pitch() const {
		if (!channel_control) return 1.0f;
		float pitch = 1.0f;
		FMOD_ERR_CHECK(channel_control->getPitch(&pitch));
		return pitch;
	}

	float FmodChannelControl::get_audibility() const {
		ERR_FAIL_COND_V(!channel_control, 0.0f);
		float audibility = 0.0f;
		FMOD_ERR_CHECK(channel_control->getAudibility(&audibility));
		return audibility;
	}

	void FmodChannelControl::set_volume_db(float volume) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setVolume(FmodUtils::dbfs_to_sample(volume)));
	}

	float FmodChannelControl::get_volume_db() const {
		if (!channel_control) return 0.0f;
		float volume = 0.0f;
		FMOD_ERR_CHECK(channel_control->getVolume(&volume));
		return FmodUtils::sample_to_dbfs(volume);
	}

	void FmodChannelControl::set_volume_ramp(const bool ramp) {
		if (!channel_control) return;
		FMOD_ERR_CHECK(channel_control->setVolumeRamp(ramp));
	}

	bool FmodChannelControl::get_volume_ramp() const {
		if (!channel_control) return false;
		bool ramp = false;
		FMOD_ERR_CHECK(channel_control->getVolumeRamp(&ramp));
		return ramp;
	}

	void FmodChannelControl::set_mute(const bool mute) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setMute(mute));
	}

	bool FmodChannelControl::get_mute() const {
		if (!channel_control) return false;
		bool mute = false;
		FMOD_ERR_CHECK(channel_control->getMute(&mute));
		return mute;
	}

	void FmodChannelControl::set_3d_attributes(
		const Vector3 pos,
		const Vector3 vel
	) {
		ERR_FAIL_COND(!channel_control);
		FMOD_VECTOR fmod_pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR fmod_vel = { vel.x, vel.y, vel.z };
		FMOD_ERR_CHECK(channel_control->set3DAttributes(&fmod_pos, &fmod_vel));
	}

	Dictionary FmodChannelControl::get_3d_attributes() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());
		FMOD_VECTOR fmod_pos = {};
		FMOD_VECTOR fmod_vel = {};
		FMOD_ERR_CHECK_V(channel_control->get3DAttributes(&fmod_pos, &fmod_vel), Dictionary());
		Dictionary result;
		result["pos"] = Vector3(fmod_pos.x, fmod_pos.y, fmod_pos.z);
		result["vel"] = Vector3(fmod_vel.x, fmod_vel.y, fmod_vel.z);
		return result;
	}

	void FmodChannelControl::set_3d_cone_orientation(const Vector3 orientation) {
		ERR_FAIL_COND(!channel_control);
		FMOD_VECTOR fmod_orientation = { orientation.x, orientation.y, orientation.z };
		FMOD_ERR_CHECK(channel_control->set3DConeOrientation(&fmod_orientation));
	}

	Vector3 FmodChannelControl::get_3d_cone_orientation() const {
		ERR_FAIL_COND_V(!channel_control, Vector3());
		FMOD_VECTOR fmod_orientation = {};
		FMOD_ERR_CHECK_V(channel_control->get3DConeOrientation(&fmod_orientation), Vector3());
		return Vector3(fmod_orientation.x, fmod_orientation.y, fmod_orientation.z);
	}

	void FmodChannelControl::set_3d_cone_settings(
		const float inside_cone_angle,
		const float outside_cone_angle,
		const float outside_volume_db
	) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->set3DConeSettings(inside_cone_angle, outside_cone_angle, FmodUtils::db_to_linear(outside_volume_db)));
	}

	Dictionary FmodChannelControl::get_3d_cone_settings() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());
		float inside_cone_angle = 360.0f, outside_cone_angle = 360.f, outside_volume = 1.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DConeSettings(&inside_cone_angle, &outside_cone_angle, &outside_volume), Dictionary());
		Dictionary result;
		result["inside_cone_angle"] = inside_cone_angle;
		result["outside_cone_angle"] = outside_cone_angle;
		result["outside_volume_db"] = FmodUtils::linear_to_db(outside_volume);
		return result;
	}

	void FmodChannelControl::set_3d_custom_rolloff(const PackedVector3Array points) {
		ERR_FAIL_COND(!channel_control);

		const int num_points = points.size();
		custom_rolloff_points.clear();
		if (num_points == 0) {
			// 传入空数组表示清除自定义 rolloff
			FMOD_ERR_CHECK(channel_control->set3DCustomRolloff(nullptr, 0));
			return;
		}

		// 使用 std::vector 管理内存，自动释放
		custom_rolloff_points.reserve(num_points);

		for (int i = 0; i < num_points; i++) {
			const Vector3& p = points[i];
			custom_rolloff_points.push_back({ p.x, p.y, p.z });
		}

		// 传递数组首地址（连续内存）
		FMOD_ERR_CHECK(channel_control->set3DCustomRolloff(custom_rolloff_points.data(), num_points));
	}

	PackedVector3Array FmodChannelControl::get_3d_custom_rolloff() const {
		if (!channel_control) return PackedVector3Array();
		FMOD_VECTOR* fmod_points = nullptr;
		int numpoints = 0;
		FMOD_ERR_CHECK_V(channel_control->get3DCustomRolloff(&fmod_points, &numpoints), PackedVector3Array());
		PackedVector3Array points;
		for (int index = 0; index < numpoints; index++) {
			Vector3 point = Vector3(fmod_points[index].x, fmod_points[index].y, fmod_points[index].z);
			points.append(point);
		}
		return points;
	}

	void FmodChannelControl::set_3d_distance_filter(const bool custom, const float custom_level, const float center_freq) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->set3DDistanceFilter(custom, custom_level, center_freq));
	}

	Dictionary FmodChannelControl::get_3d_distance_filter() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());
		bool custom = false;
		float custom_level = 1.0f, center_freq = 1500.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DDistanceFilter(&custom, &custom_level, &center_freq), Dictionary());
		Dictionary result;
		result["custom"] = custom;
		result["custom_level"] = custom_level;
		result["center_freq"] = center_freq;
		return result;
	}

	void FmodChannelControl::set_3d_doppler_level(const float level) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->set3DDopplerLevel(level));
	}

	float FmodChannelControl::get_3d_doppler_level() const {
		if (!channel_control) return 1.0f;
		float level = 1.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DDopplerLevel(&level), 1.0f);
		return level;
	}

	void FmodChannelControl::set_3d_level(const float level) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->set3DLevel(level));
	}

	float FmodChannelControl::get_3d_level() const {
		if (!channel_control) return 1.0f;
		float level = 1.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DLevel(&level), 1.0f);
		return level;
	}

	void FmodChannelControl::set_3d_min_distance(const float min) {
		ERR_FAIL_COND(!channel_control);
		float min_distance = 1.0f, max_distance = 100000.0f;
		FMOD_ERR_CHECK(channel_control->get3DMinMaxDistance(&min_distance, &max_distance));
		min_distance = min;
		FMOD_ERR_CHECK(channel_control->set3DMinMaxDistance(min_distance, max_distance));
	}

	float FmodChannelControl::get_3d_min_distance() const {
		if (!channel_control) return 1.0f;
		float min_distance = 1.0f, max_distance = 100000.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DMinMaxDistance(&min_distance, &max_distance), 1.0f);
		return min_distance;
	}

	void FmodChannelControl::set_3d_max_distance(const float max) {
		ERR_FAIL_COND(!channel_control);
		float min_distance = 1.0f, max_distance = 100000.0f;
		FMOD_ERR_CHECK(channel_control->get3DMinMaxDistance(&min_distance, &max_distance));
		max_distance = max;
		FMOD_ERR_CHECK(channel_control->set3DMinMaxDistance(min_distance, max_distance));
	}

	float FmodChannelControl::get_3d_max_distance() const {
		if (!channel_control) return 100000.0f;
		float min_distance = 1.0f, max_distance = 100000.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DMinMaxDistance(&min_distance, &max_distance), 100000.0f);
		return max_distance;
	}

	void FmodChannelControl::set_3d_occlusion(const float direct_occlusion, const float reverb_occlusion) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->set3DOcclusion(direct_occlusion, reverb_occlusion));
	}

	Dictionary FmodChannelControl::get_3d_occlusion() const {
		if (!channel_control) return Dictionary();
		float direct_occlusion = 0.0f, reverb_occlusion = 0.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DOcclusion(&direct_occlusion, &reverb_occlusion), Dictionary());
		Dictionary result;
		result["direct_occlusion"] = direct_occlusion;
		result["reverb_occlusion"] = reverb_occlusion;
		return result;
	}

	void FmodChannelControl::set_3d_spread(const float angle) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->set3DSpread(angle));
	}

	float FmodChannelControl::get_3d_spread() const {
		if (!channel_control) return 0.0f;
		float angle = 0.0f;
		FMOD_ERR_CHECK_V(channel_control->get3DSpread(&angle), 0.0f);
		return angle;
	}

	void FmodChannelControl::set_pan(const float pan) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setPan(pan));
	}

	void FmodChannelControl::set_mix_levels_input(const PackedFloat32Array& levels) {
		ERR_FAIL_COND(!channel_control);
		ERR_FAIL_COND_MSG(levels.is_empty(), "Input levels cannot be empty.");

		const int num_levels = levels.size();
		std::vector<float> fmod_levels;
		fmod_levels.reserve(num_levels);
		for (int i = 0; i < num_levels; i++) {
			const float level = levels[i];
			fmod_levels.push_back(level);
		}

		FMOD_ERR_CHECK(channel_control->setMixLevelsInput(fmod_levels.data(), num_levels));
	}

	void FmodChannelControl::set_mix_levels_output(
		const float front_left,
		const float front_right,
		const float center,
		const float lfe,
		const float surround_left,
		const float surround_right,
		const float back_left,
		const float back_right
	) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setMixLevelsOutput(
			front_left,
			front_right,
			center,
			lfe,
			surround_left,
			surround_right,
			back_left,
			back_right
		));
	}

	void FmodChannelControl::set_mix_matrix(
		const PackedFloat32Array& matrix,
		const int outchannels,
		const int inchannels,
		const int inchannel_hop
	) {
		ERR_FAIL_COND(!channel_control);
		ERR_FAIL_COND_MSG(matrix.is_empty(), "Matrix data is empty");
		ERR_FAIL_COND_MSG(outchannels <= 0 || outchannels > FMOD_MAX_CHANNEL_WIDTH, "outchannels must be between 1 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_MSG(inchannels <= 0 || inchannels > FMOD_MAX_CHANNEL_WIDTH, "inchannels must be between 1 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_MSG(inchannel_hop < 0 || inchannel_hop > FMOD_MAX_CHANNEL_WIDTH, "inchannel_hop must be between 0 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_MSG(inchannel_hop > 0 && inchannel_hop < inchannels, "inchannel_hop must be 0 or >= inchannels");

		const int effective_hop = inchannel_hop > 0 ? inchannel_hop : inchannels;
		const int expected_size = (outchannels - 1) * effective_hop + inchannels;
		ERR_FAIL_COND_MSG(matrix.size() < expected_size,
			vformat("Matrix size (%d) is smaller than expected (%d)", matrix.size(), expected_size));

		PackedFloat32Array fmod_matrix = matrix;
		FMOD_ERR_CHECK(channel_control->setMixMatrix(
			fmod_matrix.ptrw(),
			outchannels,
			inchannels,
			inchannel_hop
		));
	}

	Dictionary FmodChannelControl::get_mix_matrix() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());

		int outchannels = FMOD_MAX_CHANNEL_WIDTH;
		int inchannels = FMOD_MAX_CHANNEL_WIDTH;
		const int inchannel_hop = FMOD_MAX_CHANNEL_WIDTH;

		PackedFloat32Array buffer;
		buffer.resize(outchannels * inchannel_hop);

		FMOD_ERR_CHECK_V(channel_control->getMixMatrix(
			buffer.ptrw(),
			&outchannels,
			&inchannels,
			inchannel_hop
		), Dictionary());
		ERR_FAIL_COND_V_MSG(outchannels < 0 || outchannels > FMOD_MAX_CHANNEL_WIDTH || inchannels < 0 || inchannels > FMOD_MAX_CHANNEL_WIDTH, Dictionary(), "FMOD returned invalid matrix dimensions");

		PackedFloat32Array matrix;
		matrix.resize(outchannels * inchannels);
		for (int out = 0; out < outchannels; out++) {
			for (int in = 0; in < inchannels; in++) {
				matrix[out * inchannels + in] = buffer[out * inchannel_hop + in];
			}
		}

		Dictionary result;
		result["matrix"] = matrix;
		result["outchannels"] = outchannels;
		result["inchannels"] = inchannels;
		result["inchannel_hop"] = inchannels;
		return result;
	}

	void FmodChannelControl::set_reverb_properties(const int instance, const float wet) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setReverbProperties(instance, wet));
	}

	float FmodChannelControl::get_reverb_properties(const int instance) const {
		ERR_FAIL_COND_V(!channel_control, 0.0f);
		float wet = 0.0f;
		FMOD_ERR_CHECK_V(channel_control->getReverbProperties(instance, &wet), 0.0f);
		return wet;
	}

	void FmodChannelControl::set_low_pass_gain(const float gain) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setLowPassGain(gain));
	}

	float FmodChannelControl::get_low_pass_gain() const {
		if (!channel_control) return 1.0f;
		float gain = 1.0f;
		FMOD_ERR_CHECK_V(channel_control->getLowPassGain(&gain), 1.0f);
		return gain;
	}

	void FmodChannelControl::add_dsp(const int index, Ref<FmodDSP> dsp) {
		ERR_FAIL_COND(!channel_control);
		ERR_FAIL_COND_MSG(dsp.is_null(), "DSP is null");
		ERR_FAIL_COND_MSG(!dsp->dsp, "DSP internal pointer is null");
		FMOD_RESULT result = channel_control->addDSP(index, dsp->dsp);
		if (result != FMOD_OK) {
			ERR_PRINT(vformat("Failed to add DSP: %s", FMOD_ErrorString(result)));
		}
	}

	void FmodChannelControl::remove_dsp(Ref<FmodDSP> dsp) {
		ERR_FAIL_COND(!channel_control);
		ERR_FAIL_COND_MSG(dsp.is_null(), "DSP is null");
		ERR_FAIL_COND_MSG(!dsp->dsp, "DSP internal pointer is null");
		FMOD_ERR_CHECK(channel_control->removeDSP(dsp->dsp));
	}

	int FmodChannelControl::get_num_dsps() const {
		ERR_FAIL_COND_V(!channel_control, 0);
		int numdsps = 0;
		FMOD_ERR_CHECK(channel_control->getNumDSPs(&numdsps));
		return numdsps;
	}

	Ref<FmodDSP> FmodChannelControl::get_dsp(const int index) const {
		ERR_FAIL_COND_V(!channel_control, Ref<FmodDSP>());
		ERR_FAIL_COND_V_MSG(index < 0, Ref<FmodDSP>(), "DSP index must be >= 0.");

		FMOD::DSP* dsp_ptr = nullptr;
		FMOD_ERR_CHECK_V(channel_control->getDSP(index, &dsp_ptr), Ref<FmodDSP>());

		if (!dsp_ptr) return Ref<FmodDSP>();

		// 该 DSP 由 ChannelControl 持有，返回的包装器不得释放它
		Ref<FmodDSP> dsp;
		dsp.instantiate();
		dsp->setup(dsp_ptr, false);

		return dsp;
	}

	void FmodChannelControl::set_dsp_index(Ref<FmodDSP> dsp, const int index) {
		ERR_FAIL_COND(!channel_control);
		ERR_FAIL_COND_MSG(dsp.is_null() || !dsp->dsp, "DSP is invalid.");
		FMOD_ERR_CHECK(channel_control->setDSPIndex(dsp->dsp, index));
	}

	int FmodChannelControl::get_dsp_index(Ref<FmodDSP> dsp) const {
		ERR_FAIL_COND_V(!channel_control, -1);
		ERR_FAIL_COND_V_MSG(dsp.is_null() || !dsp->dsp, -1, "DSP is invalid.");
		int index = -1;
		FMOD_ERR_CHECK(channel_control->getDSPIndex(dsp->dsp, &index));
		return index;
	}

	Dictionary FmodChannelControl::get_dsp_clock() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());
		unsigned long long dspclock = 0, parentclock = 0;
		FMOD_ERR_CHECK_V(channel_control->getDSPClock(&dspclock, &parentclock), Dictionary());
		Dictionary result;
		result["dsp_lock"] = static_cast<uint64_t>(dspclock);
		result["parent_clock"] = static_cast<uint64_t>(parentclock);
		return result;
	}

	void FmodChannelControl::set_delay(
		const uint64_t start,
		const uint64_t end,
		const bool stopchannels
	) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setDelay(start, end, stopchannels));
	}

	Dictionary FmodChannelControl::get_delay() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());
		unsigned long long start = 0, end = 0;
		bool stopchannels = true;
		FMOD_ERR_CHECK_V(channel_control->getDelay(&start, &end, &stopchannels), Dictionary());
		Dictionary result;
		result["start"] = static_cast<uint64_t>(start);
		result["end"] = static_cast<uint64_t>(end);
		result["stop_channels"] = stopchannels;
		return result;
	}

	void FmodChannelControl::add_fade_point(const uint64_t dspclock, const float volume) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->addFadePoint(dspclock, volume));
	}

	void FmodChannelControl::set_fade_point_ramp(const uint64_t dspclock, const float volume) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->setFadePointRamp(dspclock, volume));
	}

	void FmodChannelControl::remove_fade_points(const uint64_t start, const uint64_t end) {
		ERR_FAIL_COND(!channel_control);
		FMOD_ERR_CHECK(channel_control->removeFadePoints(start, end));
	}

	Dictionary FmodChannelControl::get_fade_points() const {
		ERR_FAIL_COND_V(!channel_control, Dictionary());
		unsigned int numpoints = 0;
		FMOD_ERR_CHECK_V(channel_control->getFadePoints(&numpoints, nullptr, nullptr), Dictionary());

		PackedInt64Array point_dspclocks;
		PackedFloat32Array point_volumes;
		point_dspclocks.resize(numpoints);
		point_volumes.resize(numpoints);

		if (numpoints > 0) {
			std::vector<unsigned long long> dspclocks;
			std::vector<float> volumes;
			dspclocks.resize(numpoints);
			volumes.resize(numpoints);
			FMOD_ERR_CHECK_V(channel_control->getFadePoints(&numpoints, dspclocks.data(), volumes.data()), Dictionary());
			point_dspclocks.resize(numpoints);
			point_volumes.resize(numpoints);
			for (unsigned int i = 0; i < numpoints; i++) {
				point_dspclocks.set(i, static_cast<int64_t>(dspclocks[i]));
				point_volumes.set(i, volumes[i]);
			}
		}

		Dictionary result;
		result["num_points"] = static_cast<uint32_t>(numpoints);
		result["point_dspclocks"] = point_dspclocks;
		result["point_volumes"] = point_volumes;
		return result;
	}

	void FmodChannelControl::set_callback() {
		ERR_FAIL_COND_MSG(!channel_control, "ChannelControl is null");
		channel_control->setUserData(this);
		FMOD_ERR_CHECK(channel_control->setCallback(fmod_channel_control_callback));
	}

	void FmodChannelControl::clear_callback() {
		if (channel_control) {
			channel_control->setCallback(nullptr);
			channel_control->setUserData(nullptr);
		}
	}

	void FmodChannelControl::_on_callback(
		FMOD_CHANNELCONTROL_TYPE controltype,
		FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
		void* commanddata1,
		void* commanddata2
	) {
		call_deferred("emit_signal", StringName("callback_received"), static_cast<int>(callbacktype));
	}
}

FMOD_RESULT F_CALL fmod_channel_control_callback(
	FMOD_CHANNELCONTROL* channelcontrol,
	FMOD_CHANNELCONTROL_TYPE controltype,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
	void* commanddata1,
	void* commanddata2
) {
	// 获取 userdata（指向 FmodChannelControl 对象）
	void* userdata = nullptr;
	FMOD_RESULT result = reinterpret_cast<FMOD::ChannelControl*>(channelcontrol)->getUserData(&userdata);

	if (result != FMOD_OK || !userdata) {
		return FMOD_OK;
	}

	// 转换为基类指针
	godot::FmodChannelControl* control = static_cast<godot::FmodChannelControl*>(userdata);

	// 调用虚函数，让子类处理具体逻辑
	control->_on_callback(controltype, callbacktype, commanddata1, commanddata2);

	return FMOD_OK;
}
