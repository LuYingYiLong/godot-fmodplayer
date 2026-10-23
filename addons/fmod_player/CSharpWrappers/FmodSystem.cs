#pragma warning disable CS0109
using System;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using Godot;
using Godot.Collections;

namespace FmodPlayer;

[Tool]
public partial class FmodSystem : GodotObject
{

	private new static readonly StringName NativeName = new StringName("FmodSystem");

	[Obsolete("Wrapper types cannot be constructed with constructors (it only instantiate the underlying FmodSystem object), please use the Instantiate() method instead.")]
	protected FmodSystem() { }

	private static CSharpScript _wrapperScriptAsset;

	/// <summary>
	/// Try to cast the script on the supplied <paramref name="godotObject"/> to the <see cref="FmodSystem"/> wrapper type,
	/// if no script has attached to the type, or the script attached to the type does not inherit the <see cref="FmodSystem"/> wrapper type,
	/// a new instance of the <see cref="FmodSystem"/> wrapper script will get attaches to the <paramref name="godotObject"/>.
	/// </summary>
	/// <remarks>The developer should only supply the <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</remarks>
	/// <param name="godotObject">The <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</param>
	/// <returns>The existing or a new instance of the <see cref="FmodSystem"/> wrapper script attached to the supplied <paramref name="godotObject"/>.</returns>
	public new static FmodSystem Bind(GodotObject godotObject)
	{
		if (!IsInstanceValid(godotObject))
			return null;

		if (godotObject is FmodSystem wrapperScriptInstance)
			return wrapperScriptInstance;

#if DEBUG
		var expectedType = typeof(FmodSystem);
		var currentObjectClassName = godotObject.GetClass();
		if (!ClassDB.IsParentClass(expectedType.Name, currentObjectClassName))
			throw new InvalidOperationException($"The supplied GodotObject ({currentObjectClassName}) is not the {expectedType.Name} type.");
#endif

		if (_wrapperScriptAsset is null)
		{
			var scriptPathAttribute = typeof(FmodSystem).GetCustomAttributes<ScriptPathAttribute>().FirstOrDefault();
			if (scriptPathAttribute is null) throw new UnreachableException();
			_wrapperScriptAsset = ResourceLoader.Load<CSharpScript>(scriptPathAttribute.Path);
		}

		var instanceId = godotObject.GetInstanceId();
		godotObject.SetScript(_wrapperScriptAsset);
		return (FmodSystem)InstanceFromId(instanceId);
	}

	/// <summary>
	/// Creates an instance of the GDExtension <see cref="FmodSystem"/> type, and attaches a wrapper script instance to it.
	/// </summary>
	/// <returns>The wrapper instance linked to the underlying GDExtension "FmodSystem" type.</returns>
	public new static FmodSystem Instantiate() => Bind(ClassDB.Instantiate(NativeName).As<GodotObject>());

	public enum FmodInitFlags
	{
		FlagNormal = 0,
		FlagStreamFromUpdate = 1,
		Flag3dRighthanded = 4,
		FlagClipOutput = 8,
		FlagChannelLowpass = 256,
		FlagChannelDistancefilter = 512,
		FlagProfileEnable = 65536,
		FlagVol0BecomesVirtual = 131072,
		FlagGeometryUseclosest = 262144,
		FlagPreferDolbyDownmix = 524288,
		FlagThreadUnsafe = 1048576,
		FlagProfileMeterAll = 2097152,
		FlagMemoryTracking = 4194304,
	}

	public enum FmodOutputType
	{
		Autodetect = 0,
		Unknown = 1,
		Nosound = 2,
		Wavwriter = 3,
		NosoundNrt = 4,
		WavwriterNrt = 5,
		Wasapi = 6,
		Asio = 7,
		Pulseaudio = 8,
		Alsa = 9,
		Coreaudio = 10,
		Audiotrack = 11,
		Opensl = 12,
		Audioout = 13,
		Audio3d = 14,
		Webaudio = 15,
		Nnaudio = 16,
		Winsonic = 17,
		Aaudio = 18,
		Audioworklet = 19,
		Audioworkletp = 20,
		Phase = 21,
		Ohaudio = 22,
		Max = 23,
		Forceint = 65536,
	}

