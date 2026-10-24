#ifndef FMOD_DSP_H
#define FMOD_DSP_H

#include "core/fmod_system.h"
#include "dsp/fmod_dsp_connection.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include "dsp/fmod_dsp_helpers.h"

#include <mutex>
#include <unordered_map>

namespace godot {

	class FmodDSP : public RefCounted {
		GDCLASS(FmodDSP, RefCounted)

	private:
		bool owns_dsp = false;
		bool callbacks_set = false;                                                     // 记录是否设置了回调

		std::mutex getparam_data_mutex;
		std::unordered_map<void*, PackedByteArray> getparam_data_storage;

		void _detach_from_dsp();

		Callable _create_callback;
		Callable _release_callback;
		Callable _reset_callback;
		Callable _process_callback;
		Callable _setposition_callback;
		Callable _setparam_float_callback;
		Callable _setparam_int_callback;
		Callable _setparam_bool_callback;
		Callable _setparam_data_callback;
		Callable _getparam_float_callback;
		Callable _getparam_int_callback;
		Callable _getparam_bool_callback;
		Callable _getparam_data_callback;

	protected:
		static void _bind_methods();

	public:
		FmodDSP();
		~FmodDSP();

		enum FmodDSPMode {
			DSP_MODE_FILTER = FMOD_DSP_MODE_FILTER,
			DSP_MODE_GENERATOR = FMOD_DSP_MODE_GENERATOR,
		};

		static constexpr int64_t DSP_COOLDOWN_INFINITE = FMOD_DSP_COOLDOWN_INFINITE;

		enum FmodDSPType {
			DSP_TYPE_UNKNOWN,                                                           // 它是通过非 FMOD 插件创建的，目的不明
			DSP_TYPE_MIXER,                                                             // 不处理信号，作为一个单元纯粹用于混音输入
			DSP_TYPE_OSCILLATOR,                                                        // 生成正弦/方形/锯齿/三角形或噪声音调
			DSP_TYPE_LOWPASS,                                                           // 滤波器采用高质量共振低通滤波算法来输出声音，但占用更多CPU时间 (已弃用)
			DSP_TYPE_ITLOWPASS,                                                         // 滤波器使用脉冲追踪器中使用的共振低通滤波算法来处理声音，但截止范围有限 (0 到 8060Hz)
			DSP_TYPE_HIGHPASS,                                                          // 滤波器使用共振高通滤波算法来发声 (已弃用)
			DSP_TYPE_ECHO,                                                              // 在声音上产生回声，并以所需速率淡出
			DSP_TYPE_FADER,                                                             // 对单元的音量进行声像和缩放
			DSP_TYPE_FLANGE,                                                            // 在声音上产生法兰效应
			DSP_TYPE_DISTORTION,                                                        // 声音失真
			DSP_TYPE_NORMALIZE,                                                         // 将声音归一化或放大至一定水平
			DSP_TYPE_LIMITER,                                                           // 将声音限制在一定的音量范围内
			DSP_TYPE_PARAMEQ,                                                           // 衰减或放大选定的频率范围 (已弃用)
			DSP_TYPE_PITCHSHIFT,                                                        // 在不改变播放速度的情况下，弯曲声音的音高
			DSP_TYPE_CHORUS,                                                            // 在声音上产生合唱效果
			DSP_TYPE_ITECHO,                                                            // 在声音上产生回声，并以 Impulse Tracker 中所用的速率淡出
			DSP_TYPE_COMPRESSOR,                                                        // 动态压缩 (链接/非链接多通道，宽带)
			DSP_TYPE_SFXREVERB,                                                         // I3DL2 混响效果
			DSP_TYPE_LOWPASS_SIMPLE,                                                    // 滤波器采用无共振的简单低通效果，但具有灵活截止且速度快
			DSP_TYPE_DELAY,                                                             // 在声音的各个声道上产生不同的延迟
			DSP_TYPE_TREMOLO,                                                           // 在声音上产生颤音/切断音效果
			DSP_TYPE_SEND,                                                              // 将信号副本发送到 DSP 树中任意的返回 DSP
			DSP_TYPE_RETURN,                                                            // 接收来自多个发送 DSP 的信号
			DSP_TYPE_HIGHPASS_SIMPLE,                                                   // 滤波器通过简单的高通发声，没有共振，但截止非常灵活且速度快
			DSP_TYPE_PAN,                                                               // 可以将信号进行二维或三维声像，可能还会进行上混或下混
			DSP_TYPE_THREE_EQ,                                                          // 三段均衡器
			DSP_TYPE_FFT,                                                               // 分析信号并通过 getParameter 返回频谱信息
			DSP_TYPE_LOUDNESS_METER,                                                    // 分析信号的响度和真实峰值
			DSP_TYPE_CONVOLUTIONREVERB,                                                 // 卷积混响
			DSP_TYPE_CHANNELMIX,                                                        // 提供每个通道增益、输入信号的通道分组，同时设定输出信号的扬声器格式，以及可自定义输入到输出通道的路由
			DSP_TYPE_TRANSCEIVER,                                                       // "发送" 和 "接收" 可从最多 32 个不同的槽位中选择。它类似于发送/退回，但使用全局时隙而非退货作为目的地，它还有其他功能，多个收发器可以从一个信道接收信号，也可以多台收发器发送到一个信道，或者两者结合
			DSP_TYPE_OBJECTPAN,                                                         // 通过将输入信号传递到外部对象混频器来实现空间化
			DSP_TYPE_MULTIBAND_EQ,                                                      // 五段参数均衡器
			DSP_TYPE_MULTIBAND_DYNAMICS,                                                // (已弃用)

