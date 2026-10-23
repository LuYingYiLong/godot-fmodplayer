#ifndef FMOD_AUDIO_STREAM_H
#define FMOD_AUDIO_STREAM_H

#include "audio/fmod_sound.h"
#include "core/fmod_server.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

namespace godot {
	class FmodAudioStream : public Resource {
		GDCLASS(FmodAudioStream, Resource)

	public:
		enum CreateMode {
			MODE_DEFAULT = 0,
			MODE_STREAM = 1 << 0,       // 流式加载（从磁盘流式播放）
			MODE_SAMPLE = 1 << 1,       // 样本模式（加载到内存）
			MODE_LOOP = 1 << 2,         // 循环播放
			MODE_LOOP_BIDI = 1 << 3,    // 双向循环
			MODE_2D = 1 << 4,           // 强制 2D 模式（禁用 3D 处理）
			MODE_3D = 1 << 5,           // 强制 3D 模式
		};

	protected:
		static void _bind_methods();

		// 音频数据（由导入器填充或运行时加载）
		PackedByteArray audio_data;
		bool data_loaded = false;
		Ref<FmodSound> sound;
		unsigned int create_mode_flags = MODE_STREAM;
		int length_override = 0;
		int file_offset = 0;
		int decode_buffer_size = 0;
		int initial_subsound = -1;
		int num_subsounds = 0;
		PackedInt32Array inclusion_list;
		FmodSystem::FmodResamplerMethod resampler_method = FmodSystem::FMOD_RESAMPLER_DEFAULT;
		FmodSound::FmodSoundType suggested_sound_type = FmodSound::FMOD_SOUND_TYPE_UNKNOWN;
		int default_frequency = 0;
		int num_channels = 0;
		FmodSound::FmodSoundFormat format = FmodSound::FMOD_SOUND_FORMAT_NONE;
		String dls_name;
		String encryption_key;
		int max_polyphony = 0;
		int initial_seek_position = 0;
		FmodSystem::FmodTimeUnit initial_seek_postype = FmodSystem::FMOD_TIME_UNIT_MS;
		bool ignore_set_file_system = false;

		unsigned int _get_fmod_mode_flags() const;

	public:
		FmodAudioStream();
		virtual ~FmodAudioStream();

		virtual Ref<FmodSound> _create_sound();
		void set_audio_data(const PackedByteArray& p_data);
		PackedByteArray get_audio_data() const;

		// 获取/设置创建模式标志
		void set_mode_flags(int p_flags);
		int get_mode_flags() const;
		void add_mode_flag(CreateMode p_flag);
		void remove_mode_flag(CreateMode p_flag);
		bool has_mode_flag(CreateMode p_flag) const;

		void set_length_override(int p_length);
		int get_length_override() const;

		void set_file_offset(int p_offset);
		int get_file_offset() const;

		void set_decode_buffer_size(int p_size);
		int get_decode_buffer_size() const;

		void set_initial_subsound(int p_subsound);
		int get_initial_subsound() const;

		void set_num_subsounds(int p_count);
		int get_num_subsounds() const;

		void set_inclusion_list(const PackedInt32Array& p_list);
		PackedInt32Array get_inclusion_list() const;

		void set_resampler_method(FmodSystem::FmodResamplerMethod p_method);
		FmodSystem::FmodResamplerMethod get_resampler_method() const;

		void set_suggested_sound_type(FmodSound::FmodSoundType p_type);
		FmodSound::FmodSoundType get_suggested_sound_type() const;

		void set_default_frequency(int p_frequency);
		int get_default_frequency() const;

		void set_num_channels(int p_channels);
		int get_num_channels() const;

		void set_format(FmodSound::FmodSoundFormat p_format);
		FmodSound::FmodSoundFormat get_format() const;

		void set_dls_name(const String& p_name);
		String get_dls_name() const;

		void set_encryption_key(const String& p_key);
		String get_encryption_key() const;

		void set_max_polyphony(int p_polyphony);
		int get_max_polyphony() const;

		void set_initial_seek_position(int p_position);
		int get_initial_seek_position() const;

		void set_initial_seek_postype(FmodSystem::FmodTimeUnit p_postype);
		FmodSystem::FmodTimeUnit get_initial_seek_postype() const;

		void set_ignore_set_file_system(bool p_ignore);
		bool is_ignore_set_file_system() const;

		// 获取 Sound（延迟创建）
		Ref<FmodSound> get_sound();
		bool preload();
		bool is_preloaded() const;

		// 音频信息
		double get_length() const;
		bool is_data_loaded() const;

		// 运行时加载外部文件
		static Ref<FmodAudioStream> load_from_file(const String& p_path, int p_flags = MODE_STREAM);

		// 清理资源
		void clear();

		// 仅使 sound 缓存失效（保留数据），下次 get_sound() 会重新创建
		void invalidate_sound();
	};
}

VARIANT_ENUM_CAST(FmodAudioStream::CreateMode);

#endif // !FMOD_AUDIO_STREAM_H
