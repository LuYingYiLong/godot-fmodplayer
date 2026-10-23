#ifndef FMOD_SYSTEM_H
#define FMOD_SYSTEM_H

#include "fmod_utils.hpp"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <godot_cpp/classes/ref_counted.hpp>

#include <vector>

namespace godot {
	class FmodSound;
	class FmodChannel;
	class FmodChannelGroup;
	class FmodSoundGroup;
	class FmodGeometry;
	class FmodDSP;
	class FmodReverb3D;
	class FmodDSPConnection;

	class FmodSystem : public RefCounted {
		GDCLASS(FmodSystem, RefCounted)

	public:
		// 初始化标志
		enum FmodInitFlags {
			FMOD_INIT_FLAG_NORMAL = 0x00000000,													// 正常初始化
			FMOD_INIT_FLAG_STREAM_FROM_UPDATE = 0x00000001,										// 内部没有创建流线程，流是从 update 驱动，主要用于非实时输出
			FMOD_INIT_FLAG_3D_RIGHTHANDED = 0x00000004,											// 三维计算将采用右手坐标进行，而非默认的左手坐标
			FMOD_INIT_FLAG_CLIP_OUTPUT = 0x00000008,											// 支持对输出值大于 1.0f 或小于 -1.0f 进行硬削波
			FMOD_INIT_FLAG_CHANNEL_LOWPASS = 0x00000100,										// 启用 ChannelControl::setLowPassGain、ChannelControl::set3DOcclusion，或 Geometry API 的自动使用
			FMOD_INIT_FLAG_CHANNEL_DISTANCEFILTER = 0x00000200,									// 所有基于 FMOD_3D 的音色都会在 DSP 链中添加软件低通和高通滤波效果，作为距离自动带通滤波器，使用 set_advanced_settings 来调整中心频率
			FMOD_INIT_FLAG_PROFILE_ENABLE = 0x00010000,											// 启用基于 TCP/IP 的主机，允许 FMOD Studio 或 FMOD Profiler 连接，实时查看内存、CPU 和 DSP 图表
			FMOD_INIT_FLAG_VOL0_BECOMES_VIRTUAL = 0x00020000,									// 任何音量为 0 的声音都会变成虚拟音频，除了虚拟更新位置外不会被处理
			FMOD_INIT_FLAG_GEOMETRY_USECLOSEST = 0x00040000,									// 使用几何引擎时，只需处理最近的多边形，而不是累积所有声音到听者线路相交的多边形
			FMOD_INIT_FLAG_PREFER_DOLBY_DOWNMIX = 0x00080000,									// 使用立体声输出设备 FMOD_SPEAKERMODE_5POINT1 时，请使用 Dolby Pro Logic II 的下混算法，而非默认的立体声下混算法
			FMOD_INIT_FLAG_THREAD_UNSAFE = 0x00100000,											// 禁用 API 调用的线程安全，只有在 FMOD 是从单线程调用且没有使用 Studio API 时才使用！
			FMOD_INIT_FLAG_PROFILE_METER_ALL = 0x00200000,										// 虽然速度较慢，但可以为图表中的每台 DSP 设备添加电平计量
			FMOD_INIT_FLAG_MEMORY_TRACKING = 0x00400000											// 支持内存分配追踪，目前这功能仅在使用 Studio API 时有用，增加内存占用并降低性能
		};

