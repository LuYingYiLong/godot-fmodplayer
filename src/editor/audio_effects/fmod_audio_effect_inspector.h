#ifndef FMOD_AUDIO_EFFECT_INSPECTOR_H
#define FMOD_AUDIO_EFFECT_INSPECTOR_H

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot {
	class FmodAudioEffectInspector : public EditorInspectorPlugin {
		GDCLASS(FmodAudioEffectInspector, EditorInspectorPlugin)

	protected:
		static void _bind_methods();

	public:
		bool _can_handle(Object* p_object) const override;
		void _parse_begin(Object* p_object) override;
	};

	class FmodAudioEffectEditorPlugin : public EditorPlugin {
		GDCLASS(FmodAudioEffectEditorPlugin, EditorPlugin)

		Ref<FmodAudioEffectInspector> inspector;

	protected:
		static void _bind_methods();
		void _notification(int p_what);
	};
}

#endif // FMOD_AUDIO_EFFECT_INSPECTOR_H
