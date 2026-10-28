#include "fmod_dsp_connection.h"
#include "fmod_dsp.h"

namespace godot {
	void FmodDSPConnection::_bind_methods() {
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_STANDARD);
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_SIDECHAIN);
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_SEND);
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_SEND_SIDECHAIN);
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_PREALLOCATED);
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_MAX);
		BIND_ENUM_CONSTANT(DSPCONNECTION_TYPE_FORCEINT);

		ClassDB::bind_method(D_METHOD("set_mix", "volume_db"), &FmodDSPConnection::set_mix);
		ClassDB::bind_method(D_METHOD("get_mix"), &FmodDSPConnection::get_mix);

		ClassDB::bind_method(D_METHOD("set_mix_matrix", "matrix", "outchannels", "inchannels", "inchannel_hop"), &FmodDSPConnection::set_mix_matrix, DEFVAL(0));
		ClassDB::bind_method(D_METHOD("get_mix_matrix", "outchannels", "inchannels", "inchannel_hop"), &FmodDSPConnection::get_mix_matrix, DEFVAL(0));

		ClassDB::bind_method(D_METHOD("get_input"), &FmodDSPConnection::get_input);
		ClassDB::bind_method(D_METHOD("get_output"), &FmodDSPConnection::get_output);
		ClassDB::bind_method(D_METHOD("get_type"), &FmodDSPConnection::get_type);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mix"), "set_mix", "get_mix");
	}

	FmodDSPConnection::FmodDSPConnection() {

	}

	FmodDSPConnection::~FmodDSPConnection() {
		dsp_connection = nullptr;
	}

	void FmodDSPConnection::setup(FMOD::DSPConnection* p_connection) {
		ERR_FAIL_COND_MSG(!p_connection, "DSPConnection pointer is null");

		dsp_connection = p_connection;
	}

	void FmodDSPConnection::set_mix(const float volume_db) {
		ERR_FAIL_COND(!dsp_connection);
		FMOD_ERR_CHECK(dsp_connection->setMix(static_cast<float>(FmodUtils::dbfs_to_sample(volume_db))));
	}

	float FmodDSPConnection::get_mix() const {
		if (!dsp_connection) return 0.0f;
		float volume = 0.0f;
		FMOD_ERR_CHECK(dsp_connection->getMix(&volume));
		return static_cast<float>(FmodUtils::sample_to_dbfs(volume));
	}

	void FmodDSPConnection::set_mix_matrix(
		const PackedFloat32Array& matrix,
		const int outchannels,
		const int inchannels,
		const int inchannel_hop
	) {
		ERR_FAIL_COND(!dsp_connection);
		ERR_FAIL_COND_MSG(outchannels <= 0 || outchannels > FMOD_MAX_CHANNEL_WIDTH, "outchannels must be between 1 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_MSG(inchannels <= 0 || inchannels > FMOD_MAX_CHANNEL_WIDTH, "inchannels must be between 1 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_MSG(inchannel_hop < 0 || inchannel_hop > FMOD_MAX_CHANNEL_WIDTH, "inchannel_hop must be between 0 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_MSG(inchannel_hop > 0 && inchannel_hop < inchannels, "inchannel_hop must be 0 or >= inchannels");

		// 先限制通道与步长范围，再计算大小，避免有符号整数乘法溢出
		const int effective_hop = inchannel_hop > 0 ? inchannel_hop : inchannels;
		const int expected_size = (outchannels - 1) * effective_hop + inchannels;

		ERR_FAIL_COND_MSG(matrix.size() < expected_size,
			vformat("Matrix size (%d) is smaller than expected (%d)", matrix.size(), expected_size));

		// FMOD 的接口没有 const 限定；传入独立副本以避免修改调用方的数组
		PackedFloat32Array fmod_matrix = matrix;

		// 调用 FMOD API
		FMOD_ERR_CHECK(dsp_connection->setMixMatrix(
			fmod_matrix.ptrw(),
			outchannels,
			inchannels,
			inchannel_hop
		));
	}

	PackedFloat32Array FmodDSPConnection::get_mix_matrix(
		const int outchannels,
		const int inchannels,
		const int inchannel_hop
	) const {
		ERR_FAIL_COND_V_MSG(!dsp_connection, PackedFloat32Array(), "DSPConnection is null");
		ERR_FAIL_COND_V_MSG(outchannels <= 0 || outchannels > FMOD_MAX_CHANNEL_WIDTH, PackedFloat32Array(), "outchannels must be between 1 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_V_MSG(inchannels <= 0 || inchannels > FMOD_MAX_CHANNEL_WIDTH, PackedFloat32Array(), "inchannels must be between 1 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_V_MSG(inchannel_hop < 0 || inchannel_hop > FMOD_MAX_CHANNEL_WIDTH, PackedFloat32Array(), "inchannel_hop must be between 0 and FMOD_MAX_CHANNEL_WIDTH");
		ERR_FAIL_COND_V_MSG(inchannel_hop > 0 && inchannel_hop < inchannels, PackedFloat32Array(), "inchannel_hop must be 0 or >= inchannels");

		// FMOD 返回实际行列数，不把传入值当作容量限制
		// 固定最大尺寸也避免了先查询尺寸、后读取期间的矩阵变化
		float buffer[FMOD_MAX_CHANNEL_WIDTH * FMOD_MAX_CHANNEL_WIDTH] = {};
		int actual_outchannels = 0;
		int actual_inchannels = 0;

		FMOD_ERR_CHECK_V(dsp_connection->getMixMatrix(
			buffer,
			&actual_outchannels,
			&actual_inchannels,
			FMOD_MAX_CHANNEL_WIDTH
		), PackedFloat32Array());
		ERR_FAIL_COND_V_MSG(actual_outchannels < 0 || actual_outchannels > FMOD_MAX_CHANNEL_WIDTH || actual_inchannels < 0 || actual_inchannels > FMOD_MAX_CHANNEL_WIDTH, PackedFloat32Array(), "FMOD returned invalid matrix dimensions");
		ERR_FAIL_COND_V_MSG(actual_outchannels > outchannels || actual_inchannels > inchannels, PackedFloat32Array(), "Requested matrix dimensions are smaller than the FMOD matrix");

		const int effective_hop = inchannel_hop > 0 ? inchannel_hop : inchannels;
		PackedFloat32Array matrix;
		ERR_FAIL_COND_V(matrix.resize(outchannels * effective_hop) != OK, PackedFloat32Array());
		matrix.fill(0.0f);
		float* output = matrix.ptrw();
		for (int out = 0; out < actual_outchannels; out++) {
			for (int in = 0; in < actual_inchannels; in++) {
				output[out * effective_hop + in] = buffer[out * FMOD_MAX_CHANNEL_WIDTH + in];
			}
		}

		return matrix;
	}

	Ref<FmodDSP> FmodDSPConnection::get_input() const {
		ERR_FAIL_COND_V(!dsp_connection, Ref<FmodDSP>());
		FMOD::DSP* dsp_ptr = nullptr;
		FMOD_ERR_CHECK_V(dsp_connection->getInput(&dsp_ptr), Ref<FmodDSP>());
		Ref<FmodDSP> dsp;
		dsp.instantiate();
		dsp->setup(dsp_ptr, false);
		return dsp;
	}

	Ref<FmodDSP> FmodDSPConnection::get_output() const {
		ERR_FAIL_COND_V(!dsp_connection, Ref<FmodDSP>());
		FMOD::DSP* dsp_ptr = nullptr;
		FMOD_ERR_CHECK_V(dsp_connection->getOutput(&dsp_ptr), Ref<FmodDSP>());
		Ref<FmodDSP> dsp;
		dsp.instantiate();
		dsp->setup(dsp_ptr, false);
		return dsp;
	}

	FmodDSPConnection::FmodDSPConnectionType FmodDSPConnection::get_type() const {
		ERR_FAIL_COND_V(!dsp_connection, DSPCONNECTION_TYPE_STANDARD);
		FMOD_DSPCONNECTION_TYPE connection_type = FMOD_DSPCONNECTION_TYPE_STANDARD;
		FMOD_ERR_CHECK_V(dsp_connection->getType(&connection_type), DSPCONNECTION_TYPE_STANDARD);
		FmodDSPConnectionType to = static_cast<FmodDSPConnectionType>(connection_type);
		return to;
	}
}
