#ifndef FMOD_EDITOR_TRANSLATIONS_H
#define FMOD_EDITOR_TRANSLATIONS_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/translation.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

	class FmodEditorTranslationPlugin : public EditorPlugin {
		GDCLASS(FmodEditorTranslationPlugin, EditorPlugin)

		struct RegisteredTranslation {
			StringName domain;
			Ref<Translation> translation;
		};

		Vector<RegisteredTranslation> translations;
		Ref<EditorSettings> settings;
		bool registration_queued = false;

		void _queue_registration();
		void _register_translations();

	protected:
		static void _bind_methods();

	public:
		void _enter_tree() override;
		void _exit_tree() override;
	};

}

#endif // FMOD_EDITOR_TRANSLATIONS_H
