#ifndef FMOD_CHANNEL_CONTROL_H
#define FMOD_CHANNEL_CONTROL_H

#include "dsp/fmod_dsp.h"
#include "core/fmod_system.h"
#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>

namespace godot {
	class FmodChannelControl : public RefCounted {
		GDCLASS(FmodChannelControl, RefCounted)

	protected:
		FMOD::ChannelControl* channel_control = nullptr;
		std::vector<FMOD_VECTOR> custom_rolloff_points;

		static void _bind_methods();
		void _setup_control(FMOD::ChannelControl* control);

	public:
		FmodChannelControl();
		virtual ~FmodChannelControl();

		// 有效性检查
		bool channel_control_is_valid() const;						// 检查底层 FMOD::ChannelControl 对象是否有效
		bool channel_control_is_null() const;						// 检查底层 FMOD::ChannelControl 对象是否无效

		// 播放控制
		bool is_playing() const;									// 检查播放状态
		void stop();												// 停止该频道 (或嵌套 ChannelGroups 中的所有频道) 的播放

		void set_paused(const bool paused);							// 设置暂停状态
		bool get_paused() const;									// 获取暂停状态

		void set_mode(FmodSystem::FmodMode mode);					// 设置播放模式，控制该对象的行为
		FmodSystem::FmodMode get_mode() const;						// 获取控制该对象行为的播放模式

		void set_pitch(float pitch);								// 设置相对音高/播放速率
		float get_pitch() const;									// 获取相对音高/播放速率

		// 音量控制
		float get_audibility() const;								// 根据所有影响最终输出音量的衰减因子计算出可听度

		void set_volume_db(const float volume);						// 设置音量
		float get_volume_db() const;								// 获取音量

		void set_volume_ramp(const bool ramp);						// 设置音量变化是逐步增加还是瞬时变化
		bool get_volume_ramp() const;								// 检索音量变化是逐步减少还是瞬时变化

		void set_mute(const bool mute);								// 设置静音状态
		bool get_mute() const;										// 检索静音状态

		// 空间化
		void set_3d_attributes(
			const Vector3 pos,
			const Vector3 vel
		);															// 设置用于应用平移、衰减和多普勒的三维位置和速度
		Dictionary get_3d_attributes() const;						// 获取用于应用平移、衰减和多普勒的三维位置和速度
		void set_3d_cone_orientation(const Vector3 orientation);	// 设置三维锥体形状的方向，用于模拟遮挡
		Vector3 get_3d_cone_orientation() const;					// 获取三维锥体形状的方向，用于模拟遮挡
		void set_3d_cone_settings(
			const float inside_cone_angle,
			const float outside_cone_angle,
			const float outside_volume_db
		);															// 设置基于方向的三维锥形模拟遮挡的角度和衰减水平
		Dictionary get_3d_cone_settings() const;					// 获取基于方向的三维锥形模拟遮挡的角度和衰减水平
		void set_3d_custom_rolloff(const PackedVector3Array points);// 设置一个自定义的三维距离衰减滚出形状
		PackedVector3Array get_3d_custom_rolloff() const;			// 获取一个自定义的三维距离衰减滚出形状
		void set_flat_3d_rolloff(); // 内部播放器使用的恒定曲线，不注册为脚本方法
		void set_3d_distance_filter(
			const bool custom,
			const float custom_level,
			const float center_freq
		);															// 设置三维距离滤波器覆盖值
		Dictionary get_3d_distance_filter() const;					// 获取三维距离滤波器覆盖值

		void set_3d_doppler_level(const float level);				// 设置多普勒的缩放量 (仅支持 Channel)
		float get_3d_doppler_level() const;							// 获取多普勒的缩放量 (仅支持 Channel)

		void set_3d_level(const float level);						// 设置 3D 平移和 2D 平移之间的混合
		float get_3d_level() const;									// 获取 3D 平移和 2D 平移之间的混合

		void set_3d_min_distance(const float min);					// 设置三维滚落衰减的最小距离
		float get_3d_min_distance() const;							// 获取三维滚落衰减的最小距离

		void set_3d_max_distance(const float max);					// 设置三维滚落衰减的最大距离
		float get_3d_max_distance() const;							// 获取三维滚落衰减的最大距离

