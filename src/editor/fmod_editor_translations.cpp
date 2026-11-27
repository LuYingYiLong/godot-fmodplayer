#include "fmod_editor_translations.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/translation_domain.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/window.hpp>

namespace godot {
	void FmodEditorTranslationPlugin::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_register_translations"), &FmodEditorTranslationPlugin::_register_translations);
	}

	void FmodEditorTranslationPlugin::_enter_tree() {
		struct CatalogDirectory {
			const char* domain;
			const char* path;
		};
		constexpr CatalogDirectory directories[] = {
			{ "godot.properties", "res://addons/fmod_player/translations/properties" },
			{ "godot.editor", "res://addons/fmod_player/translations/editor" },
			{ "godot.documentation", "res://addons/fmod_player/translations/documentation" },
		};

		ResourceLoader* loader = ResourceLoader::get_singleton();
		for (const CatalogDirectory& directory : directories) {
			if (!DirAccess::dir_exists_absolute(directory.path)) {
				continue;
			}
			for (const String& filename : DirAccess::get_files_at(directory.path)) {
				if (filename.get_extension() != "po") {
					continue;
				}
				const Ref<Translation> translation = loader->load(String(directory.path).path_join(filename), "Translation");
				if (translation.is_valid()) {
					translations.push_back({ StringName(directory.domain), translation });
				}
			}
		}

		settings = EditorInterface::get_singleton()->get_editor_settings();
		if (settings.is_valid()) {
			settings->connect("settings_changed", callable_mp(this, &FmodEditorTranslationPlugin::_queue_registration));
		}
		_register_translations();
	}

	void FmodEditorTranslationPlugin::_exit_tree() {
		if (settings.is_valid()) {
			const Callable changed = callable_mp(this, &FmodEditorTranslationPlugin::_queue_registration);
			if (settings->is_connected("settings_changed", changed)) {
				settings->disconnect("settings_changed", changed);
			}
			settings.unref();
		}
		registration_queued = false;
		TranslationServer* server = TranslationServer::get_singleton();
		if (server) {
			for (const RegisteredTranslation& entry : translations) {
				server->get_or_add_domain(entry.domain)->remove_translation(entry.translation);
			}
		}
		translations.clear();
	}

	void FmodEditorTranslationPlugin::_queue_registration() {
		if (!is_inside_tree() || registration_queued) {
			return;
		}
		registration_queued = true;
		call_deferred("_register_translations");
	}

	void FmodEditorTranslationPlugin::_register_translations() {
		registration_queued = false;
		if (!is_inside_tree()) {
			return;
		}
		// Editor language setup can clear the built-in domains. Reuse our loaded catalogs.
		TranslationServer* server = TranslationServer::get_singleton();
		for (const RegisteredTranslation& entry : translations) {
			server->get_or_add_domain(entry.domain)->add_translation(entry.translation);
		}
		get_tree()->get_root()->propagate_notification(NOTIFICATION_TRANSLATION_CHANGED);
	}
}