		// 输出类型
		enum FmodOutputType {
			FMOD_OUTPUT_TYPE_AUTODETECT,														// 选择平台的最佳输出模式
			FMOD_OUTPUT_TYPE_UNKNOWN,															// 未知
			FMOD_OUTPUT_TYPE_NOSOUND,															// 全部 - 执行所有混音，但丢弃最终输出
			FMOD_OUTPUT_TYPE_WAVWRITER,															// 全部 - 将输出写入 WAV 文件
			FMOD_OUTPUT_TYPE_NOSOUND_NRT,														// 全部 - 非实时版本的 FMOD_OUTPUTTYPE_NOSOUND
			FMOD_OUTPUT_TYPE_WAVWRITER_NRT,														// 全部 - 非实时版本的 FMOD_OUTPUTTYPE_WAVWRITER
			FMOD_OUTPUT_TYPE_WASAPI,															// Win / UWP / Xbox One / Game Core - Windows 音频会话 API
			FMOD_OUTPUT_TYPE_ASIO,																// Win - 低延迟 ASIO 2.0
			FMOD_OUTPUT_TYPE_PULSEAUDIO,														// Linux - 脉冲音频
			FMOD_OUTPUT_TYPE_ALSA,																// Linux - 高级 Linux 声音架构
			FMOD_OUTPUT_TYPE_COREAUDIO,															// Mac / iOS - 核心音频
			FMOD_OUTPUT_TYPE_AUDIOTRACK,														// Android - Java 音频轨道
			FMOD_OUTPUT_TYPE_OPENSL,															// Android - OpenSL ES
			FMOD_OUTPUT_TYPE_AUDIOOUT,															// PS4 / PS5 - 音频输出
			FMOD_OUTPUT_TYPE_AUDIO3D,															// PS4 - Audio3D
			FMOD_OUTPUT_TYPE_WEBAUDIO,															// HTML5 - Web Audio ScriptProcessorNode 输出
			FMOD_OUTPUT_TYPE_NNAUDIO,															// Switch - NN Audio
			FMOD_OUTPUT_TYPE_WINSONIC,															// Win10 / Xbox One / Game Core - Windows Sonic
			FMOD_OUTPUT_TYPE_AAUDIO,															// Android - AAudio
			FMOD_OUTPUT_TYPE_AUDIOWORKLET,														// HTML5 - 网页音频音频 WorkletNode 输出
			FMOD_OUTPUT_TYPE_AUDIOWORKLETP,
			FMOD_OUTPUT_TYPE_PHASE,																// iOS - PHASE 框架
			FMOD_OUTPUT_TYPE_OHAUDIO,															// OpenHarmony - OHAudio

			FMOD_OUTPUT_TYPE_MAX,
			FMOD_OUTPUT_TYPE_FORCEINT = 65536
		};

		// 扬声器
		enum FmodSpeaker {
			FMOD_SPEAKER_NONE = -1,
			FMOD_SPEAKER_FRONT_LEFT,
			FMOD_SPEAKER_FRONT_RIGHT,
			FMOD_SPEAKER_FRONT_CENTER,
			FMOD_SPEAKER_LOW_FREQUENCY,
			FMOD_SPEAKER_SURROUND_LEFT,
			FMOD_SPEAKER_SURROUND_RIGHT,
			FMOD_SPEAKER_BACK_LEFT,
			FMOD_SPEAKER_BACK_RIGHT,
			FMOD_SPEAKER_TOP_FRONT_LEFT,
			FMOD_SPEAKER_TOP_FRONT_RIGHT,
			FMOD_SPEAKER_TOP_BACK_LEFT,
			FMOD_SPEAKER_TOP_BACK_RIGHT,

			FMOD_SPEAKER_MAX,
			FMOD_SPEAKER_FORCEINT = 65536
		};

		// 扬声器类型
		enum FmodSpeakerMode {
			FMOD_SPEAKER_MODE_DEFAULT,
			FMOD_SPEAKER_MODE_RAW,
			FMOD_SPEAKER_MODE_MONO,
			FMOD_SPEAKER_MODE_STEREO,
			FMOD_SPEAKER_MODE_QUAD,
			FMOD_SPEAKER_MODE_SURROUND,
			FMOD_SPEAKER_MODE_5POINT1,
			FMOD_SPEAKER_MODE_7POINT1,
			FMOD_SPEAKER_MODE_7POINT1POINT4,

			FMOD_SPEAKER_MODE_MAX,
			FMOD_SPEAKER_MODE_FORCEINT = 65536
		};