	public enum FmodSpeaker
	{
		None = -1,
		FrontLeft = 0,
		FrontRight = 1,
		FrontCenter = 2,
		LowFrequency = 3,
		SurroundLeft = 4,
		SurroundRight = 5,
		BackLeft = 6,
		BackRight = 7,
		TopFrontLeft = 8,
		TopFrontRight = 9,
		TopBackLeft = 10,
		TopBackRight = 11,
		Max = 12,
		Forceint = 65536,
	}

	public enum FmodSpeakerMode
	{
		Default = 0,
		Raw = 1,
		Mono = 2,
		Stereo = 3,
		Quad = 4,
		Surround = 5,
		Mode5point1 = 6,
		Mode7point1 = 7,
		Mode7point1point4 = 8,
		Max = 9,
		Forceint = 65536,
	}

	public enum FmodMode
	{
		Default = 0,
		LoopOff = 1,
		LoopNormal = 2,
		LoopBidi = 4,
		Mode2d = 8,
		Mode3d = 16,
		Createstream = 128,
		Createsample = 256,
		Createcompressedsample = 512,
		Openuser = 1024,
		Openmemory = 2048,
		OpenmemoryPoint = 268435456,
		Openraw = 4096,
		Openonly = 8192,
		Accuratetime = 16384,
		Mpegsearch = 32768,
		Nonblocking = 65536,
		Unique = 131072,
		Mode3dHeadrelative = 262144,
		Mode3dWorldrelative = 524288,
		Mode3dInverserolloff = 1048576,
		Mode3dLinearrolloff = 2097152,
		Mode3dLinearsquarerolloff = 4194304,
		Mode3dInversetaperedrolloff = 8388608,
		Mode3dCustomrolloff = 67108864,
		Mode3dIgnoregeometry = 1073741824,
		Ignoretags = 33554432,
		Lowmem = 134217728,
		VirtualPlayfromstart = -2147483648,
	}

	public enum FmodTimeUnit
	{
		Ms = 1,
		Pcm = 2,
		Pcmbytes = 4,
		Rawbytes = 8,
		Pcmfraction = 16,
		Modorder = 256,
		Modrow = 512,
		Modpattern = 1024,
	}

	public enum FmodResamplerMethod
	{
		Default = 0,
		Nointerp = 1,
		Linear = 2,
		Cubic = 3,
		Spline = 4,
	}

	public enum FmodPortType
	{
		Music = 0,
		CopyrightMusic = 1,
		Voice = 2,
		Controller = 3,
		Personal = 4,
		Vibration = 5,
		Aux = 6,
		Passthrough = 7,
		VrVibration = 8,
		Max = 9,
	}

	public new static class GDExtensionPropertyName
	{
		public new static readonly StringName MaxSoftwareChannels = "max_software_channels";
		public new static readonly StringName DopplerScale = "doppler_scale";
		public new static readonly StringName DistanceFactor = "distance_factor";
		public new static readonly StringName RolloffScale = "rolloff_scale";
		public new static readonly StringName Num3dListeners = "3d_num_listeners";
		public new static readonly StringName MaxMpegCodecs = "max_mpeg_codecs";
		public new static readonly StringName MaxAdpcmCodecs = "max_adpcm_codecs";
		public new static readonly StringName MaxXmaCodecs = "max_xma_codecs";
		public new static readonly StringName MaxVorbisCodecs = "max_vorbis_codecs";
		public new static readonly StringName MaxAt9Codecs = "max_at9_codecs";
		public new static readonly StringName MaxFadpcmCodecs = "max_fadpcm_codecs";
		public new static readonly StringName MaxOpusCodecs = "max_opus_codecs";
		public new static readonly StringName AsioNumChannels = "asio_num_channels";
		public new static readonly StringName Vol0VirtualVol = "vol0_virtual_vol";
		public new static readonly StringName DefaultDecodeBufferSize = "default_decode_buffer_size";
		public new static readonly StringName ProfilePort = "profile_port";
		public new static readonly StringName GeometryMaxFadeTime = "geometry_max_fade_time";
		public new static readonly StringName DistanceFilterCenterFreq = "distance_filter_center_freq";
		public new static readonly StringName Reverb3dInstance = "reverb_3d_instance";
		public new static readonly StringName DspBufferPoolSize = "dsp_buffer_pool_size";
		public new static readonly StringName ResamplerMethod = "resampler_method";
		public new static readonly StringName RandomSeed = "random_seed";
		public new static readonly StringName MaxConvolutionThreads = "max_convolution_threads";
		public new static readonly StringName MaxSpatialObjects = "max_spatial_objects";
		public new static readonly StringName Max3dWorldSize = "3d_max_world_size";
	}