			DSP_TYPE_MAX,
			DSP_TYPE_FORCEINT = 65536
		};

		enum FmodDSPParameterDataType {
			DSP_PARAMETER_DATA_TYPE_USER = 0,
			DSP_PARAMETER_DATA_TYPE_OVERALLGAIN = -1,
			DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES = -2,
			DSP_PARAMETER_DATA_TYPE_SIDECHAIN = -3,
			DSP_PARAMETER_DATA_TYPE_FFT = -4,
			DSP_PARAMETER_DATA_TYPE_3DATTRIBUTES_MULTI = -5,
			DSP_PARAMETER_DATA_TYPE_ATTENUATION_RANGE = -6,
			DSP_PARAMETER_DATA_TYPE_DYNAMIC_RESPONSE = -7,
			DSP_PARAMETER_DATA_TYPE_FINITE_LENGTH = -8,
		};

		FMOD::DSP* dsp = nullptr;

		void setup(
			FMOD::DSP* p_dsp,
			bool p_owns_dsp = true
		);

		// 连接
		Ref<FmodDSPConnection> add_input(
			Ref<FmodDSP> target_dsp,
			unsigned int type = 0
		);                                                                              // 在这个对象上添加一个 DSP 单元作为输入
		Ref<FmodDSPConnection> add_input_preallocated(Ref<FmodDSP> target_dsp, Ref<FmodDSPConnection> connection);
		Dictionary get_input(const int index) const;                                    // 在输入列表中指定索引处获取 DSP 单元
		Dictionary get_output(const int index) const;                                   // 在输出列表中指定索引处检索 DSP 单元
		int get_num_inputs() const;                                                     // 获取输入列表中的 DSP 单元数量
		int get_num_outputs() const;                                                    // 获取输出列表中的 DSP 单元数量
		void disconnect_all(const bool inputs, const bool outputs);                     // 断开所有输入和/或输出
		void disconnect_from(
			Ref<FmodDSP> target,
			Ref<FmodDSPConnection> connection = Ref<FmodDSPConnection>()
		);                                                                              // 断开指定的输入 DSP

		// Channel 格式
		void set_input_channel_format(
			const int numchannels,
			FmodSystem::FmodSpeakerMode speakermode
		);                                                                              // 设置该 DSP 处理时接收的 PCM 输入格式
		Dictionary get_input_channel_format() const;                                          // 获取该 DSP 处理时接收的 PCM 输入格式
		Dictionary get_output_channel_format(
			const int inchannels,
			FmodSystem::FmodSpeakerMode inspeakermode
		);                                                                              // 根据指定的输入处理时，该 DSP 会获取输出格式