		// 声音模式
		enum FmodMode {
			FMOD_MODE_DEFAULT = 0x00000000,
			FMOD_MODE_LOOP_OFF = 0x00000001,
			FMOD_MODE_LOOP_NORMAL = 0x00000002,
			FMOD_MODE_LOOP_BIDI = 0x00000004,
			FMOD_MODE_2D = 0x00000008,
			FMOD_MODE_3D = 0x00000010,
			FMOD_MODE_CREATESTREAM = 0x00000080,
			FMOD_MODE_CREATESAMPLE = 0x00000100,
			FMOD_MODE_CREATECOMPRESSEDSAMPLE = 0x00000200,
			FMOD_MODE_OPENUSER = 0x00000400,
			FMOD_MODE_OPENMEMORY = 0x00000800,
			FMOD_MODE_OPENMEMORY_POINT = 0x10000000,
			FMOD_MODE_OPENRAW = 0x00001000,
			FMOD_MODE_OPENONLY = 0x00002000,
			FMOD_MODE_ACCURATETIME = 0x00004000,
			FMOD_MODE_MPEGSEARCH = 0x00008000,
			FMOD_MODE_NONBLOCKING = 0x00010000,
			FMOD_MODE_UNIQUE = 0x00020000,
			FMOD_MODE_3D_HEADRELATIVE = 0x00040000,
			FMOD_MODE_3D_WORLDRELATIVE = 0x00080000,
			FMOD_MODE_3D_INVERSEROLLOFF = 0x00100000,
			FMOD_MODE_3D_LINEARROLLOFF = 0x00200000,
			FMOD_MODE_3D_LINEARSQUAREROLLOFF = 0x00400000,
			FMOD_MODE_3D_INVERSETAPEREDROLLOFF = 0x00800000,
			FMOD_MODE_3D_CUSTOMROLLOFF = 0x04000000,
			FMOD_MODE_3D_IGNOREGEOMETRY = 0x40000000,
			FMOD_MODE_IGNORETAGS = 0x02000000,
			FMOD_MODE_LOWMEM = 0x08000000,
			FMOD_MODE_VIRTUAL_PLAYFROMSTART = 0x80000000
		};

		// 时间计量单位
		enum FmodTimeUnit {
			FMOD_TIME_UNIT_MS = 0x00000001,
			FMOD_TIME_UNIT_PCM = 0x00000002,
			FMOD_TIME_UNIT_PCMBYTES = 0x00000004,
			FMOD_TIME_UNIT_RAWBYTES = 0x00000008,
			FMOD_TIME_UNIT_PCMFRACTION = 0x00000010,
			FMOD_TIME_UNIT_MODORDER = 0x00000100,
			FMOD_TIME_UNIT_MODROW = 0x00000200,
			FMOD_TIME_UNIT_MODPATTERN = 0x00000400
		};

		// 重采样方法
		enum FmodResamplerMethod {
			FMOD_RESAMPLER_DEFAULT,
			FMOD_RESAMPLER_NOINTERP,
			FMOD_RESAMPLER_LINEAR,
			FMOD_RESAMPLER_CUBIC,
			FMOD_RESAMPLER_SPLINE,
			FMOD_RESAMPLER_MAX,
			FMOD_RESAMPLER_FORCEINT = 65536
		};

		// 可用于音频路由的端口类型
		enum FmodPortType {
			FMOD_PORT_TYPE_MUSIC,																// 背景音乐，把 FMOD_PORT_INDEX_NONE 传递成端口索引
			FMOD_PORT_TYPE_COPYRIGHT_MUSIC,														// 背景音乐版权所有，FMOD_PORT_INDEX_NONE 作为移植索引
			FMOD_PORT_TYPE_VOICE,																// 语音聊天，通过目标用户的平台特定用户 ID 作为端口索引
			FMOD_PORT_TYPE_CONTROLLER,															// 控制器扬声器，通过目标用户的平台特定用户 ID 作为端口索引
			FMOD_PORT_TYPE_PERSONAL,															// 个人音频设备，通过目标用户的平台特定用户 ID 作为端口索引
			FMOD_PORT_TYPE_VIBRATION,															// 控制器振动，通过目标用户的特定平台用户 ID 作为端口索引
			FMOD_PORT_TYPE_AUX,																	// 辅助输出端口，FMOD_PORT_INDEX_NONE 端口索引
			FMOD_PORT_TYPE_PASSTHROUGH,															// 直通输出端口，FMOD_PORT_INDEX_NONE 作为端口索引通过
			FMOD_PORT_TYPE_VR_VIBRATION,														// VR 控制器振动，通过目标用户的平台特定用户 ID 作为端口索引
			FMOD_PORT_TYPE_MAX																	// 支持的最大端口类型数
		};