	public new long MaxSoftwareChannels
	{
		get => Get(GDExtensionPropertyName.MaxSoftwareChannels).As<long>();
		set => Set(GDExtensionPropertyName.MaxSoftwareChannels, value);
	}

	public new double DopplerScale
	{
		get => Get(GDExtensionPropertyName.DopplerScale).As<double>();
		set => Set(GDExtensionPropertyName.DopplerScale, value);
	}

	public new double DistanceFactor
	{
		get => Get(GDExtensionPropertyName.DistanceFactor).As<double>();
		set => Set(GDExtensionPropertyName.DistanceFactor, value);
	}

	public new double RolloffScale
	{
		get => Get(GDExtensionPropertyName.RolloffScale).As<double>();
		set => Set(GDExtensionPropertyName.RolloffScale, value);
	}

	public new long Num3dListeners
	{
		get => Get(GDExtensionPropertyName.Num3dListeners).As<long>();
		set => Set(GDExtensionPropertyName.Num3dListeners, value);
	}

	public new long MaxMpegCodecs
	{
		get => Get(GDExtensionPropertyName.MaxMpegCodecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxMpegCodecs, value);
	}

	public new long MaxAdpcmCodecs
	{
		get => Get(GDExtensionPropertyName.MaxAdpcmCodecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxAdpcmCodecs, value);
	}

	public new long MaxXmaCodecs
	{
		get => Get(GDExtensionPropertyName.MaxXmaCodecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxXmaCodecs, value);
	}

	public new long MaxVorbisCodecs
	{
		get => Get(GDExtensionPropertyName.MaxVorbisCodecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxVorbisCodecs, value);
	}

	public new long MaxAt9Codecs
	{
		get => Get(GDExtensionPropertyName.MaxAt9Codecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxAt9Codecs, value);
	}

	public new long MaxFadpcmCodecs
	{
		get => Get(GDExtensionPropertyName.MaxFadpcmCodecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxFadpcmCodecs, value);
	}

	public new long MaxOpusCodecs
	{
		get => Get(GDExtensionPropertyName.MaxOpusCodecs).As<long>();
		set => Set(GDExtensionPropertyName.MaxOpusCodecs, value);
	}

	public new long AsioNumChannels
	{
		get => Get(GDExtensionPropertyName.AsioNumChannels).As<long>();
		set => Set(GDExtensionPropertyName.AsioNumChannels, value);
	}

	public new double Vol0VirtualVol
	{
		get => Get(GDExtensionPropertyName.Vol0VirtualVol).As<double>();
		set => Set(GDExtensionPropertyName.Vol0VirtualVol, value);
	}

	public new long DefaultDecodeBufferSize
	{
		get => Get(GDExtensionPropertyName.DefaultDecodeBufferSize).As<long>();
		set => Set(GDExtensionPropertyName.DefaultDecodeBufferSize, value);
	}

	public new long ProfilePort
	{
		get => Get(GDExtensionPropertyName.ProfilePort).As<long>();
		set => Set(GDExtensionPropertyName.ProfilePort, value);
	}

	public new long GeometryMaxFadeTime
	{
		get => Get(GDExtensionPropertyName.GeometryMaxFadeTime).As<long>();
		set => Set(GDExtensionPropertyName.GeometryMaxFadeTime, value);
	}

	public new double DistanceFilterCenterFreq
	{
		get => Get(GDExtensionPropertyName.DistanceFilterCenterFreq).As<double>();
		set => Set(GDExtensionPropertyName.DistanceFilterCenterFreq, value);
	}

