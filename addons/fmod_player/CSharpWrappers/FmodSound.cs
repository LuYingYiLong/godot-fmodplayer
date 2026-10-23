#pragma warning disable CS0109
using System;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using Godot;
using Godot.Collections;

namespace FmodPlayer;

[Tool]
public partial class FmodSound : RefCounted
{

	private new static readonly StringName NativeName = new StringName("FmodSound");

	[Obsolete("Wrapper types cannot be constructed with constructors (it only instantiate the underlying FmodSound object), please use the Instantiate() method instead.")]
	protected FmodSound() { }

	private static CSharpScript _wrapperScriptAsset;

	/// <summary>
	/// Try to cast the script on the supplied <paramref name="godotObject"/> to the <see cref="FmodSound"/> wrapper type,
	/// if no script has attached to the type, or the script attached to the type does not inherit the <see cref="FmodSound"/> wrapper type,
	/// a new instance of the <see cref="FmodSound"/> wrapper script will get attaches to the <paramref name="godotObject"/>.
	/// </summary>
	/// <remarks>The developer should only supply the <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</remarks>
	/// <param name="godotObject">The <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</param>
	/// <returns>The existing or a new instance of the <see cref="FmodSound"/> wrapper script attached to the supplied <paramref name="godotObject"/>.</returns>
	public new static FmodSound Bind(GodotObject godotObject)
	{
		if (!IsInstanceValid(godotObject))
			return null;

		if (godotObject is FmodSound wrapperScriptInstance)
			return wrapperScriptInstance;

#if DEBUG
		var expectedType = typeof(FmodSound);
		var currentObjectClassName = godotObject.GetClass();
		if (!ClassDB.IsParentClass(expectedType.Name, currentObjectClassName))
			throw new InvalidOperationException($"The supplied GodotObject ({currentObjectClassName}) is not the {expectedType.Name} type.");
#endif

		if (_wrapperScriptAsset is null)
		{
			var scriptPathAttribute = typeof(FmodSound).GetCustomAttributes<ScriptPathAttribute>().FirstOrDefault();
			if (scriptPathAttribute is null) throw new UnreachableException();
			_wrapperScriptAsset = ResourceLoader.Load<CSharpScript>(scriptPathAttribute.Path);
		}

		var instanceId = godotObject.GetInstanceId();
		godotObject.SetScript(_wrapperScriptAsset);
		return (FmodSound)InstanceFromId(instanceId);
	}

	/// <summary>
	/// Creates an instance of the GDExtension <see cref="FmodSound"/> type, and attaches a wrapper script instance to it.
	/// </summary>
	/// <returns>The wrapper instance linked to the underlying GDExtension "FmodSound" type.</returns>
	public new static FmodSound Instantiate() => Bind(ClassDB.Instantiate(NativeName).As<GodotObject>());

	public enum FmodSoundType
	{
		Unknown = 0,
		Aiff = 1,
		Asf = 2,
		Dls = 3,
		Flac = 4,
		Fsb = 5,
		It = 6,
		Midi = 7,
		Mod = 8,
		Mpeg = 9,
		Oggvorbis = 10,
		Playlist = 11,
		Raw = 12,
		S3m = 13,
		User = 14,
		Wav = 15,
		Xm = 16,
		Xma = 17,
		Audioqueue = 18,
		At9 = 19,
		Vorbis = 20,
		Mediacodec = 21,
		Fadpcm = 22,
		Opus = 23,
		Max = 24,
	}

	public enum FmodSoundFormat
	{
		None = 0,
		Pcm8 = 1,
		Pcm16 = 2,
		Pcm24 = 3,
		Pcm32 = 4,
		Pcmfloat = 5,
		Bitstream = 6,
		Max = 7,
	}

	public enum FmodTagType
	{
		Unknown = 0,
		Id3v1 = 1,
		Id3v2 = 2,
		Vorbiscomment = 3,
		Shoutcast = 4,
		Icecast = 5,
		Asf = 6,
		Midi = 7,
		Playlist = 8,
		Fmod = 9,
		User = 10,
		Max = 11,
	}

	public enum FmodTagDataType
	{
		Binary = 0,
		Int = 1,
		Float = 2,
		String = 3,
		StringUtf16 = 4,
		StringUtf16be = 5,
		StringUtf8 = 6,
		Max = 7,
	}

	public enum FmodOpenState
	{
		Ready = 0,
		Loading = 1,
		Error = 2,
		Connecting = 3,
		Buffering = 4,
		Seeking = 5,
		Playing = 6,
		Setposition = 7,
		Max = 8,
	}

	public new static class GDExtensionMethodName
	{
		public new static readonly StringName LoadFromFile = "load_from_file";
		public new static readonly StringName SoundIsValid = "sound_is_valid";
		public new static readonly StringName SoundIsNull = "sound_is_null";
		public new static readonly StringName Setup = "setup";
		public new static readonly StringName GetName = "get_name";
		public new static readonly StringName GetFormat = "get_format";
		public new static readonly StringName GetDefaults = "get_defaults";
		public new static readonly StringName GetLength = "get_length";
		public new static readonly StringName GetNumTags = "get_num_tags";
		public new static readonly StringName GetTag = "get_tag";
		public new static readonly StringName GetOpenState = "get_open_state";
		public new static readonly StringName ReadData = "read_data";
		public new static readonly StringName SeekData = "seek_data";
		public new static readonly StringName Lock = "lock";
		public new static readonly StringName GetMusicNumChannels = "get_music_num_channels";
		public new static readonly StringName SetMusicChannelVolume = "set_music_channel_volume";
		public new static readonly StringName GetMusicChannelVolume = "get_music_channel_volume";
		public new static readonly StringName SetMusicSpeed = "set_music_speed";
		public new static readonly StringName GetMusicSpeed = "get_music_speed";
		public new static readonly StringName AddSyncPoint = "add_sync_point";
		public new static readonly StringName DeleteSyncPoint = "delete_sync_point";
		public new static readonly StringName GetNumSyncPoints = "get_num_sync_points";
		public new static readonly StringName GetSyncPoint = "get_sync_point";
		public new static readonly StringName GetSyncPointInfo = "get_sync_point_info";
		public new static readonly StringName GetNumSubSounds = "get_num_sub_sounds";
		public new static readonly StringName SetPcmreadCallback = "set_pcmread_callback";
		public new static readonly StringName GetPcmreadCallback = "get_pcmread_callback";
		public new static readonly StringName SetPcmsetposCallback = "set_pcmsetpos_callback";
		public new static readonly StringName GetPcmsetposCallback = "get_pcmsetpos_callback";
		public new static readonly StringName SetNonblockCallback = "set_nonblock_callback";
		public new static readonly StringName GetNonblockCallback = "get_nonblock_callback";
	}

