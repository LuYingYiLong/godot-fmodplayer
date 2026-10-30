#ifndef FMOD_AUDIO_PREVIEW_INSPECTOR_H
#define FMOD_AUDIO_PREVIEW_INSPECTOR_H

#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include "audio/fmod_audio_stream.h"

namespace godot {
	class FmodAudioPreviewInspector : public EditorInspectorPlugin {
		GDCLASS(FmodAudioPreviewInspector, EditorInspectorPlugin)
		bool creating_native_property = false;

	protected:
		static void _bind_methods();

	public:
		virtual bool _can_handle(Object* object) const override;
		virtual void _parse_begin(Object* object) override;
		bool _parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) override;
	};
}

#endif // !FMOD_AUDIO_PREVIEW_INSPECTOR_H