		void set_3d_occlusion(
			const float direct_occlusion,
			const float reverb_occlusion
		);															// 设置直接和混响路径的三维衰减因子
		Dictionary get_3d_occlusion() const;						// 获取直接和混响路径的三维衰减因子
		
		void set_3d_spread(const float angle);						// 设置 3D 声音在扬声器空间中的扩散
		float get_3d_spread() const;								// 获取 3D 声音在扬声器空间中的扩散

		// 声像与水平调整
		void set_pan(const float pan);								// 调节左右声像水平
		void set_mix_levels_input(const PackedFloat32Array& levels);// 为多声道信号的每个通道设置输入音量电平 (仅支持 Channel)
		void set_mix_levels_output(
			const float front_left,
			const float front_right,
			const float center,
			const float lfe,
			const float surround_left,
			const float surround_right,
			const float back_left,
			const float back_right
		);															// 为每个扬声器设置输出音量
		void set_mix_matrix(
			const PackedFloat32Array& matrix,
			const int outchannels,
			const int inchannels,
			const int inchannel_hop = 0
		);															// 设置一个二维声像矩阵，将信号从输入通道 (列) 映射到输出扬声器 (行)
		Dictionary get_mix_matrix() const;							// 获取一个二维声像矩阵，将信号从输入通道 (列) 映射到输出扬声器 (行)

		// 过滤
		void set_reverb_properties(
			const int instance,
			const float wet
		);															// 为特定混响实例设置湿音/发送电平
		float get_reverb_properties(const int instance) const;		// 获取特定混响实例的湿音/发送电平
		void set_low_pass_gain(const float gain);					// 在内置低通/距离滤波应用时，设置干信号的增益
		float get_low_pass_gain() const;							// 当内置低通/距离滤波应用时，获取干信号的增益

		// DSP 链配置
		void add_dsp(const int index, Ref<FmodDSP> dsp);			// 在指定的索引中添加一个 DSP 单元
		void remove_dsp(Ref<FmodDSP> dsp);							// 从 DSP 链中移除指定的 DSP 单元
		int get_num_dsps() const;									// 获取 DSP 链中 DSP 单元的数量
		Ref<FmodDSP> get_dsp(const int index) const;				// 在 DSP 链中指定的索引处获取 DSP 单元
		void set_dsp_index(Ref<FmodDSP> dsp, const int index);		// 在指定 DSP 的 DSP 链中设置索引
		int get_dsp_index(Ref<FmodDSP> dsp) const;					// 获取 Channel 或 ChannelGroup 的 DSP 链中的 DSP 索引
	
		// 样本精准排程
		Dictionary get_dsp_clock() const;							// 获取当前 DSP 时钟值
		void set_delay(
			const uint64_t start,
			const uint64_t end,
			const bool stopchannels = true
		);															// 设置相对于父 ChannelGroup DSP 时钟的样本准确起始 (或停止) 时间
		Dictionary get_delay() const;								// 获取相对于父 ChannelGroup DSP 时钟的样本精确起始 (或停止) 时间
		void add_fade_point(
			const uint64_t dspclock,
			const float volume
		);															// 在每次相对于父 ChannelGroup DSP 时钟添加一个采样的准确淡出点
		void set_fade_point_ramp(
			const uint64_t dspclock,
			const float volume
		);															// 在未来指定时间使用淡出点添加音量斜坡
		void remove_fade_points(
			const uint64_t start,
			const uint64_t end
		);															// 移除两个指定时钟值之间的所有淡入淡出点
		Dictionary get_fade_points() const;							// 获取存储的渐入淡出点信息

		// 概述
		void set_callback();										// 设置 ChannelControl 级别通知的回调
		void clear_callback();										// 清理回调
		virtual void _on_callback(
			FMOD_CHANNELCONTROL_TYPE controltype,
			FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
			void* commanddata1,
			void* commanddata2
		);															// 供子类重写以处理特定类型的回调
	};
}

extern "C" {
	FMOD_RESULT F_CALL fmod_channel_control_callback(
		FMOD_CHANNELCONTROL* channelcontrol,
		FMOD_CHANNELCONTROL_TYPE controltype,
		FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype,
		void* commanddata1,
		void* commanddata2
	);
}

#endif // FMOD_CHANNEL_CONTROL_H
