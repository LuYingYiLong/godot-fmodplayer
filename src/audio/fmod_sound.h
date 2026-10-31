#ifndef FMOD_SOUND_H
#define FMOD_SOUND_H

#include "core/fmod_server.h"
#include "playback/fmod_channel.h"
#include "core/fmod_system.h"
#include "audio/fmod_sound_lock.h"

#include <cstdint>
#include <unordered_map>

namespace godot {
	class FmodSound : public RefCounted {
		GDCLASS(FmodSound, RefCounted)

	public:
		enum FmodSoundType {
			FMOD_SOUND_TYPE_UNKNOWN,
			FMOD_SOUND_TYPE_AIFF,
			FMOD_SOUND_TYPE_ASF,
			FMOD_SOUND_TYPE_DLS,
			FMOD_SOUND_TYPE_FLAC,
			FMOD_SOUND_TYPE_FSB,
			FMOD_SOUND_TYPE_IT,
			FMOD_SOUND_TYPE_MIDI,
			FMOD_SOUND_TYPE_MOD,
			FMOD_SOUND_TYPE_MPEG,
			FMOD_SOUND_TYPE_OGGVORBIS,
			FMOD_SOUND_TYPE_PLAYLIST,
			FMOD_SOUND_TYPE_RAW,
			FMOD_SOUND_TYPE_S3M,
			FMOD_SOUND_TYPE_USER,
			FMOD_SOUND_TYPE_WAV,
			FMOD_SOUND_TYPE_XM,
			FMOD_SOUND_TYPE_XMA,
			FMOD_SOUND_TYPE_AUDIOQUEUE,
			FMOD_SOUND_TYPE_AT9,
			FMOD_SOUND_TYPE_VORBIS,
			FMOD_SOUND_TYPE_MEDIACODEC,
			FMOD_SOUND_TYPE_FADPCM,
			FMOD_SOUND_TYPE_OPUS,
			FMOD_SOUND_TYPE_MAX
		};

		enum FmodSoundFormat {
			FMOD_SOUND_FORMAT_NONE,
			FMOD_SOUND_FORMAT_PCM8,
			FMOD_SOUND_FORMAT_PCM16,
			FMOD_SOUND_FORMAT_PCM24,
			FMOD_SOUND_FORMAT_PCM32,
			FMOD_SOUND_FORMAT_PCMFLOAT,
			FMOD_SOUND_FORMAT_BITSTREAM,
			FMOD_SOUND_FORMAT_MAX
		};

		enum FmodTagType {
			FMOD_TAG_TYPE_UNKNOWN,
			FMOD_TAG_TYPE_ID3V1,
			FMOD_TAG_TYPE_ID3V2,
			FMOD_TAG_TYPE_VORBISCOMMENT,
			FMOD_TAG_TYPE_SHOUTCAST,
			FMOD_TAG_TYPE_ICECAST,
			FMOD_TAG_TYPE_ASF,
			FMOD_TAG_TYPE_MIDI,
			FMOD_TAG_TYPE_PLAYLIST,
			FMOD_TAG_TYPE_FMOD,
			FMOD_TAG_TYPE_USER,
			FMOD_TAG_TYPE_MAX
		};

		enum FmodTagDataType {
			FMOD_TAG_DATA_TYPE_BINARY,
			FMOD_TAG_DATA_TYPE_INT,
			FMOD_TAG_DATA_TYPE_FLOAT,
			FMOD_TAG_DATA_TYPE_STRING,
			FMOD_TAG_DATA_TYPE_STRING_UTF16,
			FMOD_TAG_DATA_TYPE_STRING_UTF16BE,
			FMOD_TAG_DATA_TYPE_STRING_UTF8,
			FMOD_TAG_DATA_TYPE_MAX
		};

		enum FmodOpenState {
			FMOD_OPEN_STATE_READY,
			FMOD_OPEN_STATE_LOADING,
			FMOD_OPEN_STATE_ERROR,
			FMOD_OPEN_STATE_CONNECTING,
			FMOD_OPEN_STATE_BUFFERING,
			FMOD_OPEN_STATE_SEEKING,
			FMOD_OPEN_STATE_PLAYING,
			FMOD_OPEN_STATE_SETPOSITION,
			FMOD_OPEN_STATE_MAX
		};

	private:
		bool owns_sound = false;
		std::shared_ptr<std::atomic_bool> system_lifetime;
		mutable int64_t next_sync_point_id = 1;
		mutable std::unordered_map<int64_t, FMOD_SYNCPOINT*> sync_points;

		Callable _pcmread_callback;
		Callable _pcmsetpos_callback;
		Callable _nonblock_callback;

		int64_t _store_sync_point(FMOD_SYNCPOINT* p_point) const;
		FMOD_SYNCPOINT* _resolve_sync_point(int64_t p_point_id) const;

	protected:
		static void _bind_methods();

	public:
		FmodSound();
		~FmodSound();

		FMOD::Sound* sound = nullptr;
		PackedByteArray data;
		String file_path;

