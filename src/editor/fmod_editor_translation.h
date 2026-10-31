#ifndef FMOD_EDITOR_TRANSLATION_H
#define FMOD_EDITOR_TRANSLATION_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/translation_domain.hpp>
#include <godot_cpp/classes/translation_server.hpp>

// Keep literal source text available for extraction and automatic control translation.
#define FMOD_EDITOR_TEXT(text) text

namespace godot {

inline String fmod_editor_translate(const String& text, const StringName& context = StringName()) {
	TranslationServer* server = TranslationServer::get_singleton();
	return server ? String(server->get_or_add_domain("godot.editor")->translate(text, context)) : text;
}

inline void fmod_set_editor_tooltip(Control* control, const String& source) {
	control->set_meta("_fmod_tooltip_source", source);
	control->set_tooltip_text(fmod_editor_translate(source));
}

inline void fmod_refresh_editor_control(Control* control) {
	if (control->has_meta("_fmod_tooltip_source")) {
		control->set_tooltip_text(fmod_editor_translate(control->get_meta("_fmod_tooltip_source")));
	}
}

}

#endif
