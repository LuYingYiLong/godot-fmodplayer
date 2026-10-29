#include "fmod_audio_effect_inspector.h"
#include "fmod_audio_effect_panner_preview.h"
#include "fmod_audio_effect_eq_preview.h"

namespace godot {
	void FmodAudioEffectInspector::_bind_methods() {
	}

	bool FmodAudioEffectInspector::_can_handle(Object* p_object) const {
		return Object::cast_to<FmodAudioEffectPanner>(p_object) != nullptr || Object::cast_to<FmodAudioEffectEQ>(p_object) != nullptr;
	}

	void FmodAudioEffectInspector::_parse_begin(Object* p_object) {
		FmodAudioEffectEQ* eq = Object::cast_to<FmodAudioEffectEQ>(p_object);
		if (eq) {
			FmodAudioEffectEQPreview* preview = memnew(FmodAudioEffectEQPreview);
			preview->set_effect(Ref<FmodAudioEffectEQ>(eq));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectPanner* panner = Object::cast_to<FmodAudioEffectPanner>(p_object);
		if (!panner) {
			return;
		}
		FmodAudioEffectPannerPreview* preview = memnew(FmodAudioEffectPannerPreview);
		preview->set_effect(Ref<FmodAudioEffectPanner>(panner));
		// 保留 EditorProperty 的编辑信号接入，不关联单一属性，避免生成重复的重置按钮
		add_custom_control(preview);
	}

	void FmodAudioEffectEditorPlugin::_bind_methods() {
	}

	void FmodAudioEffectEditorPlugin::_notification(int p_what) {
		if (p_what == NOTIFICATION_ENTER_TREE) {
			inspector.instantiate();
			add_inspector_plugin(inspector);
		}
		else if (p_what == NOTIFICATION_EXIT_TREE && inspector.is_valid()) {
			remove_inspector_plugin(inspector);
			inspector.unref();
		}
	}
}