		// 计量
		Dictionary get_metering_info() const;                                           // 获取信号测光启用的测光信息
		void set_metering_enabled(const bool input_enabled, const bool output_enabled); // 设置输入和输出信号的测光状态
		Dictionary get_metering_enabled() const;                                        // 获取输入和输出信号的测光状态

		// 参数
		int get_data_parameter_index(FmodDSPParameterDataType data_type) const;         // 获取特定数据类型的第一个数据参数的索引
		int get_num_parameters() const;                                                 // 获取该单元暴露的参数数量
		void set_parameter_bool(const int32_t index, const bool value);                 // 通过索引设置布尔参数
		bool get_parameter_bool(const int32_t index) const;                             // 通过索引获取布尔参数
		void set_parameter_data(const int32_t index, const PackedByteArray& data);      // 通过索引设置二进制数据参数
		PackedByteArray get_parameter_data(const int32_t index) const;                  // 通过索引获取二进制数据参数
		void set_parameter_float(int index, float value);                               // 通过索引设置浮点参数
		float get_parameter_float(int index) const;                                     // 通过索引获取浮点参数
		void set_parameter_int(int index, int value);                                   // 通过索引设置整数参数
		int get_parameter_int(int index) const;                                         // 通过索引获取整数参数
		Dictionary get_parameter_info(const int index) const;                           // 获取指定参数的信息

		// 处理
		void set_active(const bool active);                                             // 设置处理的激活状态
		bool get_active() const;                                                        // 获取处理的激活状态

		void set_bypass(const bool bypass);                                             // 设置处理绕过状态
		bool get_bypass() const;                                                        // 获取处理绕过状态

		void set_wet_dry_mix(const float prewet, const float postwet, const float dry); // 设定湿信号和干信号组件的比例
		Dictionary get_wet_dry_mix() const;                                             // 获取湿信号和干信号组件的比例
		bool get_idle() const;                                                          // 获取空闲状态

		// 概述
		void reset();                                                                   // 重置 DSP 的内部状态，使其准备好接收新的输入信号
		void release();                                                                 // 释放 DSP 单元，释放该对象使用的内存
		FmodDSPType get_type() const;                                                   // 获取 FMOD 注册 DSP 单元的预定义类型
		Dictionary get_info() const;                                                    // 获取 DSP 单元的信息
		Dictionary get_cpu_usage() const;                                               // 获取该设备混合线程 CPU 使用率的统计数据
		Ref<FmodSystem> get_system_object() const;                                      // 获取父系统对象

		// DSP 回调设置
		void set_create_callback(const Callable &p_callback);
		Callable get_create_callback() const;

		void set_release_callback(const Callable &p_callback);
		Callable get_release_callback() const;

		void set_reset_callback(const Callable &p_callback);
		Callable get_reset_callback() const;


		void set_process_callback(const Callable &p_callback);
		Callable get_process_callback() const;

		void set_setposition_callback(const Callable &p_callback);
		Callable get_setposition_callback() const;

		void set_setparam_float_callback(const Callable &p_callback);
		Callable get_setparam_float_callback() const;

		void set_setparam_int_callback(const Callable &p_callback);
		Callable get_setparam_int_callback() const;

		void set_setparam_bool_callback(const Callable &p_callback);
		Callable get_setparam_bool_callback() const;

		void set_setparam_data_callback(const Callable &p_callback);
		Callable get_setparam_data_callback() const;

		void set_getparam_float_callback(const Callable &p_callback);
		Callable get_getparam_float_callback() const;

		void set_getparam_int_callback(const Callable &p_callback);
		Callable get_getparam_int_callback() const;

		void set_getparam_bool_callback(const Callable &p_callback);
		Callable get_getparam_bool_callback() const;