	public new long Reverb3dInstance
	{
		get => Get(GDExtensionPropertyName.Reverb3dInstance).As<long>();
		set => Set(GDExtensionPropertyName.Reverb3dInstance, value);
	}

	public new long DspBufferPoolSize
	{
		get => Get(GDExtensionPropertyName.DspBufferPoolSize).As<long>();
		set => Set(GDExtensionPropertyName.DspBufferPoolSize, value);
	}

	public new Variant ResamplerMethod
	{
		get => Get(GDExtensionPropertyName.ResamplerMethod).As<Variant>();
		set => Set(GDExtensionPropertyName.ResamplerMethod, value);
	}

	public new long RandomSeed
	{
		get => Get(GDExtensionPropertyName.RandomSeed).As<long>();
		set => Set(GDExtensionPropertyName.RandomSeed, value);
	}

	public new long MaxConvolutionThreads
	{
		get => Get(GDExtensionPropertyName.MaxConvolutionThreads).As<long>();
		set => Set(GDExtensionPropertyName.MaxConvolutionThreads, value);
	}

	public new long MaxSpatialObjects
	{
		get => Get(GDExtensionPropertyName.MaxSpatialObjects).As<long>();
		set => Set(GDExtensionPropertyName.MaxSpatialObjects, value);
	}

	public new double Max3dWorldSize
	{
		get => Get(GDExtensionPropertyName.Max3dWorldSize).As<double>();
		set => Set(GDExtensionPropertyName.Max3dWorldSize, value);
	}

	public new static class GDExtensionMethodName
	{
		public new static readonly StringName SystemIsValid = "system_is_valid";
		public new static readonly StringName SystemIsNull = "system_is_null";
		public new static readonly StringName CreateSystem = "create_system";
		public new static readonly StringName Init = "init";
		public new static readonly StringName Close = "close";
		public new static readonly StringName Release = "release";
		public new static readonly StringName Update = "update";
		public new static readonly StringName MixerSuspend = "mixer_suspend";
		public new static readonly StringName MixerResume = "mixer_resume";
		public new static readonly StringName SetOutput = "set_output";
		public new static readonly StringName GetOutput = "get_output";
		public new static readonly StringName GetNumDrivers = "get_num_drivers";
		public new static readonly StringName GetDriverInfo = "get_driver_info";
		public new static readonly StringName SetDriver = "set_driver";
		public new static readonly StringName GetDriver = "get_driver";
		public new static readonly StringName SetSoftwareChannels = "set_software_channels";
		public new static readonly StringName GetSoftwareChannels = "get_software_channels";
		public new static readonly StringName SetSoftwareFormat = "set_software_format";
		public new static readonly StringName GetSoftwareFormat = "get_software_format";
		public new static readonly StringName SetDspBufferSize = "set_dsp_buffer_size";
		public new static readonly StringName GetDspBufferSize = "get_dsp_buffer_size";
		public new static readonly StringName SetStreamBufferSize = "set_stream_buffer_size";
		public new static readonly StringName GetStreamBufferSize = "get_stream_buffer_size";
		public new static readonly StringName SetSpeakerPosition = "set_speaker_position";
		public new static readonly StringName GetSpeakerPosition = "get_speaker_position";
		public new static readonly StringName Set3dRolloffCallback = "set_3d_rolloff_callback";
		public new static readonly StringName Get3dRolloffCallback = "get_3d_rolloff_callback";
		public new static readonly StringName Set3dListenerAttributes = "set_3d_listener_attributes";
		public new static readonly StringName Get3dListenerAttributes = "get_3d_listener_attributes";
		public new static readonly StringName SetReverbProperties = "set_reverb_properties";
		public new static readonly StringName GetReverbProperties = "get_reverb_properties";
		public new static readonly StringName AttachChannelGroupToPort = "attach_channel_group_to_port";
		public new static readonly StringName DetachChannelGroupFromPort = "detach_channel_group_from_port";
		public new static readonly StringName SetNetworkProxy = "set_network_proxy";
		public new static readonly StringName GetNetworkProxy = "get_network_proxy";
		public new static readonly StringName SetNetworkTimeout = "set_network_timeout";
		public new static readonly StringName GetNetworkTimeout = "get_network_timeout";
		public new static readonly StringName GetVersion = "get_version";
		public new static readonly StringName GetOutputHandle = "get_output_handle";
		public new static readonly StringName GetChannelsPlaying = "get_channels_playing";
		public new static readonly StringName GetCpuUsage = "get_cpu_usage";
		public new static readonly StringName GetFileUsage = "get_file_usage";
		public new static readonly StringName GetDefaultMixMatrix = "get_default_mix_matrix";
		public new static readonly StringName GetSpeakerModeChannels = "get_speaker_mode_channels";
		public new static readonly StringName CreateSoundFromFile = "create_sound_from_file";
		public new static readonly StringName CreateSoundFromMemory = "create_sound_from_memory";
		public new static readonly StringName CreateSoundFromRes = "create_sound_from_res";
		public new static readonly StringName CreateStreamFromFile = "create_stream_from_file";
		public new static readonly StringName CreateDsp = "create_dsp";
		public new static readonly StringName CreateDspByType = "create_dsp_by_type";
		public new static readonly StringName CreateChannelGroup = "create_channel_group";
		public new static readonly StringName CreateSoundGroup = "create_sound_group";
		public new static readonly StringName CreateReverb3d = "create_reverb_3d";
		public new static readonly StringName PlaySound = "play_sound";
		public new static readonly StringName PlayDsp = "play_dsp";
		public new static readonly StringName GetChannel = "get_channel";
		public new static readonly StringName GetDspInfoByType = "get_dsp_info_by_type";
		public new static readonly StringName GetMasterChannelGroup = "get_master_channel_group";
		public new static readonly StringName GetMasterSoundGroup = "get_master_sound_group";
		public new static readonly StringName GetRecordNumDrivers = "get_record_num_drivers";
		public new static readonly StringName GetRecordDriverInfo = "get_record_driver_info";
		public new static readonly StringName GetRecordPosition = "get_record_position";
		public new static readonly StringName RecordStart = "record_start";
		public new static readonly StringName RecordStop = "record_stop";
		public new static readonly StringName IsRecording = "is_recording";
		public new static readonly StringName CreateGeometry = "create_geometry";
		public new static readonly StringName LoadGeometry = "load_geometry";
		public new static readonly StringName GetGeometryOcclusion = "get_geometry_occlusion";
		public new static readonly StringName LockDsp = "lock_dsp";
		public new static readonly StringName UnlockDsp = "unlock_dsp";
	}

