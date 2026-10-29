#include "fmod_audio_effect_capture.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include <fmod_errors.h>
#include <cmath>
#include <string.h>

namespace godot {
	static constexpr uint32_t CAPTURE_DSP_USERDATA_MAGIC = 0x43415054;

	struct CaptureDSPUserData : public CaptureDSPState {
		uint32_t magic = CAPTURE_DSP_USERDATA_MAGIC;
	};

	static CaptureDSPUserData* _get_capture_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}

		CaptureDSPUserData* userdata = static_cast<CaptureDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == CAPTURE_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL capture_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}

		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}

		CaptureDSPUserData* capture_userdata = static_cast<CaptureDSPUserData*>(userdata);
		if (!capture_userdata || capture_userdata->magic != CAPTURE_DSP_USERDATA_MAGIC) {
			dsp_state->plugindata = nullptr;
			return FMOD_OK;
		}

		dsp_state->plugindata = capture_userdata;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL capture_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			CaptureDSPUserData* userdata = static_cast<CaptureDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == CAPTURE_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT capture_process(CaptureDSPState* state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {

		if (!inbufferarray || !outbufferarray) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (!inbufferarray->buffer || !outbufferarray->buffer) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int channels = inbufferarray->numchannels;
		float* in = static_cast<float*>(inbufferarray->buffer);
		float* out = static_cast<float*>(outbufferarray->buffer);
		if (!in || !out || channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (out != in) {
			memcpy(out, in, static_cast<size_t>(length) * static_cast<size_t>(channels) * sizeof(float));
		}

		if (!state || !state->ring_buffer || !state->buffer_mutex || !state->pushed_frames || !state->discarded_frames) {
			return FMOD_OK;
		}

		std::lock_guard<std::mutex> lock(*state->buffer_mutex);
		const size_t written = state->ring_buffer->write(in, length, channels);

		*state->pushed_frames += length;
		if (written < length) {
			*state->discarded_frames += length - written;
		}

		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL capture_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length, const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {
		return capture_process(_get_capture_userdata(dsp_state), length, inbufferarray, outbufferarray);
	}

	void FmodAudioEffectCapture::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_buffer_length", "buffer_length"), &FmodAudioEffectCapture::set_buffer_length);
		ClassDB::bind_method(D_METHOD("get_buffer_length"), &FmodAudioEffectCapture::get_buffer_length);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "buffer_length", PROPERTY_HINT_RANGE, "0.1,10.0,0.1,suffix:s"), "set_buffer_length", "get_buffer_length");

		ClassDB::bind_method(D_METHOD("can_get_buffer", "frames"), &FmodAudioEffectCapture::can_get_buffer);
		ClassDB::bind_method(D_METHOD("clear_buffer"), &FmodAudioEffectCapture::clear_buffer);
		ClassDB::bind_method(D_METHOD("get_buffer", "frames"), &FmodAudioEffectCapture::get_buffer);
		ClassDB::bind_method(D_METHOD("get_discarded_frames"), &FmodAudioEffectCapture::get_discarded_frames);
		ClassDB::bind_method(D_METHOD("get_frames_available"), &FmodAudioEffectCapture::get_frames_available);
		ClassDB::bind_method(D_METHOD("get_pushed_frames"), &FmodAudioEffectCapture::get_pushed_frames);
	}

	FmodAudioEffectCapture::FmodAudioEffectCapture() {
		_update_buffer_size();
	}

	FmodAudioEffectCapture::~FmodAudioEffectCapture() {
		// Stop callbacks before the derived class's buffer and mutex are destroyed.
		if (bus.is_valid()) {
			remove_from_bus(bus);
		}
	}

	void FmodAudioEffectCapture::_update_buffer_size() {
		// Preserve the existing 48 kHz sizing policy, expressed in complete frames.
		buffer_size = static_cast<int>(CLAMP(buffer_length * 48000.0, 512.0, 480000.0));

		std::lock_guard<std::mutex> lock(buffer_mutex);
		ring_buffer.resize(static_cast<size_t>(buffer_size));
	}

	void FmodAudioEffectCapture::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		// 如果已经应用过，先移除
		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		// 获取主系统
		Ref<FmodSystem> system = FmodServer::get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		// 创建自定义 DSP
		Ref<FmodDSP> dsp = create_custom_dsp(system);
		if (!dsp.is_valid()) {
			ERR_PRINT("FmodAudioEffectCapture: Failed to create custom DSP");
			return;
		}

		// 检查 DSP 内部指针
		if (!dsp->dsp) {
			ERR_PRINT("FmodAudioEffectCapture: DSP internal pointer is null after setup");
			return;
		}

		// 添加到 DSP 链
		dsp_chain.push_back(dsp);

		// 添加到总线（-1 表示添加到末尾）
		bus->add_dsp(-1, dsp);

		dsp_created = true;
	}

	void FmodAudioEffectCapture::remove_from_bus(Ref<FmodChannelGroup> p_bus) {
		// 调用基类实现
		FmodAudioEffect::remove_from_bus(p_bus);

		// 清空缓冲区统计
		pushed_frames.store(0);
		discarded_frames.store(0);
		dsp_created = false;
	}

	// 重写基类虚函数 - DSP 创建
	void FmodAudioEffectCapture::_on_dsp_create(FMOD_DSP_STATE* dsp_state) {
		// 分配捕获特定的状态
		CaptureDSPState* state = new CaptureDSPState();
		state->ring_buffer = &ring_buffer;
		state->buffer_mutex = &buffer_mutex;
		state->pushed_frames = &pushed_frames;
		state->discarded_frames = &discarded_frames;
		state->channels = 2;														// 默认立体声

		// 存储到 DSP 状态
		dsp_state->plugindata = state;
	}

	// 重写基类虚函数 - DSP 处理
	void FmodAudioEffectCapture::_on_dsp_process(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {
		CaptureDSPState* state = dsp_state ? static_cast<CaptureDSPState*>(dsp_state->plugindata) : nullptr;
		capture_process(state, length, inbufferarray, outbufferarray);
	}

	// 重写基类虚函数 - DSP 释放
	void FmodAudioEffectCapture::_on_dsp_release(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state->plugindata) {
			CaptureDSPState* state = static_cast<CaptureDSPState*>(dsp_state->plugindata);
			delete state;
			dsp_state->plugindata = nullptr;
		}
	}

	bool FmodAudioEffectCapture::can_get_buffer(const int64_t frames) const {
		if (frames < 0) return false;
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return ring_buffer.data_left() >= static_cast<uint64_t>(frames);
	}

	void FmodAudioEffectCapture::clear_buffer() {
		{
			std::lock_guard<std::mutex> lock(buffer_mutex);
			ring_buffer.clear();
			pushed_frames.store(0);
			discarded_frames.store(0);
		}
		emit_changed();
	}

	PackedVector2Array FmodAudioEffectCapture::get_buffer(const int64_t frames) {
		PackedVector2Array result;
		if (frames <= 0) return result;
		size_t to_read;
		{
			std::lock_guard<std::mutex> lock(buffer_mutex);
			to_read = static_cast<size_t>(std::min(static_cast<uint64_t>(frames), static_cast<uint64_t>(ring_buffer.data_left())));
		}
		if (to_read == 0) return result;
		// Allocation and packed-array setup must not hold up the audio callback.
		ERR_FAIL_COND_V(result.resize(static_cast<int>(to_read)) != OK, result);
		Vector2* destination = result.ptrw();
		size_t read;
		{
			std::lock_guard<std::mutex> lock(buffer_mutex);
			// Another reader, clear or resize may have changed availability.
			read = ring_buffer.read(destination, to_read);
		}
		if (read < to_read) result.resize(static_cast<int>(read));
		return result;
	}

	int64_t FmodAudioEffectCapture::get_discarded_frames() const {
		return discarded_frames.load();
	}

	int64_t FmodAudioEffectCapture::get_frames_available() const {
		std::lock_guard<std::mutex> lock(buffer_mutex);
		return static_cast<int64_t>(ring_buffer.data_left());
	}

	int64_t FmodAudioEffectCapture::get_pushed_frames() const {
		return pushed_frames.load();
	}

	void FmodAudioEffectCapture::set_buffer_length(double p_buffer_length) {
		ERR_FAIL_COND_MSG(!std::isfinite(p_buffer_length) || p_buffer_length <= 0, "Buffer length must be finite and positive");
		buffer_length = p_buffer_length;
		_update_buffer_size();
		emit_changed();
	}

	double FmodAudioEffectCapture::get_buffer_length() const {
		return buffer_length;
	}

	Ref<FmodDSP> FmodAudioEffectCapture::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());

		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, get_dsp_description(), sizeof(FMOD_DSP_DESCRIPTION));

		CaptureDSPUserData* dsp_userdata = new CaptureDSPUserData();
		dsp_userdata->ring_buffer = &ring_buffer;
		dsp_userdata->buffer_mutex = &buffer_mutex;
		dsp_userdata->pushed_frames = &pushed_frames;
		dsp_userdata->discarded_frames = &discarded_frames;
		dsp_userdata->channels = 2;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectCapture::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;

		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotCaptureDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = capture_dsp_create_callback;
			desc.release = capture_dsp_release_callback;
			desc.process = capture_dsp_process_callback;
			initialized = true;
		}

		return &desc;
	}
}
