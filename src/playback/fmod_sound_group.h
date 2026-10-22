#ifndef FMOD_SOUND_GROUP_H
#define FMOD_SOUND_GROUP_H

#include "core/fmod_utils.hpp"
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {
	class FmodSound;

	class FmodSoundGroup : public RefCounted {
		GDCLASS(FmodSoundGroup, RefCounted)

	public:
		FmodSoundGroup();
		~FmodSoundGroup();

		enum Behavior {
			BEHAVIOR_FAIL,												// 调用 FmodSystem::play_sound 时，过剩的声音会失败
			BEHAVIOR_MUTE,												// 多余的声音会从静音开始，当足够多的声音停止时才会变得可听见
			BEHAVIOR_STEAL_LOWEST,										// 过多的声音会从乐队中最安静的声音中被窃取
			BEHAVIOR_MAX,												// 最大 SoundGroup 行为数量
		};

	private:
		FMOD::SoundGroup* sound_group = nullptr;

	protected:
		static void _bind_methods();

	public:
		bool sound_group_is_valid() const;								// 检查 SoundGroup 是否有效
		bool sound_group_is_null() const;								// 检查 SoundGroup 是否无效

		void setup(FMOD::SoundGroup* p_sound_group);

		// 基础功能
		void set_max_audible(const int max_audible);					// 设置 SoundGroup 同时可听到的最大回放次数
		int get_max_audible() const;									// 获取 SoundGroup 同时可听到的最大回放次数

		void set_max_audible_behavior(const Behavior behavior);			// 设置 SoundGroup 播放过多声音时，该功能会改变声音播放的行为
		Behavior get_max_audible_behavior() const;						// 获取 SoundGroup 播放过多声音时的行为

		void set_mute_fade_speed(const float speed);					// 设置静音淡出时间
		float get_mute_fade_speed() const;								// 获取静音淡出时间

		void set_volume_db(const float volume_db);						// 设置音量
		float get_volume_db() const;									// 获取音量

		// 音效功能
		int get_num_sounds() const;										// 获取 SoundGroup 中的 FmodSound 数量
		Ref<FmodSound> get_sound(const int index) const;				// 获取 FmodSound
		int get_num_playing() const;									// 获取 SoundGroup 正在播放的 Channel 数量
		void stop();													// 停止 SoundGroup 中的所有声音

		// 概述
		String get_name() const;										// 获取 SoundGroup 的名称
		void release();													// 释放 SoundGroup 对象，并将所有 FmodSound 返回主声组
	};
}

VARIANT_ENUM_CAST(FmodSoundGroup::Behavior);

#endif // !FMOD_SOUND_GROUP_H