	public new bool SystemIsValid() => 
		Call(GDExtensionMethodName.SystemIsValid, []).As<bool>();

	public new bool SystemIsNull() => 
		Call(GDExtensionMethodName.SystemIsNull, []).As<bool>();

	public new static FmodSystem CreateSystem(long maxChannels = 0, FmodSystem.FmodInitFlags flags = (FmodSystem.FmodInitFlags)32)
	{
		return FmodSystem.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.CreateSystem, [maxChannels, Variant.From(flags)]).As<GodotObject>());
	}

	public new void Init(long maxChannels = 0, FmodSystem.FmodInitFlags flags = (FmodSystem.FmodInitFlags)32)
	{
		Call(GDExtensionMethodName.Init, [maxChannels, Variant.From(flags)]);
	}

	public new void Close() => 
		Call(GDExtensionMethodName.Close, []);

	public new void Release() => 
		Call(GDExtensionMethodName.Release, []);

	public new void Update() => 
		Call(GDExtensionMethodName.Update, []);

	public new void MixerSuspend() => 
		Call(GDExtensionMethodName.MixerSuspend, []);

	public new void MixerResume() => 
		Call(GDExtensionMethodName.MixerResume, []);

	public new void SetOutput(FmodSystem.FmodOutputType outputType) => 
		Call(GDExtensionMethodName.SetOutput, [Variant.From(outputType)]);

	public new FmodSystem.FmodOutputType GetOutput() => 
		Call(GDExtensionMethodName.GetOutput, []).As<FmodSystem.FmodOutputType>();

	public new long GetNumDrivers() => 
		Call(GDExtensionMethodName.GetNumDrivers, []).As<long>();

	public new Godot.Collections.Dictionary GetDriverInfo(long id) => 
		Call(GDExtensionMethodName.GetDriverInfo, [id]).As<Godot.Collections.Dictionary>();

	public new void SetDriver(long driver) => 
		Call(GDExtensionMethodName.SetDriver, [driver]);

	public new long GetDriver() => 
		Call(GDExtensionMethodName.GetDriver, []).As<long>();

	public new void SetSoftwareFormat(long sampleRate, FmodSystem.FmodSpeakerMode speakerMode, long numRawSpeakers) => 
		Call(GDExtensionMethodName.SetSoftwareFormat, [sampleRate, Variant.From(speakerMode), numRawSpeakers]);

	public new Godot.Collections.Dictionary GetSoftwareFormat() => 
		Call(GDExtensionMethodName.GetSoftwareFormat, []).As<Godot.Collections.Dictionary>();

	public new void SetDspBufferSize(long bufferLength, long numBuffers) => 
		Call(GDExtensionMethodName.SetDspBufferSize, [bufferLength, numBuffers]);

	public new Godot.Collections.Dictionary GetDspBufferSize() => 
		Call(GDExtensionMethodName.GetDspBufferSize, []).As<Godot.Collections.Dictionary>();

	public new void SetStreamBufferSize(long fileBufferSize = 8, FmodSystem.FmodTimeUnit fileBufferSizeType = (FmodSystem.FmodTimeUnit)16384)
	{
		Call(GDExtensionMethodName.SetStreamBufferSize, [fileBufferSize, Variant.From(fileBufferSizeType)]);
	}

	public new Godot.Collections.Dictionary GetStreamBufferSize() => 
		Call(GDExtensionMethodName.GetStreamBufferSize, []).As<Godot.Collections.Dictionary>();

	public new void SetSpeakerPosition(FmodSystem.FmodSpeaker speaker, double x, double y, bool active) => 
		Call(GDExtensionMethodName.SetSpeakerPosition, [Variant.From(speaker), x, y, active]);

	public new Godot.Collections.Dictionary GetSpeakerPosition(FmodSystem.FmodSpeaker speaker) => 
		Call(GDExtensionMethodName.GetSpeakerPosition, [Variant.From(speaker)]).As<Godot.Collections.Dictionary>();

	public new void Set3dRolloffCallback(Callable callback) => 
		Call(GDExtensionMethodName.Set3dRolloffCallback, [callback]);

	public new Callable Get3dRolloffCallback() => 
		Call(GDExtensionMethodName.Get3dRolloffCallback, []).As<Callable>();

	public new void SetNetworkProxy(string proxy) => 
		Call(GDExtensionMethodName.SetNetworkProxy, [proxy]);

	public new string GetNetworkProxy() => 
		Call(GDExtensionMethodName.GetNetworkProxy, []).As<string>();

	public new void SetNetworkTimeout(long timeout) => 
		Call(GDExtensionMethodName.SetNetworkTimeout, [timeout]);

	public new long GetNetworkTimeout() => 
		Call(GDExtensionMethodName.GetNetworkTimeout, []).As<long>();

	public new Godot.Collections.Dictionary GetVersion() => 
		Call(GDExtensionMethodName.GetVersion, []).As<Godot.Collections.Dictionary>();

	public new long GetOutputHandle() => 
		Call(GDExtensionMethodName.GetOutputHandle, []).As<long>();

	public new Godot.Collections.Dictionary GetChannelsPlaying() => 
		Call(GDExtensionMethodName.GetChannelsPlaying, []).As<Godot.Collections.Dictionary>();

	public new Godot.Collections.Dictionary GetCpuUsage() => 
		Call(GDExtensionMethodName.GetCpuUsage, []).As<Godot.Collections.Dictionary>();

	public new Godot.Collections.Dictionary GetFileUsage() => 
		Call(GDExtensionMethodName.GetFileUsage, []).As<Godot.Collections.Dictionary>();

	public new float[] GetDefaultMixMatrix(FmodSystem.FmodSpeakerMode sourceSpeakerMode, FmodSystem.FmodSpeakerMode targetSpeakerMode, long arrayLength, long hop) => 
		Call(GDExtensionMethodName.GetDefaultMixMatrix, [Variant.From(sourceSpeakerMode), Variant.From(targetSpeakerMode), arrayLength, hop]).As<float[]>();

	public new long GetSpeakerModeChannels(FmodSystem.FmodSpeakerMode mode) => 
		Call(GDExtensionMethodName.GetSpeakerModeChannels, [Variant.From(mode)]).As<long>();

	public new FmodSound CreateSoundFromFile(string path, long mode = 0, FmodResamplerMethod resamplerMethod = FmodResamplerMethod.Default) =>
		FmodSound.Bind(Call(GDExtensionMethodName.CreateSoundFromFile, [path, mode, Variant.From(resamplerMethod)]).As<RefCounted>());

	public new FmodSound CreateSoundFromMemory(byte[] data, long mode = 0, FmodResamplerMethod resamplerMethod = FmodResamplerMethod.Default) =>
		FmodSound.Bind(Call(GDExtensionMethodName.CreateSoundFromMemory, [data, mode, Variant.From(resamplerMethod)]).As<RefCounted>());

	public new FmodSound CreateSoundFromRes(string path, long mode = 0, FmodResamplerMethod resamplerMethod = FmodResamplerMethod.Default) =>
		FmodSound.Bind(Call(GDExtensionMethodName.CreateSoundFromRes, [path, mode, Variant.From(resamplerMethod)]).As<RefCounted>());

	public new FmodSound CreateStreamFromFile(string path, long mode = 0, FmodResamplerMethod resamplerMethod = FmodResamplerMethod.Default) =>
		FmodSound.Bind(Call(GDExtensionMethodName.CreateStreamFromFile, [path, mode, Variant.From(resamplerMethod)]).As<RefCounted>());

	public FmodDSPConnection CreateDspConnection(FmodDSPConnection.FmodDspConnectionType type = FmodDSPConnection.FmodDspConnectionType.Standard) =>
		FmodDSPConnection.Bind(Call("create_dsp_connection", [Variant.From(type)]).As<RefCounted>());

	public new FmodDSP CreateDsp(string name, FmodDSP.FmodDspMode mode = FmodDSP.FmodDspMode.Filter, long defaultCooldown = FmodDSP.DspCooldownInfinite, long inputChannels = 0, long outputChannels = 0) =>
		FmodDSP.Bind(Call(GDExtensionMethodName.CreateDsp, [name, Variant.From(mode), defaultCooldown, inputChannels, outputChannels]).As<RefCounted>());

	public new FmodDSP CreateDspByType(long type) => 
		FmodDSP.Bind(Call(GDExtensionMethodName.CreateDspByType, [type]).As<RefCounted>());

	public new FmodChannelGroup CreateChannelGroup(string name) => 
		FmodChannelGroup.Bind(Call(GDExtensionMethodName.CreateChannelGroup, [name]).As<RefCounted>());

	public new FmodSoundGroup CreateSoundGroup(string name) => 
		FmodSoundGroup.Bind(Call(GDExtensionMethodName.CreateSoundGroup, [name]).As<RefCounted>());

	public new FmodReverb3D CreateReverb3d() => 
		FmodReverb3D.Bind(Call(GDExtensionMethodName.CreateReverb3d, []).As<RefCounted>());

	public new FmodChannel PlaySound(FmodSound sound, FmodChannelGroup channelGroup, bool paused = false) => 
		FmodChannel.Bind(Call(GDExtensionMethodName.PlaySound, [sound, channelGroup, paused]).As<RefCounted>());

	public new FmodChannel PlayDsp(FmodDSP dsp, FmodChannelGroup channelGroup, bool paused = false) => 
		FmodChannel.Bind(Call(GDExtensionMethodName.PlayDsp, [dsp, channelGroup, paused]).As<RefCounted>());

	public new FmodChannel GetChannel(long id) => 
		FmodChannel.Bind(Call(GDExtensionMethodName.GetChannel, [id]).As<RefCounted>());

	public new Godot.Collections.Dictionary GetDspInfoByType(long type) => 
		Call(GDExtensionMethodName.GetDspInfoByType, [type]).As<Godot.Collections.Dictionary>();

	public new FmodChannelGroup GetMasterChannelGroup() => 
		FmodChannelGroup.Bind(Call(GDExtensionMethodName.GetMasterChannelGroup, []).As<RefCounted>());

	public new FmodSoundGroup GetMasterSoundGroup() => 
		FmodSoundGroup.Bind(Call(GDExtensionMethodName.GetMasterSoundGroup, []).As<RefCounted>());

	public new Godot.Collections.Dictionary GetRecordNumDrivers() => 
		Call(GDExtensionMethodName.GetRecordNumDrivers, []).As<Godot.Collections.Dictionary>();

	public new Godot.Collections.Dictionary GetRecordDriverInfo(long id) => 
		Call(GDExtensionMethodName.GetRecordDriverInfo, [id]).As<Godot.Collections.Dictionary>();

	public new long GetRecordPosition(long id) => 
		Call(GDExtensionMethodName.GetRecordPosition, [id]).As<long>();

	public new void RecordStart(long id, FmodSound sound, bool loop) => 
		Call(GDExtensionMethodName.RecordStart, [id, sound, loop]);

	public new void RecordStop(long id) => 
		Call(GDExtensionMethodName.RecordStop, [id]);

	public new bool IsRecording(long id) => 
		Call(GDExtensionMethodName.IsRecording, [id]).As<bool>();

	public new FmodGeometry CreateGeometry(long maxPolygons = 9999, long maxVertices = 9999) => 
		FmodGeometry.Bind(Call(GDExtensionMethodName.CreateGeometry, [maxPolygons, maxVertices]).As<RefCounted>());

	public new FmodGeometry LoadGeometry(byte[] data) => 
		FmodGeometry.Bind(Call(GDExtensionMethodName.LoadGeometry, [data]).As<RefCounted>());

	public new Godot.Collections.Dictionary GetGeometryOcclusion(Vector3 listener, Vector3 source) => 
		Call(GDExtensionMethodName.GetGeometryOcclusion, [listener, source]).As<Godot.Collections.Dictionary>();

	public new void LockDsp() => 
		Call(GDExtensionMethodName.LockDsp, []);

	public new void UnlockDsp() => 
		Call(GDExtensionMethodName.UnlockDsp, []);

	public new void SetSoftwareChannels(long numChannels) => 
		Call(GDExtensionMethodName.SetSoftwareChannels, [numChannels]);

	public new long GetSoftwareChannels() => 
		Call(GDExtensionMethodName.GetSoftwareChannels, []).As<long>();

	public new void Set3dListenerAttributes(long listener, Vector3 position, Vector3 velocity, Vector3 forward, Vector3 up) => 
		Call(GDExtensionMethodName.Set3dListenerAttributes, [listener, position, velocity, forward, up]);

	public new Godot.Collections.Dictionary Get3dListenerAttributes(long listener) => 
		Call(GDExtensionMethodName.Get3dListenerAttributes, [listener]).As<Godot.Collections.Dictionary>();

	public new void SetReverbProperties(long instance, double decayTime, double earlyDelay, double lateDelay, double hfReference, double hfDecayRatio, double diffusion, double density, double lowShelfFrequency, double lowShelfGain, double highCut, double earlyLateMix, double wetLevel) => 
		Call(GDExtensionMethodName.SetReverbProperties, [instance, decayTime, earlyDelay, lateDelay, hfReference, hfDecayRatio, diffusion, density, lowShelfFrequency, lowShelfGain, highCut, earlyLateMix, wetLevel]);

	public new Godot.Collections.Dictionary GetReverbProperties(long instance) => 
		Call(GDExtensionMethodName.GetReverbProperties, [instance]).As<Godot.Collections.Dictionary>();

	public new void AttachChannelGroupToPort(FmodChannelGroup channelGroup, FmodSystem.FmodPortType portType, ulong portIndex = 0, bool passThru = false) => 
		Call(GDExtensionMethodName.AttachChannelGroupToPort, [channelGroup, Variant.From(portType), portIndex, passThru]);

	public new void DetachChannelGroupFromPort(FmodChannelGroup channelGroup) => 
		Call(GDExtensionMethodName.DetachChannelGroupFromPort, [channelGroup]);

}
