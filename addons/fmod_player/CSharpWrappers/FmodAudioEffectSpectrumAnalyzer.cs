#pragma warning disable CS0109
using System;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using Godot;
using Godot.Collections;

namespace FmodPlayer;

[Tool]
public partial class FmodAudioEffectSpectrumAnalyzer : FmodAudioEffect
{

	private new static readonly StringName NativeName = new StringName("FmodAudioEffectSpectrumAnalyzer");

	[Obsolete("Wrapper types cannot be constructed with constructors (it only instantiate the underlying FmodAudioEffectSpectrumAnalyzer object), please use the Instantiate() method instead.")]
	protected FmodAudioEffectSpectrumAnalyzer() { }

	private static CSharpScript _wrapperScriptAsset;

	/// <summary>
	/// Try to cast the script on the supplied <paramref name="godotObject"/> to the <see cref="FmodAudioEffectSpectrumAnalyzer"/> wrapper type,
	/// if no script has attached to the type, or the script attached to the type does not inherit the <see cref="FmodAudioEffectSpectrumAnalyzer"/> wrapper type,
	/// a new instance of the <see cref="FmodAudioEffectSpectrumAnalyzer"/> wrapper script will get attaches to the <paramref name="godotObject"/>.
	/// </summary>
	/// <remarks>The developer should only supply the <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</remarks>
	/// <param name="godotObject">The <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</param>
	/// <returns>The existing or a new instance of the <see cref="FmodAudioEffectSpectrumAnalyzer"/> wrapper script attached to the supplied <paramref name="godotObject"/>.</returns>
	public new static FmodAudioEffectSpectrumAnalyzer Bind(GodotObject godotObject)
	{
		if (!IsInstanceValid(godotObject))
			return null;

		if (godotObject is FmodAudioEffectSpectrumAnalyzer wrapperScriptInstance)
			return wrapperScriptInstance;

#if DEBUG
		var expectedType = typeof(FmodAudioEffectSpectrumAnalyzer);
		var currentObjectClassName = godotObject.GetClass();
		if (!ClassDB.IsParentClass(expectedType.Name, currentObjectClassName))
			throw new InvalidOperationException($"The supplied GodotObject ({currentObjectClassName}) is not the {expectedType.Name} type.");
#endif

		if (_wrapperScriptAsset is null)
		{
			var scriptPathAttribute = typeof(FmodAudioEffectSpectrumAnalyzer).GetCustomAttributes<ScriptPathAttribute>().FirstOrDefault();
			if (scriptPathAttribute is null) throw new UnreachableException();
			_wrapperScriptAsset = ResourceLoader.Load<CSharpScript>(scriptPathAttribute.Path);
		}

		var instanceId = godotObject.GetInstanceId();
		godotObject.SetScript(_wrapperScriptAsset);
		return (FmodAudioEffectSpectrumAnalyzer)InstanceFromId(instanceId);
	}

	/// <summary>
	/// Creates an instance of the GDExtension <see cref="FmodAudioEffectSpectrumAnalyzer"/> type, and attaches a wrapper script instance to it.
	/// </summary>
	/// <returns>The wrapper instance linked to the underlying GDExtension "FmodAudioEffectSpectrumAnalyzer" type.</returns>
	public new static FmodAudioEffectSpectrumAnalyzer Instantiate() => Bind(ClassDB.Instantiate(NativeName).As<GodotObject>());

	public bool ImmediateMode
	{
		get => Call("get_immediate_mode").As<bool>();
		set => Call("set_immediate_mode", [value]);
	}

	public void SetImmediateMode(bool enabled) => Call("set_immediate_mode", [enabled]);
	public bool GetImmediateMode() => ImmediateMode;

	public enum FftSizeEnum
	{
		Size256 = 0,
		Size512 = 1,
		Size1024 = 2,
		Size2048 = 3,
		Size4096 = 4,
		Max = 5,
	}

	public enum MagnitudeMode
	{
		Average = 0,
		Max = 1,
	}

	public new static class GDExtensionPropertyName
	{
		public new static readonly StringName BufferLength = "buffer_length";
		public new static readonly StringName FftSize = "fft_size";
	}

	public new double BufferLength
	{
		get => Get(GDExtensionPropertyName.BufferLength).As<double>();
		set => Set(GDExtensionPropertyName.BufferLength, value);
	}

	public new Variant FftSize
	{
		get => Get(GDExtensionPropertyName.FftSize).As<Variant>();
		set => Set(GDExtensionPropertyName.FftSize, value);
	}

	public new static class GDExtensionMethodName
	{
		public new static readonly StringName GetMagnitudeForFrequencyRange = "get_magnitude_for_frequency_range";
	}

	public new Vector2 GetMagnitudeForFrequencyRange(double begin, double end, FmodAudioEffectSpectrumAnalyzer.MagnitudeMode mode = FmodAudioEffectSpectrumAnalyzer.MagnitudeMode.Max) => 
		Call(GDExtensionMethodName.GetMagnitudeForFrequencyRange, [begin, end, Variant.From(mode)]).As<Vector2>();

}