		void set_getparam_data_callback(const Callable &p_callback);
		Callable get_getparam_data_callback() const;


		// 内部处理函数
		FMOD_RESULT _handle_create(FMOD_DSP_STATE* dsp_state);
		FMOD_RESULT _handle_release(FMOD_DSP_STATE* dsp_state);
		FMOD_RESULT _handle_reset(FMOD_DSP_STATE* dsp_state);
		FMOD_RESULT _handle_process(FMOD_DSP_STATE* dsp_state, unsigned int length, const FMOD_DSP_BUFFER* input, const FMOD_DSP_BUFFER* output);
		FMOD_RESULT _handle_setposition(FMOD_DSP_STATE* dsp_state, unsigned int pos);
		FMOD_RESULT _handle_setparam_float(FMOD_DSP_STATE* dsp_state, int index, float value);
		FMOD_RESULT _handle_setparam_int(FMOD_DSP_STATE* dsp_state, int index, int value);
		FMOD_RESULT _handle_setparam_bool(FMOD_DSP_STATE* dsp_state, int index, FMOD_BOOL value);
		FMOD_RESULT _handle_setparam_data(FMOD_DSP_STATE* dsp_state, int index, void* data, unsigned int length);
		FMOD_RESULT _handle_getparam_float(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valuestr);
		FMOD_RESULT _handle_getparam_int(FMOD_DSP_STATE* dsp_state, int index, int* value, char* valuestr);
		FMOD_RESULT _handle_getparam_bool(FMOD_DSP_STATE* dsp_state, int index, FMOD_BOOL* value, char* valuestr);
		FMOD_RESULT _handle_getparam_data(FMOD_DSP_STATE* dsp_state, int index, void** data, unsigned int* length, char* valuestr);
		void _release_getparam_data(void* p_data);
	};
}

extern "C" {
	// 自定义 DSP 状态数据
	struct MyDSPState {
		float gain;									                                    // 增益值
		bool bypass;								                                    // 是否旁路
		float sample_rate;							                                    // 采样率
	};

	FMOD_RESULT F_CALL GD_FMOD_DSP_CALLBACK(
		FMOD_DSP* dsp,
		FMOD_DSP_CALLBACK_TYPE type,
		void* data
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_CREATE_CALLBACK(
		FMOD_DSP_STATE* dsp_state
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_RELEASE_CALLBACK(
		FMOD_DSP_STATE* dsp_state
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_RESET_CALLBACK(
		FMOD_DSP_STATE* dsp_state
	);


	FMOD_RESULT F_CALL GD_FMOD_DSP_PROCESS_CALLBACK(FMOD_DSP_STATE* dsp_state, unsigned int length, const FMOD_DSP_BUFFER* input, const FMOD_DSP_BUFFER* output);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SETPOSITION_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		unsigned int pos
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_FLOAT_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		float value
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_INT_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		int value
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_BOOL_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		FMOD_BOOL value
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SETPARAM_DATA_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		void* data,
		unsigned int length
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_FLOAT_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		float* value,
		char* valuestr
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_INT_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		int* value,
		char* valuestr
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_BOOL_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		FMOD_BOOL* value,
		char* valuestr
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_GETPARAM_DATA_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int index,
		void** data,
		unsigned int* length,
		char* valuestr
	);


	FMOD_RESULT F_CALL GD_FMOD_DSP_SYSTEM_REGISTER_CALLBACK(
		FMOD_DSP_STATE* dsp_state
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SYSTEM_DEREGISTER_CALLBACK(
		FMOD_DSP_STATE* dsp_state
	);

	FMOD_RESULT F_CALL GD_FMOD_DSP_SYSTEM_MIX_CALLBACK(
		FMOD_DSP_STATE* dsp_state,
		int stage
	);
}

VARIANT_ENUM_CAST(FmodDSP::FmodDSPType);
VARIANT_ENUM_CAST(FmodDSP::FmodDSPParameterDataType);

VARIANT_ENUM_CAST(FmodDSP::FmodDSPMode);

#endif // !FMOD_DSP_H
