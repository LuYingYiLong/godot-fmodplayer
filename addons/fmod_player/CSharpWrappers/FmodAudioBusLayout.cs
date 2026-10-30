#pragma warning disable CS0109
using System;
using System.Diagnostics;
using System.Linq;
using System.Reflection;
using Godot;
using Godot.Collections;

namespace FmodPlayer;

[Tool]
public partial class FmodAudioBusLayout : Resource
{

	private new static readonly StringName NativeName = new StringName("FmodAudioBusLayout");

	[Obsolete("Wrapper types cannot be constructed with constructors (it only instantiate the underlying FmodAudioBusLayout object), please use the Instantiate() method instead.")]
	protected FmodAudioBusLayout() { }

	private static CSharpScript _wrapperScriptAsset;

	/// <summary>
	/// Try to cast the script on the supplied <paramref name="godotObject"/> to the <see cref="FmodAudioBusLayout"/> wrapper type,
	/// if no script has attached to the type, or the script attached to the type does not inherit the <see cref="FmodAudioBusLayout"/> wrapper type,
	/// a new instance of the <see cref="FmodAudioBusLayout"/> wrapper script will get attaches to the <paramref name="godotObject"/>.
	/// </summary>
	/// <remarks>The developer should only supply the <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</remarks>
	/// <param name="godotObject">The <paramref name="godotObject"/> that represents the correct underlying GDExtension type.</param>
	/// <returns>The existing or a new instance of the <see cref="FmodAudioBusLayout"/> wrapper script attached to the supplied <paramref name="godotObject"/>.</returns>
	public new static FmodAudioBusLayout Bind(GodotObject godotObject)
	{
		if (!IsInstanceValid(godotObject))
			return null;

		if (godotObject is FmodAudioBusLayout wrapperScriptInstance)
			return wrapperScriptInstance;

#if DEBUG
		var expectedType = typeof(FmodAudioBusLayout);
		var currentObjectClassName = godotObject.GetClass();
		if (!ClassDB.IsParentClass(expectedType.Name, currentObjectClassName))
			throw new InvalidOperationException($"The supplied GodotObject ({currentObjectClassName}) is not the {expectedType.Name} type.");
#endif

		if (_wrapperScriptAsset is null)
		{
			var scriptPathAttribute = typeof(FmodAudioBusLayout).GetCustomAttributes<ScriptPathAttribute>().FirstOrDefault();
			if (scriptPathAttribute is null) throw new UnreachableException();
			_wrapperScriptAsset = ResourceLoader.Load<CSharpScript>(scriptPathAttribute.Path);
		}

		var instanceId = godotObject.GetInstanceId();
		godotObject.SetScript(_wrapperScriptAsset);
		return (FmodAudioBusLayout)InstanceFromId(instanceId);
	}

	/// <summary>
	/// Creates an instance of the GDExtension <see cref="FmodAudioBusLayout"/> type, and attaches a wrapper script instance to it.
	/// </summary>
	/// <returns>The wrapper instance linked to the underlying GDExtension "FmodAudioBusLayout" type.</returns>
	public new static FmodAudioBusLayout Instantiate() => Bind(ClassDB.Instantiate(NativeName).As<GodotObject>());

	public new static class GDExtensionMethodName
	{
		public new static readonly StringName GetBusCount = "get_bus_count";
		public new static readonly StringName GetBusIds = "get_bus_ids";
		public new static readonly StringName FindBusId = "find_bus_id";
		public new static readonly StringName GetBusName = "get_bus_name";
		public new static readonly StringName AddBus = "add_bus";
		public new static readonly StringName RemoveBus = "remove_bus";
		public new static readonly StringName MoveBus = "move_bus";
		public new static readonly StringName RenameBus = "rename_bus";
		public new static readonly StringName CanSendTo = "can_send_to";
		public new static readonly StringName SetBusSend = "set_bus_send";
		public new static readonly StringName GetBusSend = "get_bus_send";
		public new static readonly StringName SetBusVolumeDb = "set_bus_volume_db";
		public new static readonly StringName GetBusVolumeDb = "get_bus_volume_db";
		public new static readonly StringName SetBusMute = "set_bus_mute";
		public new static readonly StringName IsBusMute = "is_bus_mute";
		public new static readonly StringName SetBusSolo = "set_bus_solo";
		public new static readonly StringName IsBusSolo = "is_bus_solo";
		public new static readonly StringName SetBusBypass = "set_bus_bypass";
		public new static readonly StringName IsBusBypass = "is_bus_bypass";
		public new static readonly StringName AddBusEffect = "add_bus_effect";
		public new static readonly StringName RemoveBusEffect = "remove_bus_effect";
		public new static readonly StringName MoveBusEffect = "move_bus_effect";
		public new static readonly StringName SetBusEffectEnabled = "set_bus_effect_enabled";
		public new static readonly StringName IsBusEffectEnabled = "is_bus_effect_enabled";
		public new static readonly StringName GetBusEffect = "get_bus_effect";
		public new static readonly StringName GetBusEffectIds = "get_bus_effect_ids";
		public new static readonly StringName SetState = "set_state";
		public new static readonly StringName GetState = "get_state";
		public new static readonly StringName Validate = "validate";
	}

