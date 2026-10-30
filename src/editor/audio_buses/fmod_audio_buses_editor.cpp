#include "fmod_audio_buses_editor.h"
#include "core/fmod_server.h"
#include "editor/audio_effects/fmod_audio_effect_capture_preview.h"
#include "editor/audio_effects/fmod_audio_effect_spectrum_analyzer_preview.h"
#include "mixer/fmod_audio_bus_runtime.h"
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_inspector.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	static String resolve_path(const String& path) {
		if (!path.begins_with("uid://")) {
			return path;
		}
		const int64_t uid = ResourceUID::get_singleton()->text_to_id(path);
		return ResourceUID::get_singleton()->has_id(uid) ? ResourceUID::get_singleton()->get_id_path(uid) : String();
	}

	static String path_reference(const String& path) {
		const int64_t uid = ResourceLoader::get_singleton()->get_resource_uid(path);
		return uid == ResourceUID::INVALID_ID ? path : ResourceUID::get_singleton()->id_to_text(uid);
	}

	static String default_path() {
		return ProjectSettings::get_singleton()->get_setting_with_override("audio/fmod/default_bus_layout");
	}

	void FmodAudioBusesDock::_bind_methods() {
		ClassDB::bind_method(D_METHOD("get_layout"), &FmodAudioBusesDock::get_layout);
		ClassDB::bind_method(D_METHOD("save_layout", "path"), &FmodAudioBusesDock::save_layout, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("open_layout", "path"), &FmodAudioBusesDock::open_layout);
		ClassDB::bind_method(D_METHOD("has_unsaved_changes"), &FmodAudioBusesDock::has_unsaved_changes);
		ClassDB::bind_method(D_METHOD("get_edited_path"), &FmodAudioBusesDock::get_edited_path);
	}

	FmodAudioBusesDock::FmodAudioBusesDock() {
		set_v_size_flags(SIZE_EXPAND_FILL);
		set_name("FmodAudioBuses");
		set_title("FMOD Audio");
		set_layout_key("FmodAudioBuses");
		set_translation_domain("godot.editor");
		set_auto_translate_mode(AUTO_TRANSLATE_MODE_ALWAYS);
		set_default_slot(DOCK_SLOT_BOTTOM);
		set_available_layouts(DOCK_LAYOUT_HORIZONTAL | DOCK_LAYOUT_FLOATING);
		set_icon_name("AudioStreamPlayer");
		Ref<Shortcut> shortcut;
		shortcut.instantiate();
		Ref<InputEventKey> key;
		key.instantiate();
		key->set_keycode(KEY_A);
		key->set_alt_pressed(true);
		key->set_shift_pressed(true);
		Array keys;
		keys.push_back(key);
		shortcut->set_events(keys);
		set_dock_shortcut(shortcut);
		auto vb = memnew(VBoxContainer);
		add_child(vb);
		vb->set_v_size_flags(SIZE_EXPAND_FILL);
		toolbar = memnew(HBoxContainer);
		vb->add_child(toolbar);
		auto caption = memnew(Label);
		caption->set_text("Layout:");
		toolbar->add_child(caption);
		file_label = memnew(Label);
		file_label->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		file_label->set_mouse_filter(MOUSE_FILTER_PASS);
		file_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
		file_label->set_h_size_flags(SIZE_EXPAND_FILL);
		toolbar->add_child(file_label);
		const char* names[] = { "Add Bus", "Load", "Save As", "Load Default", "Create" };
		const char* tooltips[] = { "Add a new Audio Bus to this layout.", "Load an existing Bus Layout.", "Save this Bus Layout to a file.", "Load the default Bus Layout.", "Create a new Bus Layout." };
		for (int i = 0; i < 5; i++) {
			if (i == 1) {
				toolbar->add_child(memnew(VSeparator));
			}
			auto button = memnew(Button);
			button->set_text(names[i]);
			button->set_tooltip_text(tooltips[i]);
			toolbar->add_child(button);
			button->connect("pressed", callable_mp(this, &FmodAudioBusesDock::_file_command).bind(i));
			editing_buttons.push_back(button);
		}
		extra_menu = memnew(MenuButton);
		extra_menu->set_text("FMOD");
		extra_menu->set_flat(false);
		toolbar->add_child(extra_menu);
		extra_menu->get_popup()->add_item("Set as Default", 5);
		extra_menu->get_popup()->add_item("Create FMOD Layout from Godot", 6);
		extra_menu->get_popup()->add_item("Open Godot Audio Panel", 7);
		extra_menu->get_popup()->connect("id_pressed", callable_mp(this, &FmodAudioBusesDock::_file_command));
		margin = memnew(MarginContainer);
		margin->set_name("Body");
		margin->set_theme_type_variation("NoBorderBottomPanel");
		margin->set_v_size_flags(SIZE_EXPAND_FILL);
		vb->add_child(margin);
		scroll = memnew(ScrollContainer);
		scroll->set_name("BusPanel");
		scroll->set_custom_minimum_size(Vector2(0.0f, 40.0f * EditorInterface::get_singleton()->get_editor_scale()));
		scroll->set_scroll_hint_mode(ScrollContainer::SCROLL_HINT_MODE_ALL);
		margin->add_child(scroll);
		rows = memnew(HBoxContainer);
		rows->set_v_size_flags(SIZE_EXPAND_FILL);
		scroll->add_child(rows);
		status = memnew(Label);
		status->set_name("SyncHint");
		status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		status->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		status->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
		status->set_mouse_filter(MOUSE_FILTER_IGNORE);
		status->set_custom_minimum_size(Vector2(0.0f, 40.0f * EditorInterface::get_singleton()->get_editor_scale()));
		margin->add_child(status);
		save_timer = memnew(Timer);
		save_timer->set_wait_time(0.8);
		save_timer->set_one_shot(true);
		save_timer->connect("timeout", callable_mp(this, &FmodAudioBusesDock::_save_timeout));
		add_child(save_timer);
		meter_timer = memnew(Timer);
		meter_timer->set_wait_time(1.0 / 30.0);
		meter_timer->connect("timeout", callable_mp(this, &FmodAudioBusesDock::_meter_timeout));
		add_child(meter_timer);
		file_dialog = memnew(EditorFileDialog);
		file_dialog->add_filter("*.tres,*.res", "FMOD Bus Layout");
		file_dialog->connect("file_selected", callable_mp(this, &FmodAudioBusesDock::_file_chosen));
		add_child(file_dialog);
		error_dialog = memnew(AcceptDialog);
		error_dialog->set_title("FMOD Audio");
		add_child(error_dialog);
		connect("visibility_changed", callable_mp(this, &FmodAudioBusesDock::_visibility_changed));
	}

	FmodAudioBusesDock::~FmodAudioBusesDock() {
		_unobserve_effects();
		if (layout.is_valid() && layout->is_connected("changed", callable_mp(this, &FmodAudioBusesDock::_changed))) {
			layout->disconnect("changed", callable_mp(this, &FmodAudioBusesDock::_changed));
		}
	}

	bool FmodAudioBusesDock::is_read_only() const {
		return FmodServer::is_syncing_godot_buses();
	}

	void FmodAudioBusesDock::_notification(int what) {
		if ((what == NOTIFICATION_THEME_CHANGED || what == NOTIFICATION_READY) && scroll) {
			// The native Audio panel gets its inner padding and background from Tree.
			scroll->add_theme_stylebox_override("panel", get_theme_stylebox("panel", "Tree"));
		}
		if (what == NOTIFICATION_READY) {
			if (layout.is_null()) {
				_activate(FmodServer::get_audio_bus_layout(), is_read_only() ? String() : default_path());
			}
			if (!is_read_only() && layout->get_path().is_empty() && ResourceLoader::get_singleton()->exists(default_path())) {
				save_blocked = true;
				_show_error(fmod_editor_translate("The default FMOD bus layout could not be loaded. Create or open a valid layout before saving."));
			}
			FmodServer::get_singleton()->connect("bus_layout_changed", callable_mp(this, &FmodAudioBusesDock::_server_changed));
			_visibility_changed();
		}
		else if (what == NOTIFICATION_EXIT_TREE) {
			if (unsaved && !save_blocked) {
				save_layout();
			}
			meter_timer->stop();
			save_timer->stop();
			if (FmodServer::get_singleton() && FmodServer::get_singleton()->is_connected("bus_layout_changed", callable_mp(this, &FmodAudioBusesDock::_server_changed))) {
				FmodServer::get_singleton()->disconnect("bus_layout_changed", callable_mp(this, &FmodAudioBusesDock::_server_changed));
			}
		}
		else if (what == NOTIFICATION_TRANSLATION_CHANGED && file_label) {
			_refresh_header();
			if (layout.is_valid()) {
				for (int i = 0; i < rows->get_child_count(); i++) {
					auto row = Object::cast_to<FmodAudioBusEditor>(rows->get_child(i));
					if (row) {
						row->refresh();
					}
				}
			}
		}
	}

	void FmodAudioBusesDock::_activate(const Ref<FmodAudioBusLayout>& data, const String& path) {
		if (data.is_null()) {
			return;
		}
		loading = true;
		_unobserve_effects();
		if (layout.is_valid() && layout->is_connected("changed", callable_mp(this, &FmodAudioBusesDock::_changed))) {
			layout->disconnect("changed", callable_mp(this, &FmodAudioBusesDock::_changed));
		}
		layout = data;
		edited_path = path;
		unsaved = false;
		save_blocked = false;
		save_timer->stop();
		layout->connect("changed", callable_mp(this, &FmodAudioBusesDock::_changed));
		for (int i = rows->get_child_count() - 1; i >= 0; i--) {
			Node* child = rows->get_child(i);
			rows->remove_child(child);
			child->queue_free();
		}
		loading = false;
		_rebuild();
	}

	void FmodAudioBusesDock::_refresh_header() {
		if (is_read_only()) {
			file_label->set_text(fmod_editor_translate("Godot Audio Buses"));
			status->set_text(fmod_editor_translate("Synchronizing with Godot Audio Buses. Edit the source layout in the Godot Audio panel."));
		}
		else {
			const String path = resolve_path(edited_path);
			file_label->set_text((unsaved ? "(*) " : "") + (path.is_empty() ? edited_path : path.get_file()));
			file_label->set_tooltip_text(path.is_empty() ? edited_path : path);
			status->set_text("");
		}
		status->set_visible(is_read_only());
		scroll->set_visible(!is_read_only());
		for (auto button : editing_buttons) {
			button->set_disabled(is_read_only());
		}
		extra_menu->get_popup()->set_item_disabled(0, is_read_only());
		_visibility_changed();
	}

	void FmodAudioBusesDock::_unobserve_effects() {
		for (int i = 0; i < int(observed_effects.size()); i++) {
			if (observed_effects[i]->is_connected("changed", observers[i])) {
				observed_effects[i]->disconnect("changed", observers[i]);
			}
		}
		observed_effects.clear();
		observers.clear();
	}

	void FmodAudioBusesDock::_observe_effects() {
		_unobserve_effects();
		std::set<uint64_t> seen;
		for (const auto& bus : layout->get_buses()) {
			for (const auto& slot : bus.effects) {
				if (slot.effect.is_valid() && seen.insert(slot.effect->get_instance_id()).second) {
					auto callback = callable_mp(this, &FmodAudioBusesDock::_effect_changed);
					slot.effect->connect("changed", callback);
					observed_effects.push_back(slot.effect);
					observers.push_back(callback);
				}
			}
		}
	}

	void FmodAudioBusesDock::_changed() {
		if (!loading && !is_read_only()) {
			unsaved = true;
			if (!save_blocked) {
				save_timer->start();
			}
		}
		if (!refresh_pending) {
			refresh_pending = true;
			callable_mp(this, &FmodAudioBusesDock::_rebuild).call_deferred();
		}
	}

	void FmodAudioBusesDock::_effect_changed() {
		if (!is_read_only()) {
			unsaved = true;
			if (!save_blocked) {
				save_timer->start();
			}
			_refresh_header();
		}
	}

	void FmodAudioBusesDock::_rebuild() {
		refresh_pending = false;
		if (layout.is_null()) {
			return;
		}
		auto ids = layout->get_bus_ids();
		bool rebuild = rows->get_child_count() != ids.size();
		if (!rebuild) {
			for (int i = 0; i < ids.size(); i++) {
				if (rows->get_child(i)->get_meta("bus_id", -1) != Variant(ids[i])) {
					rebuild = true;
					break;
				}
			}
		}
		if (rebuild) {
			for (int i = rows->get_child_count() - 1; i >= 0; i--) {
				Node* node = rows->get_child(i);
				rows->remove_child(node);
				node->queue_free();
			}
			for (int i = 0; i < ids.size(); i++) {
				auto row = memnew(FmodAudioBusEditor);
				row->set_meta("bus_id", ids[i]);
				rows->add_child(row);
				row->setup(this, ids[i]);
			}
		}
		else {
			for (int i = 0; i < rows->get_child_count(); i++) {
				Object::cast_to<FmodAudioBusEditor>(rows->get_child(i))->refresh();
			}
		}
		_observe_effects();
		_refresh_header();
	}

	void FmodAudioBusesDock::_server_changed() {
		if (layout != FmodServer::get_audio_bus_layout()) {
			_activate(FmodServer::get_audio_bus_layout(), is_read_only() ? String() : FmodServer::get_audio_bus_layout()->get_path());
		}
		else if (is_read_only()) {
			_changed();
		}
	}

	void FmodAudioBusesDock::commit_state(const String& action, const Array& before, const Array& after) {
		if (is_read_only() || before == after) {
			return;
		}
		auto undo = EditorInterface::get_singleton()->get_editor_undo_redo();
		undo->create_action(fmod_editor_translate(action), UndoRedo::MERGE_DISABLE, layout.ptr());
		undo->add_do_method(layout.ptr(), "set_state", after.duplicate(true));
		undo->add_undo_method(layout.ptr(), "set_state", before.duplicate(true));
		undo->commit_action();
	}

	void FmodAudioBusesDock::commit_edit(const String& action, const Ref<FmodAudioBusLayout>& edited) {
		commit_state(action, layout->get_state(), edited->get_state());
	}

	void FmodAudioBusesDock::add_bus() {
		if (is_read_only()) {
			return;
		}
		Ref<FmodAudioBusLayout> copy = layout->duplicate(false);
		String name = "New Bus";
		int n = 2;
		while (copy->find_bus_id(name) >= 0) {
			name = "New Bus " + String::num_int64(n++);
		}
		copy->add_bus(name);
		commit_edit("Add Audio Bus", copy);
	}

	void FmodAudioBusesDock::select_effect(int64_t bus, int64_t slot) {
		auto effect = layout->get_bus_effect(bus, slot);
		if (effect.is_null()) {
			return;
		}
		EditorInterface::get_singleton()->edit_resource(effect);
		callable_mp(this, &FmodAudioBusesDock::_bind_preview_runtime).bind(bus, slot).call_deferred();
	}

	void FmodAudioBusesDock::_bind_preview_runtime(int64_t bus, int64_t slot) {
		FmodServer::flush_bus_updates();
		auto inspector = EditorInterface::get_singleton()->get_inspector();
		if (inspector->get_edited_object() != layout->get_bus_effect(bus, slot).ptr()) {
			return;
		}
		// Assign the selected live slot only to data previews; parameter controls edit the resource.
		auto runtime = FmodServer::get_bus_effect_instance(bus, slot);
		const auto capture = inspector->find_children("*", "FmodAudioEffectCapturePreview", true, false);
		for (int i = 0; i < capture.size(); i++) {
			Object::cast_to<FmodAudioEffectCapturePreview>(capture[i])->set_runtime_effect(runtime);
		}
		const auto spectrum = inspector->find_children("*", "FmodAudioEffectSpectrumAnalyzerPreview", true, false);
		for (int i = 0; i < spectrum.size(); i++) {
			Object::cast_to<FmodAudioEffectSpectrumAnalyzerPreview>(spectrum[i])->set_runtime_effect(runtime);
		}
	}

	bool FmodAudioBusesDock::can_drop_effect(const Variant& data) const {
		if (is_read_only() || data.get_type() != Variant::DICTIONARY) {
			return false;
		}
		Dictionary d = data;
		String type = d.get("type", "");
		if (type == "resource") {
			Ref<FmodAudioEffect> effect = d.get("resource", Variant());
			return effect.is_valid();
		}
		if (type == "files") {
			PackedStringArray files = d.get("files", PackedStringArray());
			if (files.size() != 1) {
				return false;
			}
			Ref<FmodAudioEffect> effect = ResourceLoader::get_singleton()->load(files[0]);
			return effect.is_valid();
		}
		return type == "fmod_bus_effect" && uint64_t(d.get("layout", 0)) == layout->get_instance_id() && layout->get_bus_effect(d.get("bus", -1), d.get("slot", -1)).is_valid();
	}

	void FmodAudioBusesDock::drop_effect(int64_t bus, int index, const Variant& data) {
		if (!can_drop_effect(data)) {
			return;
		}
		Dictionary d = data;
		Ref<FmodAudioBusLayout> copy = layout->duplicate(false);
		String type = d.get("type", "");
		index = MIN(index, int(copy->get_bus_effect_ids(bus).size()));
		if (type == "fmod_bus_effect") {
			if (Input::get_singleton()->is_key_pressed(KEY_CTRL) || Input::get_singleton()->is_key_pressed(KEY_META)) {
				Ref<FmodAudioEffect> effect = copy->get_bus_effect(d["bus"], d["slot"])->duplicate(true);
				copy->add_bus_effect(bus, effect, index);
			}
			else {
				copy->move_bus_effect(d["bus"], d["slot"], bus, index);
			}
		}
		else {
			Ref<FmodAudioEffect> effect;
			if (type == "resource") {
				effect = d["resource"];
			}
			else {
				PackedStringArray paths = d["files"];
				effect = ResourceLoader::get_singleton()->load(paths[0]);
			}
			copy->add_bus_effect(bus, effect, index);
		}
		commit_edit("Move Bus Effect", copy);
	}

	void FmodAudioBusesDock::_file_command(int command) {
		if (command == 7) {
			_open_godot();
			return;
		}
		if (command == 0) {
			add_bus();
			return;
		}
		if (command == 3) {
			if (open_layout(default_path()) != OK) {
				_show_error(fmod_editor_translate("Cannot load the default FMOD bus layout."));
			}
			return;
		}
		if (command == 5) {
			_set_default();
			return;
		}
		if (command != 2 && unsaved && save_layout() != OK) {
			return;
		}
		file_action = command;
		file_dialog->set_file_mode(command == 1 ? EditorFileDialog::FILE_MODE_OPEN_FILE : EditorFileDialog::FILE_MODE_SAVE_FILE);
		file_dialog->set_title(command == 1 ? "Load FMOD Bus Layout" : "Save FMOD Bus Layout");
		file_dialog->set_current_path(command == 2 ? resolve_path(edited_path) : "res://new_fmod_bus_layout.tres");
		file_dialog->popup_file_dialog();
	}

	void FmodAudioBusesDock::_file_chosen(const String& path) {
		if (file_action == 1) {
			if (is_read_only()) {
				_show_error(fmod_editor_translate("Disable Godot bus synchronization and reload the project to edit an FMOD layout."));
				return;
			}
			if (open_layout(path) != OK) {
				_show_error(fmod_editor_translate("Cannot load FMOD bus layout: %s").replace("%s", path));
			}
			return;
		}
		if (file_action == 2) {
			if (!is_read_only()) {
				save_layout(path);
			}
			return;
		}
		Ref<FmodAudioBusLayout> resource;
		resource.instantiate();
		PackedStringArray skipped;
		if (file_action == 6) {
			Ref<FmodAudioBusRuntime> converter;
			converter.instantiate();
			resource = converter->import_godot(&skipped);
		}
		const Error error = ResourceSaver::get_singleton()->save(resource, path, ResourceSaver::FLAG_CHANGE_PATH);
		if (error != OK) {
			_show_error(fmod_editor_translate("Cannot save FMOD bus layout: %s").replace("%s", path));
			return;
		}
		if (!is_read_only()) {
			open_layout(path);
		}
		else {
			EditorInterface::get_singleton()->get_resource_filesystem()->scan();
		}
		if (!skipped.is_empty()) {
			_show_error(fmod_editor_translate("Unsupported Godot effects were skipped:\n%s").replace("%s", String("\n").join(skipped)));
		}
	}

	Error FmodAudioBusesDock::open_layout(const String& path) {
		if (is_read_only()) {
			return ERR_BUSY;
		}
		if (unsaved && save_layout() != OK) {
			return ERR_CANT_CREATE;
		}
		const String resolved = resolve_path(path);
		if (resolved.is_empty() || !ResourceLoader::get_singleton()->exists(resolved)) {
			return ERR_FILE_NOT_FOUND;
		}
		Ref<FmodAudioBusLayout> resource = ResourceLoader::get_singleton()->load(resolved, "FmodAudioBusLayout", ResourceLoader::CACHE_MODE_IGNORE_DEEP);
		if (resource.is_null() || resource->validate() != OK) {
			return ERR_INVALID_DATA;
		}
		const Error error = FmodServer::set_audio_bus_layout(resource);
		if (error != OK) {
			return error;
		}
		_activate(resource, path_reference(resolved));
		return OK;
	}

	Error FmodAudioBusesDock::save_layout(const String& destination) {
		if (is_read_only()) {
			return ERR_BUSY;
		}
		if (save_blocked && destination.is_empty()) {
			return ERR_FILE_CORRUPT;
		}
		String path = resolve_path(destination.is_empty() ? edited_path : destination);
		if (path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}
		if (layout->validate() != OK) {
			return ERR_INVALID_DATA;
		}
		const Error error = ResourceSaver::get_singleton()->save(layout, path, ResourceSaver::FLAG_CHANGE_PATH);
		if (error != OK) {
			unsaved = true;
			_show_error(fmod_editor_translate("Cannot save FMOD bus layout: %s").replace("%s", path));
			return error;
		}
		const String old_default = resolve_path(default_path());
		edited_path = path_reference(path);
		unsaved = false;
		save_blocked = false;
		save_timer->stop();
		if (old_default == path) {
			ProjectSettings::get_singleton()->set_setting("audio/fmod/default_bus_layout", edited_path);
			ProjectSettings::get_singleton()->save();
		}
		_refresh_header();
		return OK;
	}

	void FmodAudioBusesDock::_set_default() {
		if (save_layout() != OK) {
			return;
		}
		ProjectSettings::get_singleton()->set_setting("audio/fmod/default_bus_layout", edited_path);
		ProjectSettings::get_singleton()->save();
	}

	void FmodAudioBusesDock::_save_timeout() {
		if (unsaved && !save_blocked) {
			save_layout();
		}
	}

	void FmodAudioBusesDock::_show_error(const String& message) {
		error_dialog->set_text(message);
		if (is_inside_tree()) {
			error_dialog->popup_centered();
		}
	}

	void FmodAudioBusesDock::_visibility_changed() {
		if (!meter_timer || !is_inside_tree()) {
			return;
		}
		if (is_visible_in_tree() && scroll->is_visible_in_tree()) {
			meter_timer->start();
		}
		else {
			meter_timer->stop();
		}
	}

	void FmodAudioBusesDock::_meter_timeout() {
		for (int i = 0; i < rows->get_child_count(); i++) {
			auto row = Object::cast_to<FmodAudioBusEditor>(rows->get_child(i));
			if (row && row->meter) {
				row->meter->sample(1.0f / 30.0f);
			}
		}
	}

	void FmodAudioBusesDock::_open_godot() {
		const auto nodes = EditorInterface::get_singleton()->get_base_control()->find_children("*", "EditorAudioBuses", true, false);
		for (int i = 0; i < nodes.size(); i++) {
			auto dock = Object::cast_to<EditorDock>(nodes[i]);
			if (dock) {
				dock->make_visible();
				return;
			}
		}
	}

	void FmodAudioBusesDock::_update_layout(int32_t value) {
		if (margin) {
			margin->set_theme_type_variation(value == DOCK_LAYOUT_FLOATING ? "NoBorderHorizontalBottom" : "NoBorderBottomPanel");
		}
		if (scroll) {
			scroll->set_scroll_hint_mode(value == DOCK_LAYOUT_FLOATING ? ScrollContainer::SCROLL_HINT_MODE_TOP_AND_LEFT : ScrollContainer::SCROLL_HINT_MODE_ALL);
		}
	}

	void FmodAudioBusesDock::_save_layout_to_config(const Ref<ConfigFile>& config, const String& section) const {
		config->set_value(section, "layout", edited_path);
		config->set_value(section, "scroll", scroll->get_h_scroll());
	}

	void FmodAudioBusesDock::_load_layout_from_config(const Ref<ConfigFile>& config, const String& section) {
		String path = config->get_value(section, "layout", String());
		if (!is_read_only() && !path.is_empty()) {
			open_layout(path);
		}
		scroll->set_h_scroll(config->get_value(section, "scroll", 0));
	}

	Variant FmodAudioBusesDock::_get_drag_data(const Vector2&) {
		return {};
	}

	bool FmodAudioBusesDock::_can_drop_data(const Vector2&, const Variant& data) const {
		if (is_read_only() || data.get_type() != Variant::DICTIONARY || layout.is_null()) {
			return false;
		}
		Dictionary d = data;
		return d.get("type", "") == Variant("fmod_audio_bus") && uint64_t(d.get("layout", 0)) == layout->get_instance_id() && int64_t(d.get("id", 0)) != 0;
	}

	void FmodAudioBusesDock::_drop_data(const Vector2&, const Variant& data) {
		if (!_can_drop_data({}, data)) {
			return;
		}
		Dictionary d = data;
		Ref<FmodAudioBusLayout> copy = layout->duplicate(false);
		copy->move_bus(d["id"], copy->get_bus_count() - 1);
		commit_edit("Move Audio Bus", copy);
	}

	void FmodAudioBusExportPlugin::_bind_methods() {
	}

	void FmodAudioBusExportPlugin::_include_resource(const String& reference, std::set<String>& visited) {
		const String path = resolve_path(reference);
		if (path.is_empty() || !visited.insert(path).second) {
			return;
		}
		if (!FileAccess::file_exists(path)) {
			ERR_PRINT("FMOD layout export dependency is missing: " + path);
			return;
		}
		add_file(path, FileAccess::get_file_as_bytes(path), false);
		for (const String& dependency : ResourceLoader::get_singleton()->get_dependencies(path)) {
			PackedStringArray parts = dependency.split("::");
			_include_resource(parts[parts.size() - 1], visited);
		}
	}

	void FmodAudioBusExportPlugin::_export_begin(const PackedStringArray&, bool, const String&, uint32_t) {
		if (bool(ProjectSettings::get_singleton()->get_setting_with_override("audio/fmod/sync_godot_audio_buses"))) {
			return;
		}
		const String path = resolve_path(default_path());
		if (path.is_empty() || !ResourceLoader::get_singleton()->exists(path)) {
			return;
		}
		std::set<String> visited;
		_include_resource(path, visited);
		// Extra files are not added to Godot's filtered UID cache. Resolve the
		// project default for the exported config, then restore its authoring UID.
		default_reference = ProjectSettings::get_singleton()->get_setting("audio/fmod/default_bus_layout");
		restore_default = true;
		ProjectSettings::get_singleton()->set_setting("audio/fmod/default_bus_layout", path);
	}

	void FmodAudioBusExportPlugin::_export_end() {
		if (restore_default) {
			ProjectSettings::get_singleton()->set_setting("audio/fmod/default_bus_layout", default_reference);
		}
		restore_default = false;
	}

	void FmodAudioBusesEditorPlugin::_bind_methods() {
	}

	void FmodAudioBusesEditorPlugin::_notification(int what) {
		if (what == NOTIFICATION_ENTER_TREE) {
			dock = memnew(FmodAudioBusesDock);
			add_dock(dock);
			exporter.instantiate();
			add_export_plugin(exporter);
		}
		else if (what == NOTIFICATION_EXIT_TREE) {
			if (dock) {
				remove_dock(dock);
				memdelete(dock);
				dock = nullptr;
			}
			if (exporter.is_valid()) {
				remove_export_plugin(exporter);
				exporter.unref();
			}
		}
	}

	bool FmodAudioBusesEditorPlugin::_handles(Object* object) const {
		return Object::cast_to<FmodAudioBusLayout>(object) != nullptr;
	}

	void FmodAudioBusesEditorPlugin::_edit(Object* object) {
		auto data = Object::cast_to<FmodAudioBusLayout>(object);
		if (dock && data && !data->get_path().is_empty()) {
			dock->open_layout(data->get_path());
		}
	}

	void FmodAudioBusesEditorPlugin::_make_visible(bool visible) {
		if (visible && dock) {
			dock->make_visible();
		}
	}
} //namespace godot
