#pragma warning disable CS0109
using System;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using Godot;
using Godot.Collections;

namespace FmodPlayer;

[Tool]
public partial class FmodServer : GodotObject
{

	private new static readonly StringName NativeName = new StringName("FmodServer");

	[Obsolete("Wrapper types cannot be constructed with constructors (it only instantiate the underlying FmodServer object), please use the Instantiate() method instead.")]
	protected FmodServer() { }

	private static CSharpScript _wrapperScriptAsset;

	/// <summary>
	/// Try to cast the script on the supplied <paramref name="godotObject"/> to the <see cref="FmodServer"/> wrapper type,
	/// if no script has attached to the type, or the script attached to the type does not inherit the <see cref="FmodServer"/> wrapper type,
	/// a new instance of the <see cref="FmodServer"/> wrapper script will get attaches to the <paramref name="godotObject"/>.
	/// </summary>
	/// <remarks>The developer should only supply the <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</remarks>
	/// <param name="godotObject">The <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</param>
	/// <returns>The existing or a new instance of the <see cref="FmodServer"/> wrapper script attached to the supplied <paramref name="godotObject"/>.</returns>
	public new static FmodServer Bind(GodotObject godotObject)
	{
		if (!IsInstanceValid(godotObject))
			return null;

		if (godotObject is FmodServer wrapperScriptInstance)
			return wrapperScriptInstance;

#if DEBUG
		var expectedType = typeof(FmodServer);
		var currentObjectClassName = godotObject.GetClass();
		if (!ClassDB.IsParentClass(expectedType.Name, currentObjectClassName))
			throw new InvalidOperationException($"The supplied GodotObject ({currentObjectClassName}) is not the {expectedType.Name} type.");
#endif

		if (_wrapperScriptAsset is null)
		{
			var scriptPathAttribute = typeof(FmodServer).GetCustomAttributes<ScriptPathAttribute>().FirstOrDefault();
			if (scriptPathAttribute is null) throw new UnreachableException();
			_wrapperScriptAsset = ResourceLoader.Load<CSharpScript>(scriptPathAttribute.Path);
		}

		var instanceId = godotObject.GetInstanceId();
		godotObject.SetScript(_wrapperScriptAsset);
		return (FmodServer)InstanceFromId(instanceId);
	}

	/// <summary>
	/// Creates an instance of the GDExtension <see cref="FmodServer"/> type, and attaches a wrapper script instance to it.
	/// </summary>
	/// <returns>The wrapper instance linked to the underlying GDExtension "FmodServer" type.</returns>
	public new static FmodServer Instantiate() => Bind(ClassDB.Instantiate(NativeName).As<GodotObject>());

	public new static class GDExtensionMethodName
	{
		public new static readonly StringName GetMainSystem = "get_main_system";
		public new static readonly StringName GetMasterChannelGroup = "get_master_channel_group";
		public new static readonly StringName GetAudioBusLayout = "get_audio_bus_layout";
		public new static readonly StringName SetAudioBusLayout = "set_audio_bus_layout";
		public new static readonly StringName GetAudioBus = "get_audio_bus";
		public new static readonly StringName GetAudioBusById = "get_audio_bus_by_id";
		public new static readonly StringName GetBusEffectInstance = "get_bus_effect_instance";
		public new static readonly StringName GetBusNames = "get_bus_names";
		public new static readonly StringName IsSyncingGodotBuses = "is_syncing_godot_buses";
		public new static readonly StringName FlushBusUpdates = "flush_bus_updates";
		public new static readonly StringName CreateLayoutFromGodot = "create_layout_from_godot";
		public new static readonly StringName GenerateBusLayout = "generate_bus_layout";
		public new static readonly StringName ResetMainSystem = "reset_main_system";
	}

	public static class GDExtensionSignalName
	{
		public static readonly StringName BusLayoutChanged = "bus_layout_changed";
	}

	public new static FmodSystem GetMainSystem() =>
		FmodSystem.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetMainSystem, []).As<GodotObject>());

	public new static FmodChannelGroup GetMasterChannelGroup() =>
		FmodChannelGroup.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetMasterChannelGroup, []).As<GodotObject>());

	public new static FmodAudioBusLayout GetAudioBusLayout() =>
		FmodAudioBusLayout.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetAudioBusLayout, []).As<GodotObject>());

	public new static Error SetAudioBusLayout(FmodAudioBusLayout layout) =>
		(Error)ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.SetAudioBusLayout, [layout]).AsInt64();

	public new static FmodAudioBus GetAudioBus(StringName name) =>
		FmodAudioBus.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetAudioBus, [name]).As<GodotObject>());

	public new static FmodAudioBus GetAudioBusById(long id) =>
		FmodAudioBus.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetAudioBusById, [id]).As<GodotObject>());

	public new static FmodAudioEffect GetBusEffectInstance(long busId, long slotId) =>
		FmodAudioEffect.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetBusEffectInstance, [busId, slotId]).As<GodotObject>());

	public new static string[] GetBusNames() =>
		ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GetBusNames, []).As<string[]>();

	public new static bool IsSyncingGodotBuses() =>
		ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.IsSyncingGodotBuses, []).As<bool>();

	public new static void FlushBusUpdates() =>
		ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.FlushBusUpdates, []);

	public new static FmodAudioBusLayout CreateLayoutFromGodot() =>
		FmodAudioBusLayout.Bind(ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.CreateLayoutFromGodot, []).As<GodotObject>());

	public new static void GenerateBusLayout() =>
		ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.GenerateBusLayout, []);

	public new static void ResetMainSystem(FmodSystem system) =>
		ClassDB.ClassCallStatic(NativeName, GDExtensionMethodName.ResetMainSystem, [system]);

}