	private:
		FMOD::System* system = nullptr;
		bool owns_system = false;
		mutable std::vector<FMOD_DSP_DESCRIPTION*> dsp_descriptions;

		FMOD_ADVANCEDSETTINGS settings = {};

		// 3D Settings
		float doppler_scale = 1.0f;
		float distance_factor = 1.0f;
		float rolloff_scale = 1.0f;
		Callable _3d_rolloff_callback;

		void _apply_advanced_settings();
		void _apply_3d_settings();
		void _release_dsp_descriptions() const;
		Ref<FmodDSP> _create_dsp_from_description(FMOD_DSP_DESCRIPTION* p_description) const;

	protected:
		static void _bind_methods();

	public:
		void setup_borrowed(FMOD::System* p_system);
		Ref<FmodDSP> create_dsp_from_description(const FMOD_DSP_DESCRIPTION& p_description) const;
		FmodSystem();
		~FmodSystem();

		// Static pointer for C callback to access the instance that set the callback
		static FmodSystem* current_callback_system;

		bool system_is_valid() const;															// 检查 FMOD System 是否有效
		bool system_is_null() const;															// 检查 FMOD System 是否无效
		FMOD::System* get_system() const;

		// 管理
		static Ref<FmodSystem> create_system();													// 创建 FMOD System 实例
		void setup(FMOD::System* p_system);														// 设置 FMOD System 指针
		void init(const int max_channels, FmodInitFlags flags);									// 初始化 FMOD System
		void close();																			// 关闭与输出的连接，回到未初始化状态，但不释放对象
		void release();																			// 关闭并释放该对象及其资源
		void update();																			// 更新 FMOD System
		void mixer_suspend();																	// 暂停调音器线程，放弃使用音频硬件，同时保持内部状态
		void mixer_resume();																	// 恢复混音线程并重新获取音频硬件访问权限

		// 设备选择
		void set_output(FmodOutputType output_type);											// 设置用于运行混音器的输出接口类型
		FmodOutputType get_output() const;														// 获取用于运行混音器的输出接口类型
		int get_num_drivers() const;															// 获取所选输出类型可用的输出驱动器数量
		Dictionary get_driver_info(const int id) const;											// 获取由其索引指定的声音设备的识别信息，且针对所选输出模式
		void set_driver(const int driver);														// 设置所选输出类型的输出驱动
		int get_driver() const;																	// 获取所选输出类型的输出驱动

		// 设置
		void set_software_channels(const int num_software_channels);							// 设置软件混合 Channel 的最大数量
		int get_software_channels() const;														// 获取软件混合 Channel 的最大数量

		void set_software_format(
			const int sample_rate,
			FmodSpeakerMode speaker_mode,
			const int num_raw_speakers
		);																						// 设置软件混音器的输出格式
		Dictionary get_software_format() const;													// 获取软件混音器的输出格式
		void set_dsp_buffer_size(const unsigned int buffer_length, const int num_buffers);		// 设置 FMOD 软件混音引擎设置缓冲区大小
		Dictionary get_dsp_buffer_size() const;													// 获取 FMOD 软件混音引擎缓冲区大小设置
		void set_stream_buffer_size(
			const unsigned int file_buffer_size = 16384,
			FmodTimeUnit file_buffer_size_type = FMOD_TIME_UNIT_RAWBYTES
		);																						// 设置新开启流的默认文件缓冲区大小
		Dictionary get_stream_buffer_size() const;												// 获取新开启流的默认文件缓冲区大小
		void set_speaker_position(
			FmodSpeaker speaker,
			const float x,
			const float y,
			const bool active
		);																						// 设置当前扬声器模式中指定扬声器的位置
		Dictionary get_speaker_position(FmodSpeaker speaker) const;								// 获取当前扬声器模式中指定扬声器的位置

		// 3D 声音设置
		void set_doppler_scale(const float doppler_scale);										// 设置多普勒位移缩放因子
		float get_doppler_scale() const;														// 获取多普勒位移缩放因子