	public new long GetBusCount() =>
		Call(GDExtensionMethodName.GetBusCount, []).As<long>();

	public new long[] GetBusIds() =>
		Call(GDExtensionMethodName.GetBusIds, []).As<long[]>();

	public new long FindBusId(StringName name) =>
		Call(GDExtensionMethodName.FindBusId, [name]).As<long>();

	public new StringName GetBusName(long id) =>
		Call(GDExtensionMethodName.GetBusName, [id]).As<StringName>();

	public new long AddBus(StringName name, long sendId = 0) =>
		Call(GDExtensionMethodName.AddBus, [name, sendId]).As<long>();

	public new Error RemoveBus(long id) =>
		(Error)Call(GDExtensionMethodName.RemoveBus, [id]).AsInt64();

	public new Error MoveBus(long id, long index) =>
		(Error)Call(GDExtensionMethodName.MoveBus, [id, index]).AsInt64();

	public new Error RenameBus(long id, StringName name) =>
		(Error)Call(GDExtensionMethodName.RenameBus, [id, name]).AsInt64();

	public new bool CanSendTo(long id, long target) =>
		Call(GDExtensionMethodName.CanSendTo, [id, target]).As<bool>();

	public new Error SetBusSend(long id, long target) =>
		(Error)Call(GDExtensionMethodName.SetBusSend, [id, target]).AsInt64();

	public new long GetBusSend(long id) =>
		Call(GDExtensionMethodName.GetBusSend, [id]).As<long>();

	public new void SetBusVolumeDb(long id, double volumeDb) =>
		Call(GDExtensionMethodName.SetBusVolumeDb, [id, volumeDb]);

	public new double GetBusVolumeDb(long id) =>
		Call(GDExtensionMethodName.GetBusVolumeDb, [id]).As<double>();

	public new void SetBusMute(long id, bool mute) =>
		Call(GDExtensionMethodName.SetBusMute, [id, mute]);

	public new bool IsBusMute(long id) =>
		Call(GDExtensionMethodName.IsBusMute, [id]).As<bool>();

	public new void SetBusSolo(long id, bool solo) =>
		Call(GDExtensionMethodName.SetBusSolo, [id, solo]);

	public new bool IsBusSolo(long id) =>
		Call(GDExtensionMethodName.IsBusSolo, [id]).As<bool>();

	public new void SetBusBypass(long id, bool bypass) =>
		Call(GDExtensionMethodName.SetBusBypass, [id, bypass]);

	public new bool IsBusBypass(long id) =>
		Call(GDExtensionMethodName.IsBusBypass, [id]).As<bool>();

	public new long AddBusEffect(long busId, FmodAudioEffect effect, long index = -1) =>
		Call(GDExtensionMethodName.AddBusEffect, [busId, effect, index]).As<long>();

	public new Error RemoveBusEffect(long busId, long slotId) =>
		(Error)Call(GDExtensionMethodName.RemoveBusEffect, [busId, slotId]).AsInt64();

	public new Error MoveBusEffect(long busId, long slotId, long targetBus, long index) =>
		(Error)Call(GDExtensionMethodName.MoveBusEffect, [busId, slotId, targetBus, index]).AsInt64();

	public new void SetBusEffectEnabled(long busId, long slotId, bool enabled) =>
		Call(GDExtensionMethodName.SetBusEffectEnabled, [busId, slotId, enabled]);

	public new bool IsBusEffectEnabled(long busId, long slotId) =>
		Call(GDExtensionMethodName.IsBusEffectEnabled, [busId, slotId]).As<bool>();

	public new FmodAudioEffect GetBusEffect(long busId, long slotId) =>
		FmodAudioEffect.Bind(Call(GDExtensionMethodName.GetBusEffect, [busId, slotId]).As<GodotObject>());

	public new long[] GetBusEffectIds(long busId) =>
		Call(GDExtensionMethodName.GetBusEffectIds, [busId]).As<long[]>();

	public new Error SetState(Godot.Collections.Array state) =>
		(Error)Call(GDExtensionMethodName.SetState, [state]).AsInt64();

	public new Godot.Collections.Array GetState() =>
		Call(GDExtensionMethodName.GetState, []).As<Godot.Collections.Array>();

	public new Error Validate() =>
		(Error)Call(GDExtensionMethodName.Validate, []).AsInt64();

}
