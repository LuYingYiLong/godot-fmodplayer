#pragma warning disable CS0109
using System;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using Godot;
using Godot.Collections;

namespace FmodPlayer;

[Tool]
public partial class FmodAudioStream : Resource
{

	private new static readonly StringName NativeName = new StringName("FmodAudioStream");

	[Obsolete("Wrapper types cannot be constructed with constructors (it only instantiate the underlying FmodAudioStream object), please use the Instantiate() method instead.")]
	protected FmodAudioStream() { }

	private static CSharpScript _wrapperScriptAsset;

	/// <summary>
	/// Try to cast the script on the supplied <paramref name="godotObject"/> to the <see cref="FmodAudioStream"/> wrapper type,
	/// if no script has attached to the type, or the script attached to the type does not inherit the <see cref="FmodAudioStream"/> wrapper type,
	/// a new instance of the <see cref="FmodAudioStream"/> wrapper script will get attaches to the <paramref name="godotObject"/>.
	/// </summary>
	/// <remarks>The developer should only supply the <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</remarks>
	/// <param name="godotObject">The <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</param>
	/// <returns>The existing or a new instance of the <see cref="FmodAudioStream"/> wrapper script attached to the supplied <paramref name="godotObject"/>.</returns>
	public new static FmodAudioStream Bind(GodotObject godotObject)
	{
		if (!IsInstanceValid(godotObject))
			return null;

		if (godotObject is FmodAudioStream wrapperScriptInstance)
			return wrapperScriptInstance;

#if DEBUG
		var expectedType = typeof(FmodAudioStream);
		var currentObjectClassName = godotObject.GetClass();
		if (!ClassDB.IsParentClass(expectedType.Name, currentObjectClassName))
			throw new InvalidOperationException($"The supplied GodotObject ({currentObjectClassName}) is not the {expectedType.Name} type.");
#endif

		if (_wrapperScriptAsset is null)
		{
			var scriptPathAttribute = typeof(FmodAudioStream).GetCustomAttributes<ScriptPathAttribute>().FirstOrDefault();
			if (scriptPathAttribute is null) throw new UnreachableException();
			_wrapperScriptAsset = ResourceLoader.Load<CSharpScript>(scriptPathAttribute.Path);
		}

		var instanceId = godotObject.GetInstanceId();
		godotObject.SetScript(_wrapperScriptAsset);
		return (FmodAudioStream)InstanceFromId(instanceId);
	}

	/// <summary>
	/// Creates an instance of the GDExtension <see cref="FmodAudioStream"/> type, and attaches a wrapper script instance to it.
	/// </summary>
	/// <returns>The wrapper instance linked to the underlying GDExtension "FmodAudioStream" type.</returns>
	public new static FmodAudioStream Instantiate() => Bind(ClassDB.Instantiate(NativeName).As<GodotObject>());

	public FmodSystem.FmodResamplerMethod ResamplerMethod
	{
		get => (FmodSystem.FmodResamplerMethod)Call("get_resampler_method").As<long>();
		set => Call("set_resampler_method", [Variant.From(value)]);
	}

	public void SetResamplerMethod(FmodSystem.FmodResamplerMethod method) => Call("set_resampler_method", [Variant.From(method)]);
	public FmodSystem.FmodResamplerMethod GetResamplerMethod() => ResamplerMethod;


	public enum CreateMode
	{
		Default = 0,
		Stream = 1,
		Sample = 2,
		Loop = 4,
		LoopBidi = 8,
		Mode2D = 16,
		Mode3D = 32,
	}

	public new static class GDExtensionPropertyName
	{
		public new static readonly StringName AudioData = "audio_data";
		public new static readonly StringName ModeFlags = "mode_flags";
		public new static readonly StringName LengthOverride = "length_override";
		public new static readonly StringName FileOffset = "file_offset";
		public new static readonly StringName DecodeBufferSize = "decode_buffer_size";
		public new static readonly StringName InitialSubsound = "initial_subsound";
		public new static readonly StringName NumSubsounds = "num_subsounds";
		public new static readonly StringName InclusionList = "inclusion_list";
		public new static readonly StringName SuggestedSoundType = "suggested_sound_type";
		public new static readonly StringName DefaultFrequency = "default_frequency";
		public new static readonly StringName NumChannels = "num_channels";
		public new static readonly StringName Format = "format";
		public new static readonly StringName DlsName = "dls_name";
		public new static readonly StringName EncryptionKey = "encryption_key";
		public new static readonly StringName MaxPolyphony = "max_polyphony";
		public new static readonly StringName InitialSeekPosition = "initial_seek_position";
		public new static readonly StringName InitialSeekPostype = "initial_seek_postype";
		public new static readonly StringName IgnoreSetFileSystem = "ignore_set_file_system";
	}

	public new byte[] AudioData
	{
		get => Get(GDExtensionPropertyName.AudioData).As<byte[]>();
		set => Set(GDExtensionPropertyName.AudioData, value);
	}

	public new long ModeFlags
	{
		get => Get(GDExtensionPropertyName.ModeFlags).As<long>();
		set => Set(GDExtensionPropertyName.ModeFlags, value);
	}