		void set_distance_factor(const float distance_factor);									// 设置距离因子
		float get_distance_factor() const;														// 获取距离因子

		void set_rolloff_scale(const float rolloff_scale);										// 设置距离衰减的缩放因子
		float get_rolloff_scale() const;														// 获取距离衰减的缩放因子

		void set_3d_num_listeners(const int num_listeners);										// 设置 3D 声音场景中 3D "听众" 的数量
		int get_3d_num_listeners() const;														// 获取 3D 声音场景中 3D "听众" 的数量

		// 高级设置
		void set_max_mpeg_codecs(const int max_codecs);											// 设置最大 MPEG 音效
		int get_max_mpeg_codecs() const;														// 获取最大 MPEG 音效

		void set_max_adpcm_codecs(const int max_codecs);										// 设置最大 IMA-ADPCM 音效
		int get_max_adpcm_codecs() const;														// 获取最大 IMA-ADPCM 音效

		void set_max_xma_codecs(const int max_codecs);											// 设置最大 XMA 音效
		int get_max_xma_codecs() const;															// 获取最大 XMA 音效

		void set_max_vorbis_codecs(const int max_codecs);										// 设置最大 Vorbis 音效
		int get_max_vorbis_codecs() const;														// 获取最大 Vorbis 音效

		void set_max_at9_codecs(const int max_codecs);											// 设置最大 AT9 音效
		int get_max_at9_codecs() const;															// 获取最大 AT9 音效

		void set_max_fadpcm_codecs(const int max_codecs);										// 设置最大 FADPCM 音效
		int get_max_fadpcm_codecs() const;														// 获取最大 FADPCM 音效

		void set_max_opus_codecs(const int max_codecs);											// 设置最大 Opus 音效
		int get_max_opus_codecs() const;														// 获取最大 Opus 音效

		void set_asio_num_channels(const int num_channels);										// 设置输入时的元素数和输出时的元素数
		int get_asio_num_channels() const;														// 获取输入时的元素数和输出时的元素数

		void set_vol0_virtual_vol(const float vol);												// 设置虚拟音频
		float get_vol0_virtual_vol() const;														// 获取虚拟音频

		void set_default_decode_buffer_size(const unsigned int size);							// 用于 Streams 时，设置双缓冲区的默认大小
		unsigned int get_default_decode_buffer_size() const;									// 用于 Streams 时，获取双缓冲区的默认大小

		void set_profile_port(const unsigned short port);										// 设置 FMOD Profiler 监听连接端口
		unsigned short get_profile_port() const;												// 获取 FMOD Profiler 监听连接端口

		void set_geometry_max_fade_time(const unsigned int time);								// 设置 Channel 在遮挡变化时淡入新音量的最长时间
		unsigned int get_geometry_max_fade_time() const;										// 获取 Channel 在遮挡变化时淡入新音量的最长时间

		void set_distance_filter_center_freq(const float freq);									// 设置距离滤波器的默认中心频率
		float get_distance_filter_center_freq() const;											// 获取距离滤波器的默认中心频率

		void set_reverb_3d_instance(const int instance);										// 用于 Reverb3D 时，设置使用哪个全局混响实例
		int get_reverb_3d_instance() const;														// 用于 Reverb3D 时，获取使用哪个全局混响实例

		void set_dsp_buffer_pool_size(const int size);											// 设置 DSP 缓冲池中间混合缓冲区的数量
		int get_dsp_buffer_pool_size() const;													// 获取 DSP 缓冲池中间混合缓冲区的数量

		void set_resampler_method(FmodResamplerMethod method);									// 设置使用重采样的方法
		FmodResamplerMethod get_resampler_method() const;										// 获取使用重采样的方法

		void set_random_seed(const unsigned int seed);											// 设置种子用于初始化内部随机数生成器
		unsigned int get_random_seed() const;													// 获取种子用于初始化内部随机数生成器

		void set_max_convolution_threads(const int max_threads);								// 设置 ConvolutionReverbDSP 效果所使用的最大 CPU 线程数
		int get_max_convolution_threads() const;												// 获取 ConvolutionReverbDSP 效果所使用的最大 CPU 线程数

