#include "fmod_audio_preview_inspector.h"
#include "fmod_audio_preview_property.h"
#include "fmod_audio_stream_property.h"
#include "nodes/fmod_audio_stream_player.h"
#include "nodes/fmod_audio_stream_player_2d.h"
#include "nodes/fmod_audio_stream_player_3d.h"
#include <godot_cpp/classes/editor_inspector.hpp>

namespace godot {
	void FmodAudioPreviewInspector::_bind_methods() {

	}

	bool FmodAudioPreviewInspector::_can_handle(Object* object) const {
		if (creating_native_property) return false;
		FmodAudioStream* stream = Object::cast_to<FmodAudioStream>(object);
		return stream != nullptr || Object::cast_to<FmodAudioStreamPlayer>(object) ||
			Object::cast_to<FmodAudioStreamPlayer2D>(object) || Object::cast_to<FmodAudioStreamPlayer3D>(object);
	}

	void FmodAudioPreviewInspector::_parse_begin(Object* object) {
		// 在检查器开始时直接添加预览控件
		FmodAudioStream* stream = Object::cast_to<FmodAudioStream>(object);
		if (stream == nullptr) {
			return;
		}

		FmodAudioPreviewProperty* preview_prop = memnew(FmodAudioPreviewProperty);
		preview_prop->set_stream(Ref<FmodAudioStream>(stream));
		add_custom_control(preview_prop);
	}

	bool FmodAudioPreviewInspector::_parse_property(Object* p_object, Variant::Type p_type, const String& p_name, PropertyHint p_hint_type, const String& p_hint_string, BitField<PropertyUsageFlags> p_usage_flags, bool p_wide) {
		if (p_name != "stream" || p_type != Variant::OBJECT || Object::cast_to<FmodAudioStream>(p_object)) return false;
		// 使用原生资源属性，让 Godot 管理子检查器及属性名称样式
		// 工厂会再次遍历插件，创建期间跳过自身以避免递归
		creating_native_property = true;
		EditorProperty* property = EditorInspector::instantiate_property_editor(p_object, p_type, p_name, p_hint_type, p_hint_string, p_usage_flags, p_wide);
		creating_native_property = false;
		if (!property) return false;
		decorate_fmod_audio_stream_property(property);
		add_property_editor(p_name, property);
		return true;
	}
} // namespace godot