	public new long LengthOverride { get => Get(GDExtensionPropertyName.LengthOverride).As<long>(); set => Set(GDExtensionPropertyName.LengthOverride, value); }
	public new long FileOffset { get => Get(GDExtensionPropertyName.FileOffset).As<long>(); set => Set(GDExtensionPropertyName.FileOffset, value); }
	public new long DecodeBufferSize { get => Get(GDExtensionPropertyName.DecodeBufferSize).As<long>(); set => Set(GDExtensionPropertyName.DecodeBufferSize, value); }
	public new long InitialSubsound { get => Get(GDExtensionPropertyName.InitialSubsound).As<long>(); set => Set(GDExtensionPropertyName.InitialSubsound, value); }
	public new long NumSubsounds { get => Get(GDExtensionPropertyName.NumSubsounds).As<long>(); set => Set(GDExtensionPropertyName.NumSubsounds, value); }
	public new int[] InclusionList { get => Get(GDExtensionPropertyName.InclusionList).As<int[]>(); set => Set(GDExtensionPropertyName.InclusionList, value); }
	public new FmodSound.FmodSoundType SuggestedSoundType { get => Get(GDExtensionPropertyName.SuggestedSoundType).As<FmodSound.FmodSoundType>(); set => Set(GDExtensionPropertyName.SuggestedSoundType, Variant.From(value)); }
	public new long DefaultFrequency { get => Get(GDExtensionPropertyName.DefaultFrequency).As<long>(); set => Set(GDExtensionPropertyName.DefaultFrequency, value); }
	public new long NumChannels { get => Get(GDExtensionPropertyName.NumChannels).As<long>(); set => Set(GDExtensionPropertyName.NumChannels, value); }
	public new FmodSound.FmodSoundFormat Format { get => Get(GDExtensionPropertyName.Format).As<FmodSound.FmodSoundFormat>(); set => Set(GDExtensionPropertyName.Format, Variant.From(value)); }
	public new string DlsName { get => Get(GDExtensionPropertyName.DlsName).As<string>(); set => Set(GDExtensionPropertyName.DlsName, value); }
	public new string EncryptionKey { get => Get(GDExtensionPropertyName.EncryptionKey).As<string>(); set => Set(GDExtensionPropertyName.EncryptionKey, value); }
	public new long MaxPolyphony { get => Get(GDExtensionPropertyName.MaxPolyphony).As<long>(); set => Set(GDExtensionPropertyName.MaxPolyphony, value); }
	public new long InitialSeekPosition { get => Get(GDExtensionPropertyName.InitialSeekPosition).As<long>(); set => Set(GDExtensionPropertyName.InitialSeekPosition, value); }
	public new FmodSystem.FmodTimeUnit InitialSeekPostype { get => Get(GDExtensionPropertyName.InitialSeekPostype).As<FmodSystem.FmodTimeUnit>(); set => Set(GDExtensionPropertyName.InitialSeekPostype, Variant.From(value)); }
	public new bool IgnoreSetFileSystem { get => Get(GDExtensionPropertyName.IgnoreSetFileSystem).As<bool>(); set => Set(GDExtensionPropertyName.IgnoreSetFileSystem, value); }

	public new static class GDExtensionMethodName
	{
		public new static readonly StringName GetSound = "get_sound";
		public new static readonly StringName Preload = "preload";
		public new static readonly StringName IsPreloaded = "is_preloaded";
		public new static readonly StringName GetLength = "get_length";
		public new static readonly StringName IsDataLoaded = "is_data_loaded";
		public new static readonly StringName Clear = "clear";
		public new static readonly StringName LoadFromFile = "load_from_file";
		public new static readonly StringName AddModeFlag = "add_mode_flag";
		public new static readonly StringName RemoveModeFlag = "remove_mode_flag";
		public new static readonly StringName HasModeFlag = "has_mode_flag";
		public new static readonly StringName InvalidateSound = "invalidate_sound";
	}

	public new FmodSound GetSound() => 
		FmodSound.Bind(Call(GDExtensionMethodName.GetSound, []).As<RefCounted>());

	public new bool Preload() =>
		Call(GDExtensionMethodName.Preload, []).As<bool>();

	public new bool IsPreloaded() =>
		Call(GDExtensionMethodName.IsPreloaded, []).As<bool>();

	public new double GetLength() => 
		Call(GDExtensionMethodName.GetLength, []).As<double>();

	public new bool IsDataLoaded() => 
		Call(GDExtensionMethodName.IsDataLoaded, []).As<bool>();

	public new void Clear() => 
		Call(GDExtensionMethodName.Clear, []);

	public new static FmodAudioStream LoadFromFile(string path, long flags = 1) => 
		FmodAudioStream.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.LoadFromFile, [path, flags]).As<Resource>());

	public new void AddModeFlag(FmodAudioStream.CreateMode flag) => 
		Call(GDExtensionMethodName.AddModeFlag, [Variant.From(flag)]);

	public new void RemoveModeFlag(FmodAudioStream.CreateMode flag) => 
		Call(GDExtensionMethodName.RemoveModeFlag, [Variant.From(flag)]);

	public new bool HasModeFlag(FmodAudioStream.CreateMode flag) => 
		Call(GDExtensionMethodName.HasModeFlag, [Variant.From(flag)]).As<bool>();

	public new void InvalidateSound() => 
		Call(GDExtensionMethodName.InvalidateSound, []);

}