		void set_max_spatial_objects(const int max_objects);									// 设置每个 FMODSystem 可预留的最大空间对象数
		int get_max_spatial_objects() const;													// 获取每个 FMODSystem 可预留的最大空间对象数

		// 网络配置
		void set_network_proxy(const String& p_proxy);											// 设置一个代理服务器，用于所有后续的互联网连接
		String get_network_proxy() const;														// 获取用于互联网流媒体的代理服务器的URL
		void set_network_timeout(const int timeout);											// 设置网络流的超时
		int get_network_timeout() const;														// 获取网络流的超时

		// 信息
		Dictionary get_version() const;															// 获取 Fmod 版本
		uint64_t get_output_handle() const;														// 获取输出类型特定的内部原生接口
		Dictionary get_channels_playing() const;												// 获取当前播放 Channel 数量
		Dictionary get_cpu_usage() const;														// 获取 Core API 不同部分所使用的 CPU 资源
		Dictionary get_file_usage() const;														// 获取文件读取信息
		PackedFloat32Array get_default_mix_matrix(
			const FmodSpeakerMode source_speaker_mode,
			const FmodSpeakerMode target_speaker_mode,
			const int array_length,
			const int hop
		) const;																				// 检索用于从一种扬声器模式转换为另一种模式的默认矩阵
		int get_speaker_mode_channels(FmodSpeakerMode mode) const;								// 获取给定扬声器模式的通道数

		// 创建与获取
		Ref<FmodSound> create_sound_from_file(const String p_path, unsigned int mode, FmodResamplerMethod resampler_method = FMOD_RESAMPLER_DEFAULT) const;	// 从文件创建 FmodSound
		Ref<FmodSound> create_sound_from_memory(
			const PackedByteArray& data,
			unsigned int mode,
			FmodResamplerMethod resampler_method = FMOD_RESAMPLER_DEFAULT
		) const;																				// 从内存创建 FmodSound
		Ref<FmodSound> create_sound_from_res(const String p_path, unsigned int mode, FmodResamplerMethod resampler_method = FMOD_RESAMPLER_DEFAULT) const;		// 从资源文件创建 FmodSound
		Ref<FmodSound> create_stream_from_file(const String p_path, unsigned int mode, FmodResamplerMethod resampler_method = FMOD_RESAMPLER_DEFAULT) const;	// 从文件创建流 FmodSound
		Ref<FmodDSP> create_dsp(const String& name, unsigned int mode = FMOD_DSP_MODE_FILTER, unsigned int default_cooldown = FMOD_DSP_COOLDOWN_INFINITE, int input_channels = 0, int output_channels = 0) const;										// 创建 DSP
		Ref<FmodDSP> create_dsp_by_type(unsigned int type) const;								// 创建一个带有指定类型索引的 DSP
		Ref<FmodChannelGroup> create_channel_group(const String& p_name) const;					// 创建 ChannelGroup
		Ref<FmodSoundGroup> create_sound_group(const String& p_name) const;						// 创建 SoundGroup
		Ref<FmodReverb3D> create_reverb_3d() const;												// 创建一个 "虚拟混响" 对象，这个物体会对3D位置做出反应，并根据与混响物体中心的距离来变形混响环境
		Ref<FmodChannel> play_sound(
			Ref<FmodSound> sound,
			Ref<FmodChannelGroup> channel_group,
			const bool paused = false
		) const;																				// 在 Channel 播放一个声音
		Ref<FmodChannel> play_dsp(
			Ref<FmodDSP> dsp,
			Ref<FmodChannelGroup> channel_group,
			const bool paused = false
		) const;																				// 播放一个 DSP 及其任何输入在 Channel 的信号
		Ref<FmodChannel> get_channel(const int id) const;										// 通过 ID 获取 Channel 的句柄
		Dictionary get_dsp_info_by_type(unsigned int type) const;								// 获取内置 DSP 描述结构信息
		Ref<FmodChannelGroup> get_master_channel_group() const;									// 获取所有声音最终路由到的主通道组
		Ref<FmodSoundGroup> get_master_sound_group() const;										// 获取默认的 SoundGroup，所有声音在创建时放置的位置
		Ref<FmodDSPConnection> create_dsp_connection(unsigned int type = FMOD_DSPCONNECTION_TYPE_STANDARD) const;