	public new static FmodSound LoadFromFile(string path) => 
		FmodSound.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.LoadFromFile, [path]).As<RefCounted>());

	public new bool SoundIsValid() => 
		Call(GDExtensionMethodName.SoundIsValid, []).As<bool>();

	public new bool SoundIsNull() => 
		Call(GDExtensionMethodName.SoundIsNull, []).As<bool>();

	public new string GetName() => 
		Call(GDExtensionMethodName.GetName, []).As<string>();

	public new Godot.Collections.Dictionary GetFormat() => 
		Call(GDExtensionMethodName.GetFormat, []).As<Godot.Collections.Dictionary>();

	public new Godot.Collections.Dictionary GetDefaults() =>
		Call(GDExtensionMethodName.GetDefaults, []).As<Godot.Collections.Dictionary>();

	public new double GetLength(FmodSystem.FmodTimeUnit timeUnit = FmodSystem.FmodTimeUnit.Ms) => 
		Call(GDExtensionMethodName.GetLength, [Variant.From(timeUnit)]).As<double>();

	public new Godot.Collections.Dictionary GetNumTags() => 
		Call(GDExtensionMethodName.GetNumTags, []).As<Godot.Collections.Dictionary>();

	public new Godot.Collections.Dictionary GetTag(long index, string name = "") => 
		Call(GDExtensionMethodName.GetTag, [index, name]).As<Godot.Collections.Dictionary>();

	public new Godot.Collections.Dictionary GetOpenState() => 
		Call(GDExtensionMethodName.GetOpenState, []).As<Godot.Collections.Dictionary>();

	public new byte[] ReadData(long length) => 
		Call(GDExtensionMethodName.ReadData, [length]).As<byte[]>();

	public new void SeekData(long pcm) => 
		Call(GDExtensionMethodName.SeekData, [pcm]);

	public new FmodSoundLock Lock(long offset, long length) => 
		FmodSoundLock.Bind(Call(GDExtensionMethodName.Lock, [offset, length]).As<RefCounted>());

	public new long GetMusicNumChannels() => 
		Call(GDExtensionMethodName.GetMusicNumChannels, []).As<long>();

	public new void SetMusicChannelVolume(long channel, double volumeDb) => 
		Call(GDExtensionMethodName.SetMusicChannelVolume, [channel, volumeDb]);

	public new double GetMusicChannelVolume(long unnamedArg0) => 
		Call(GDExtensionMethodName.GetMusicChannelVolume, [unnamedArg0]).As<double>();

	public new void SetMusicSpeed(double speed) => 
		Call(GDExtensionMethodName.SetMusicSpeed, [speed]);

	public new double GetMusicSpeed() => 
		Call(GDExtensionMethodName.GetMusicSpeed, []).As<double>();

	public new long AddSyncPoint(long offset, FmodSystem.FmodTimeUnit timeUnit, string name) =>
		Call(GDExtensionMethodName.AddSyncPoint, [offset, Variant.From(timeUnit), name]).As<long>();

	public new bool DeleteSyncPoint(long point) =>
		Call(GDExtensionMethodName.DeleteSyncPoint, [point]).As<bool>();

	public new long GetNumSyncPoints() =>
		Call(GDExtensionMethodName.GetNumSyncPoints, []).As<long>();

	public new long GetSyncPoint(long index) => 
		Call(GDExtensionMethodName.GetSyncPoint, [index]).As<long>();

	public new Godot.Collections.Dictionary GetSyncPointInfo(long point, FmodSystem.FmodTimeUnit timeUnit = FmodSystem.FmodTimeUnit.Ms) => 
		Call(GDExtensionMethodName.GetSyncPointInfo, [point, Variant.From(timeUnit)]).As<Godot.Collections.Dictionary>();

	public new long GetNumSubSounds() => 
		Call(GDExtensionMethodName.GetNumSubSounds, []).As<long>();

	public new void SetPcmreadCallback(Callable callback) => 
		Call(GDExtensionMethodName.SetPcmreadCallback, [callback]);

	public new Callable GetPcmreadCallback() => 
		Call(GDExtensionMethodName.GetPcmreadCallback, []).As<Callable>();

	public new void SetPcmsetposCallback(Callable callback) => 
		Call(GDExtensionMethodName.SetPcmsetposCallback, [callback]);

	public new Callable GetPcmsetposCallback() => 
		Call(GDExtensionMethodName.GetPcmsetposCallback, []).As<Callable>();

	public new void SetNonblockCallback(Callable callback) => 
		Call(GDExtensionMethodName.SetNonblockCallback, [callback]);

	public new Callable GetNonblockCallback() => 
		Call(GDExtensionMethodName.GetNonblockCallback, []).As<Callable>();

}