		static Ref<FmodSound> load_from_file(const String& p_path);

		// 有效性检查
		bool sound_is_valid() const;									// 检查声音是否有效
		bool sound_is_null() const;										// 检查声音是否无效

		void setup(FMOD::Sound* p_sound, bool p_owns_sound = true, std::shared_ptr<std::atomic_bool> p_system_lifetime = {});

		// 格式信息
		String get_name() const;										// 获取一个声音的名称
		Dictionary get_format() const;									// 获取声音的格式信息
		Dictionary get_defaults() const;								// 获取声音的默认播放属性
		double get_length(
			FmodSystem::FmodTimeUnit time_unit = FmodSystem::FMOD_TIME_UNIT_MS
		) const;														// 获取声音时长
		Dictionary get_num_tags() const;								// 获取元数据标签数量
		Dictionary get_tag(
			const int index, const String& name) const;					// 获取元数据标签

		// 数据读取
		Dictionary get_open_state() const;								// 读取声音在非阻挡标志打开后所处的状态，或流缓冲区的当前状态
		PackedByteArray read_data(const unsigned int length) const;		// 利用 FMOD 内部编解码器，将打开的声音读取数据到指定缓冲区
		void seek_data(const unsigned int pcm);							// 寻找用于数据读取的声音，使用 FMOD 内部编解码器
		Ref<FmodSoundLock> lock(
			const unsigned int offset,
			const unsigned int length
		) const;														// 提供声音的一部分或全部采样数据，用于直接操作

		// 音乐
		int get_music_num_channels() const;								// 获取 MOD/S3M/XM/IT/MIDI 文件内的音乐通道数量
		void set_music_channel_volume(
			const int channel,
			const float volume_db
		) const;														// 设置 MOD/S3M/XM/IT/MIDI 音乐通道的音量
		float get_music_channel_volume(const int channel) const;		// 获取 MOD/S3M/XM/IT/MIDI 音乐通道的音量
		void set_music_speed(const float speed);						// 设置 MOD/S3M/XM/IT/MIDI 音乐的相对速度
		float get_music_speed() const;									// 获取 MOD/S3M/XM/IT/MIDI 音乐的相对速度

		// 同步/标记
		int64_t add_sync_point(
			const unsigned int offset,
			FmodSystem::FmodTimeUnit time_unit,
			const String& name
		);																// 添加一个同步点
		bool delete_sync_point(const int64_t point);					// 移除一个同步点
		int get_num_sync_points() const;								// 获取同步点数量
		int64_t get_sync_point(const int index) const;					// 获取一个同步点
		Dictionary get_sync_point_info(
			const int64_t point,
			FmodSystem::FmodTimeUnit time_unit = FmodSystem::FMOD_TIME_UNIT_MS
		) const;														// 获取嵌入同步点上的信息
		int get_num_sub_sounds() const;									// 获取一个声音内的子声音数量

		// PCM 读取回调 - 用于用户创建的声音或拦截解码器
		void set_pcmread_callback(const Callable& p_callback);
		Callable get_pcmread_callback() const;
		FMOD_RESULT _handle_pcmread_callback(void* data, unsigned int datalen) const;

		// PCM 设置位置回调 - 用于用户创建的声音或拦截 setPosition
		void set_pcmsetpos_callback(const Callable& p_callback);
		Callable get_pcmsetpos_callback() const;
		FMOD_RESULT _handle_pcmsetpos_callback(int subsound, unsigned int position, FMOD_TIMEUNIT postype) const;

		// 非阻塞加载完成回调
		void set_nonblock_callback(const Callable& p_callback);
		Callable get_nonblock_callback() const;
		FMOD_RESULT _handle_nonblock_callback(FMOD_RESULT result) const;
	};
}

VARIANT_ENUM_CAST(FmodSound::FmodSoundType);
VARIANT_ENUM_CAST(FmodSound::FmodSoundFormat);
VARIANT_ENUM_CAST(FmodSound::FmodTagType);
VARIANT_ENUM_CAST(FmodSound::FmodTagDataType);
VARIANT_ENUM_CAST(FmodSound::FmodOpenState);

extern "C" {
	FMOD_RESULT F_CALL GD_FMOD_SOUND_PCMREAD_CALLBACK(
		FMOD_SOUND* sound,
		void* data,
		unsigned int datalen
	);																	// 读取回调，用于用户创建的声音，或在正常声音开启时拦截 FMOD 解码器

	FMOD_RESULT F_CALL GD_FMOD_SOUND_PCMSETPOS_CALLBACK(
		FMOD_SOUND* sound,
		int subsound,
		unsigned int position,
		FMOD_TIMEUNIT postype
	);																	// 为用户创建的声音设置位置回调，或在 API setPositon 调用中拦截 FMOD 解码器

	FMOD_RESULT F_CALL GD_FMOD_SOUND_NONBLOCK_CALLBACK(
		FMOD_SOUND* sound,
		FMOD_RESULT result
	);																	// 回调应在声音加载完成或非阻挡寻道时调用
}

#endif // !FMOD_SOUND_H