		// 运行控制
		void set_3d_listener_attributes(
			const int listener,
			const Vector3 position,
			const Vector3 velocity,
			const Vector3 forward,
			const Vector3 up
		);																						// 设置指定三维声音听者的位置、力度和方向
		Dictionary get_3d_listener_attributes(const int listener) const;						// 获取指定三维声音听者的位置、力度和方向
		void set_reverb_properties(
			const int instance,
			const float decay_time,
			const float early_delay,
			const float late_delay,
			const float hf_reference,
			const float hf_decay_ratio,
			const float diffusion,
			const float density,
			const float low_shelf_frequency,
			const float low_shelf_gain,
			const float high_cut,
			const float early_late_mix,
			const float wet_level
		);																						// 设置指定混响实例当前的混响环境
		Dictionary get_reverb_properties(const int instance) const;								// 获取指定混响实例当前的混响环境
		void attach_channel_group_to_port(
			Ref<FmodChannelGroup> channel_group,
			const FmodPortType prot_type,
			const uint64_t port_index = FMOD_PORT_INDEX_NONE,
			const bool pass_thru = false
		);																						// 将指定的 ChannelGroup 输出连接到输出驱动上的音频端口
		void detach_channel_group_from_port(Ref<FmodChannelGroup> channel_group);				// 将指定 ChannelGroup 的输出从输出驱动上的音频端口断开

		// 录音
		Dictionary get_record_num_drivers() const;												// 获取该输出模式下可用的录音设备数量
		Dictionary get_record_driver_info(const int id) const;									// 获取音频设备的识别信息，这些信息由其索引指定，且针对输出模式
		int get_record_position(const int id) const;											// 获取 PCM 采样中记录缓冲区的当前记录位置
		void record_start(const int id, Ref<FmodSound> sound, const bool loop);					// 启动录制引擎，录制到预设的声音对象
		void record_stop(const int id);															// 停止录制引擎向预设的声音对象录制
		bool is_recording(const int id) const;													// 检索 FMOD 录制 API 的状态，即是否正在录制

		// 几何管理
		Ref<FmodGeometry> create_geometry(
			const int max_polygons = 9999,
			const int max_vertices = 9999
		) const;																				// 创建几何

		void set_3d_max_world_size(const float max_world_size);									// 设置最大世界大小
		float get_3d_max_world_size() const;													// 获取最大世界大小

		Ref<FmodGeometry> load_geometry(const PackedByteArray& data) const;						// 从包含预存几何数据的内存块创建几何对象
		Dictionary get_geometry_occlusion(const Vector3 listener, const Vector3 source) const;	// 计算听者与声源之间的几何遮挡

		// 概述
		void lock_dsp();																		// 互斥函数，将 DSP 引擎 (异步运行于另一线程中) 锁定，使其无法执行
		void unlock_dsp();																		// 互斥函数，用于解锁 DSP 引擎 (异步运行于另一线程) ，并让它继续执行

		void set_3d_rolloff_callback(const Callable& p_callback);
		Callable get_3d_rolloff_callback() const;
		float _handle_3d_rolloff_callback(float distance) const;
	};
}

extern "C" {
	// 衰减回调
	float F_CALL GD_3D_ROLLOFF_CALLBACK(
		FMOD_CHANNELCONTROL* channel_control,
		float distance
	);
}

VARIANT_ENUM_CAST(FmodSystem::FmodInitFlags);
VARIANT_ENUM_CAST(FmodSystem::FmodOutputType);
VARIANT_ENUM_CAST(FmodSystem::FmodSpeaker);
VARIANT_ENUM_CAST(FmodSystem::FmodSpeakerMode);
VARIANT_ENUM_CAST(FmodSystem::FmodMode);
VARIANT_ENUM_CAST(FmodSystem::FmodTimeUnit);
VARIANT_ENUM_CAST(FmodSystem::FmodResamplerMethod);
VARIANT_ENUM_CAST(FmodSystem::FmodPortType);

#endif // !FMOD_SYSTEM_H
