// UI structure, taper and meter behavior adapted from Godot's editor_audio_buses.cpp.
// Copyright (c) Godot Engine contributors. MIT license (see LICENSE).
#include "core/fmod_server.h"
#include "fmod_audio_buses_editor.h"
#include "mixer/fmod_audio_bus.h"
#include <cmath>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/style_box_empty.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/translation_domain.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	String fmod_editor_translate(const String& text) {
		return TranslationServer::get_singleton()->get_or_add_domain("godot.editor")->translate(text);
	}

	static float scale() {
		return EditorInterface::get_singleton()->get_editor_scale();
	}

	float fmod_bus_position_to_db(float v) {
		return v > 0.6f ? 22.22f * v - 16.2f : v < 0.05f ? 830.72f * v - 80.0f
			: 45.0f * std::pow(v - 1.0f, 3.0f);
	}

	float fmod_bus_db_to_position(float db) {
		return db > -2.88f ? (db + 16.2f) / 22.22f : db < -38.602f ? (db + 80.0f) / 830.72f
			: std::cbrt(db / 45.0f) + 1.0f;
	}

	static bool meter_notch_has_label(float db) {
		return db >= -6.0f || db == -24.0f || db == -72.0f;
	}

	static String meter_notch_text(float db) {
		return (db >= 0.0f ? "+" : "-") + String::num(std::abs(db), 0) + " dB";
	}

	void FmodAudioBusMeterNotches::_bind_methods() {
	}

	Vector2 FmodAudioBusMeterNotches::_get_minimum_size() const {
		const Ref<Font> font = get_theme_font("font", "Label");
		const int font_size = get_theme_font_size("font_size", "Label");
		float width = 0.0f;
		float height = 0.0f;
		for (float db = 6.0f; db >= -80.0f; db -= 6.0f) {
			if (meter_notch_has_label(db)) {
				width = MAX(width, font->get_string_size(meter_notch_text(db), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x);
				height += font->get_height(font_size);
			}
		}
		// Match EditorAudioMeterNotches: a five-pixel notch and two-pixel gap.
		return Vector2(width + 5.0f + 2.0f, height);
	}

	void FmodAudioBusMeterNotches::_notification(int what) {
		if (what == NOTIFICATION_THEME_CHANGED) {
			update_minimum_size();
			queue_redraw();
		}
	}

	void FmodAudioBusMeterNotches::_draw() {
		const float s = scale();
		const Ref<Font> font = get_theme_font("font", "Label");
		const int font_size = get_theme_font_size("font_size", "Label");
		const Color color = get_theme_color("font_color", "Editor");
		for (float db = 6.0f; db >= -80.0f; db -= 6.0f) {
			const float y = (1.0f - fmod_bus_db_to_position(db)) * get_size().y;
			draw_line(Vector2(0.0f, y), Vector2(5.0f * s, y), color, std::round(s));
			if (meter_notch_has_label(db)) {
				draw_string(font, Vector2(7.0f * s, y + font->get_height(font_size) / 4.0f), meter_notch_text(db), HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, color);
			}
		}
	}

	void FmodAudioBusEffectsTree::_bind_methods() {
	}

	Variant FmodAudioBusEffectsTree::_get_drag_data(const Vector2& pos) {
		TreeItem* item = get_item_at_position(pos);
		if (!row || !item || row->dock->is_read_only() || int64_t(item->get_metadata(0)) <= 0) {
			return {};
		}
		Dictionary d;
		d["type"] = "fmod_bus_effect";
		d["layout"] = row->dock->get_layout()->get_instance_id();
		d["bus"] = row->bus_id;
		d["slot"] = item->get_metadata(0);
		auto label = memnew(Label);
		label->set_text(item->get_text(0));
		set_drag_preview(label);
		return d;
	}

	bool FmodAudioBusEffectsTree::_can_drop_data(const Vector2&, const Variant& data) const {
		const bool allowed = row && row->dock->can_drop_effect(data);
		const_cast<FmodAudioBusEffectsTree*>(this)->set_drop_mode_flags(allowed ? DROP_MODE_INBETWEEN : DROP_MODE_DISABLED);
		return allowed;
	}

	void FmodAudioBusEffectsTree::_drop_data(const Vector2& pos, const Variant& data) {
		if (row) {
			row->dock->drop_effect(row->bus_id, row->_drop_index(pos), data);
		}
	}

	void FmodAudioBusEditor::_bind_methods() {
	}

	void FmodAudioBusEditor::setup(FmodAudioBusesDock* owner, int64_t id) {
		dock = owner;
		bus_id = id;
		set_translation_domain("godot.editor");
		set_tooltip_text("Drag & drop to rearrange.");
		set_focus_mode(FOCUS_CLICK);
		set_v_size_flags(SIZE_EXPAND_FILL);
		auto vb = memnew(VBoxContainer);
		vb->add_theme_constant_override("separation", int(4.0f * scale()));
		add_child(vb);
		track = memnew(LineEdit);
		track->set_name("BusName");
		track->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		vb->add_child(track);
		track->connect("text_submitted", callable_mp(this, &FmodAudioBusEditor::_renamed));
		track->connect("focus_exited", callable_mp(this, &FmodAudioBusEditor::_name_exited));
		auto flags = memnew(HBoxContainer);
		vb->add_child(flags);
		int flag = 0;
		for (auto destination : { &solo, &mute, &bypass }) {
			Button* b = memnew(Button);
			*destination = b;
			b->set_theme_type_variation("FlatButton");
			b->set_toggle_mode(true);
			b->set_focus_mode(FOCUS_ACCESSIBILITY);
			flags->add_child(b);
			b->set_tooltip_text(flag == 0 ? "Solo" : flag == 1 ? "Mute"
				: "Bypass");
			b->connect("pressed", callable_mp(this, &FmodAudioBusEditor::_toggle).bind(flag++));
			Ref<StyleBoxEmpty> empty;
			empty.instantiate();
			for (const char* state : { "normal", "hover", "hover_mirrored", "focus", "focus_mirrored" }) {
				b->add_theme_stylebox_override(state, empty);
			}
			Ref<StyleBoxFlat> pressed;
			pressed.instantiate();
			pressed->set_content_margin_all(0);
			pressed->set_bg_color(Color(1, 1, 1, 0));
			pressed->set_border_width(SIDE_BOTTOM, int(std::round(3.0f * scale())));
			for (const char* state : { "pressed", "pressed_mirrored", "hover_pressed", "hover_pressed_mirrored" }) {
				b->add_theme_stylebox_override(state, pressed);
			}
		}
		flags->add_spacer(false);
		options = memnew(MenuButton);
		options->set_tooltip_text("Bus Options");
		flags->add_child(options);
		auto menu = options->get_popup();
		menu->add_item("Duplicate Bus");
		menu->add_item("Delete Bus");
		menu->add_item("Reset Volume");
		menu->connect("index_pressed", callable_mp(this, &FmodAudioBusEditor::_bus_option));
		auto separator = memnew(HSeparator);
		separator->set_mouse_filter(MOUSE_FILTER_PASS);
		vb->add_child(separator);
		auto top_space = memnew(Control);
		top_space->set_custom_minimum_size(Vector2(0.0f, 6.0f * scale()));
		vb->add_child(top_space);
		auto meters = memnew(HBoxContainer);
		vb->add_child(meters);
		volume = memnew(VSlider);
		volume->set_name("Volume");
		volume->set_min(0.0);
		volume->set_max(1.0);
		volume->set_step(0.0001);
		volume->set_clip_contents(false);
		meters->add_child(volume);
		volume_panel = memnew(Panel);
		volume_panel->set_as_top_level(true);
		volume_panel->set_mouse_filter(MOUSE_FILTER_PASS);
		volume->add_child(volume_panel);
		volume_panel->hide();
		volume_label = memnew(Label);
		volume_label->set_mouse_filter(MOUSE_FILTER_PASS);
		volume_panel->add_child(volume_label);
		tooltip_timer = memnew(Timer);
		tooltip_timer->set_wait_time(0.8);
		tooltip_timer->set_one_shot(true);
		add_child(tooltip_timer);
		tooltip_timer->connect("timeout", Callable(volume_panel, "hide"));
		volume->connect("value_changed", callable_mp(this, &FmodAudioBusEditor::_volume_changed));
		volume->connect("drag_started", callable_mp(this, &FmodAudioBusEditor::_drag_started));
		volume->connect("drag_ended", callable_mp(this, &FmodAudioBusEditor::_drag_ended));
		volume->connect("gui_input", callable_mp(this, &FmodAudioBusEditor::_volume_input));
		meter = memnew(FmodAudioBusMeter);
		meter->set_name("Meter");
		meter->setup(bus_id);
		meters->add_child(meter);
		auto notches = memnew(FmodAudioBusMeterNotches);
		notches->set_name("MeterScale");
		notches->set_mouse_filter(MOUSE_FILTER_PASS);
		meters->add_child(notches);
		auto bottom_space = memnew(Control);
		bottom_space->set_custom_minimum_size(Vector2(0.0f, 2.0f * scale()));
		vb->add_child(bottom_space);
		effects = memnew(FmodAudioBusEffectsTree);
		effects->row = this;
		effects->set_name("Effects");
		effects->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		effects->set_hide_root(true);
		effects->set_hide_folding(true);
		effects->set_custom_minimum_size(Vector2(0.0f, 80.0f) * scale());
		effects->set_v_size_flags(SIZE_EXPAND_FILL);
		effects->set_theme_type_variation("EditorAudioBusEffectsTree");
		effects->set_allow_rmb_select(true);
		effects->set_allow_reselect(true);
		effects->set_focus_mode(FOCUS_CLICK);
		vb->add_child(effects);
		effects->connect("item_edited", callable_mp(this, &FmodAudioBusEditor::_effect_edited));
		effects->connect("cell_selected", callable_mp(this, &FmodAudioBusEditor::_effect_selected));
		effects->connect("focus_exited", Callable(effects, "deselect_all"));
		effects->connect("item_mouse_selected", callable_mp(this, &FmodAudioBusEditor::_effect_rmb));
		effects->connect("gui_input", callable_mp(this, &FmodAudioBusEditor::_effects_input));
		send = memnew(OptionButton);
		send->set_name("Send");
		send->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		send->set_clip_text(true);
		send->set_fit_to_longest_item(false);
		send->connect("item_selected", callable_mp(this, &FmodAudioBusEditor::_send_selected));
		vb->add_child(send);
		effect_menu = memnew(PopupMenu);
		effect_menu->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		add_child(effect_menu);
		effect_menu->connect("index_pressed", callable_mp(this, &FmodAudioBusEditor::_effect_added));
		PackedStringArray types = ClassDBSingleton::get_singleton()->get_inheriters_from_class("FmodAudioEffect");
		types.sort();
		for (const String& type : types) {
			if (ClassDBSingleton::get_singleton()->can_instantiate(type)) {
				effect_menu->add_item(type.replace("FmodAudioEffect", ""));
				effect_menu->set_item_metadata(-1, type);
			}
		}
		effect_options = memnew(PopupMenu);
		effect_options->add_item("Delete Effect");
		effect_options->add_item("Duplicate Effect");
		add_child(effect_options);
		effect_options->connect("index_pressed", callable_mp(this, &FmodAudioBusEditor::_effect_option));
		_theme();
		refresh();
	}

	void FmodAudioBusEditor::_theme() {
		if (!solo || !is_inside_tree() || changing_theme) {
			return;
		}
		changing_theme = true;
		add_theme_stylebox_override("panel", get_theme_stylebox(bus_id == 0 ? "master" : has_focus() ? "focus"
			: "normal",
			"EditorAudioBus"));
		auto settings = EditorInterface::get_singleton()->get_editor_settings();
		int mode = settings->get_setting("interface/theme/icon_and_font_color");
		Color base = settings->get_setting("interface/theme/base_color");
		bool dark = mode == 0 ? base.get_luminance() < 0.5f : mode == 2;
		Color colors[3] = { dark ? Color(1, 0.89, 0.22) : Color(1.9, 1.74, 0.83), dark ? Color(1, 0.16, 0.16) : Color(2.35, 1.03, 1.03), dark ? Color(0.13, 0.8, 1) : Color(1.03, 2.04, 2.35) };
		Button* buttons[3] = { solo, mute, bypass };
		const char* icons[3] = { "AudioBusSolo", "AudioBusMute", "AudioBusBypass" };
		for (int i = 0; i < 3; i++) {
			buttons[i]->set_button_icon(get_theme_icon(icons[i], "EditorIcons"));
			buttons[i]->add_theme_color_override("icon_pressed_color", colors[i]);
			buttons[i]->add_theme_color_override("icon_hover_pressed_color", colors[i].darkened(dark ? 0.15f : 0.65f));
			Ref<StyleBoxFlat> style = buttons[i]->get_theme_stylebox("pressed");
			style->set_border_color(colors[i].darkened(dark ? 0.15f : 0.65f));
		}
		options->set_button_icon(get_theme_icon("GuiTabMenuHl", "EditorIcons"));
		volume_panel->add_theme_stylebox_override("panel", get_theme_stylebox("panel", "TooltipPanel"));
		volume_label->add_theme_color_override("font_color", get_theme_color("font_color", "TooltipLabel"));
		changing_theme = false;
	}

	void FmodAudioBusEditor::_notification(int what) {
		if (what == NOTIFICATION_THEME_CHANGED || what == NOTIFICATION_READY || what == NOTIFICATION_FOCUS_ENTER || what == NOTIFICATION_FOCUS_EXIT) {
			_theme();
		}
	}

	void FmodAudioBusEditor::refresh() {
		if (!dock || !dock->get_layout()->find_bus(bus_id)) {
			return;
		}
		updating = true;
		const auto* b = dock->get_layout()->find_bus(bus_id);
		const bool ro = dock->is_read_only();
		if (!track->has_focus()) {
			track->set_text(b->name);
		}
		track->set_editable(bus_id != 0 && !ro);
		volume->set_editable(!ro);
		volume->set_value(fmod_bus_db_to_position(b->volume_db));
		solo->set_pressed_no_signal(b->solo);
		mute->set_pressed_no_signal(b->mute);
		bypass->set_pressed_no_signal(b->bypass);
		solo->set_disabled(ro);
		mute->set_disabled(ro);
		bypass->set_disabled(ro);
		options->get_popup()->set_item_disabled(0, ro);
		options->get_popup()->set_item_disabled(1, ro || bus_id == 0);
		options->get_popup()->set_item_disabled(2, ro);
		send->clear();
		if (bus_id == 0) {
			send->add_item(fmod_editor_translate("Speakers"));
		}
		else {
			for (const auto& target : dock->get_layout()->get_buses()) {
				if (dock->get_layout()->can_send_to(bus_id, target.id)) {
					send->add_item(target.name);
					send->set_item_metadata(-1, target.id);
					if (target.id == b->send_id) {
						send->select(send->get_item_count() - 1);
					}
				}
			}
		}
		send->set_disabled(ro || bus_id == 0);
		const auto ids = dock->get_layout()->get_bus_effect_ids(bus_id);
		bool rebuild = effects->get_root() == nullptr;
		TreeItem* root = effects->get_root();
		int n = 0;
		if (root) {
			for (TreeItem* child = root->get_first_child(); child; child = child->get_next()) {
				if (n < ids.size() && int64_t(child->get_metadata(0)) != ids[n]) {
					rebuild = true;
				}
				n++;
			}
		}
		if (n != ids.size() + 1) {
			rebuild = true;
		}
		if (rebuild) {
			effects->clear();
			root = effects->create_item();
			for (const auto& s : b->effects) {
				TreeItem* item = effects->create_item(root);
				item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
				item->set_metadata(0, s.id);
			}
			TreeItem* add = effects->create_item(root);
			add->set_metadata(0, int64_t(0));
			add->set_selectable(0, false);
			add->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);
		}
		TreeItem* item = root->get_first_child();
		for (const auto& s : b->effects) {
			item->set_editable(0, !ro);
			item->set_checked(0, s.enabled);
			String text = s.effect->get_name();
			item->set_text(0, text.is_empty() ? s.effect->get_class().replace("FmodAudioEffect", "") : text);
			item = item->get_next();
		}
		if (item) {
			item->set_text(0, fmod_editor_translate("Add Effect"));
			item->set_editable(0, !ro);
		}
		updating = false;
	}

	void FmodAudioBusEditor::_renamed(const String& name) {
		if (updating || dock->is_read_only() || name == String(dock->get_layout()->get_bus_name(bus_id))) {
			return;
		}
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		if (copy->rename_bus(bus_id, name) == OK) {
			dock->commit_edit("Rename Audio Bus", copy);
		}
		else {
			track->set_text(dock->get_layout()->get_bus_name(bus_id));
		}
	}

	void FmodAudioBusEditor::_name_exited() {
		_renamed(track->get_text());
	}

	void FmodAudioBusEditor::_volume_input(const Ref<InputEvent>& input) {
		Ref<InputEventMouseButton> mouse = input;
		if (mouse.is_valid() && mouse->get_button_index() == MOUSE_BUTTON_LEFT && mouse->is_pressed() && !dock->is_read_only()) {
			drag_before = dock->get_layout()->get_state();
		}
	}

	void FmodAudioBusEditor::_drag_started() {
		if (drag_before.is_empty()) {
			drag_before = dock->get_layout()->get_state();
		}
	}

	void FmodAudioBusEditor::_drag_ended(bool) {
		if (!drag_before.is_empty()) {
			Array after = dock->get_layout()->get_state();
			dock->commit_state("Change Audio Bus Volume", drag_before, after);
			drag_before = Array();
		}
	}

	void FmodAudioBusEditor::_volume_changed(double value) {
		if (updating || dock->is_read_only()) {
			return;
		}
		float db = fmod_bus_position_to_db(float(value));
		if (Input::get_singleton()->is_key_pressed(KEY_CTRL) || Input::get_singleton()->is_key_pressed(KEY_META)) {
			db = std::round(db);
		}
		String label = (db > 0.05f ? "+" : "") + String::num(std::abs(db) < 0.05f ? 0.0f : db, 1) + " dB";
		volume->set_tooltip_text(label);
		volume_label->set_text(label);
		volume_panel->set_size(volume_label->get_minimum_size());
		volume_panel->set_position(volume->get_global_position() + Vector2(-volume_label->get_minimum_size().x, (volume->get_size().y - 10.0f) * (1.0f - float(value)) - 10.0f));
		volume_panel->show();
		tooltip_timer->start();
		if (!drag_before.is_empty()) {
			dock->get_layout()->set_bus_volume_db(bus_id, db);
		}
		else {
			Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
			copy->set_bus_volume_db(bus_id, db);
			dock->commit_edit("Change Audio Bus Volume", copy);
		}
	}

	void FmodAudioBusEditor::_toggle(int flag) {
		if (updating || dock->is_read_only()) {
			return;
		}
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		if (flag == 0) {
			copy->set_bus_solo(bus_id, solo->is_pressed());
		}
		else if (flag == 1) {
			copy->set_bus_mute(bus_id, mute->is_pressed());
		}
		else {
			copy->set_bus_bypass(bus_id, bypass->is_pressed());
		}
		dock->commit_edit(flag == 0 ? "Toggle Audio Bus Solo" : flag == 1 ? "Toggle Audio Bus Mute"
			: "Toggle Audio Bus Bypass Effects",
			copy);
	}

	void FmodAudioBusEditor::_send_selected(int index) {
		if (updating || dock->is_read_only()) {
			return;
		}
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		if (copy->set_bus_send(bus_id, send->get_item_metadata(index)) == OK) {
			dock->commit_edit("Select Audio Bus Send", copy);
		}
	}

	void FmodAudioBusEditor::_effect_selected() {
		TreeItem* item = effects->get_selected();
		if (item && int64_t(item->get_metadata(0))) {
			dock->select_effect(bus_id, item->get_metadata(0));
		}
	}

	void FmodAudioBusEditor::_effect_edited() {
		if (updating || dock->is_read_only()) {
			return;
		}
		TreeItem* item = effects->get_edited();
		if (!item) {
			return;
		}
		const int64_t slot = item->get_metadata(0);
		if (!slot) {
			effect_menu->set_position(Vector2i(effects->get_screen_position() + effects->get_local_mouse_position()));
			effect_menu->popup();
			return;
		}
		if (!checkbox_click) {
			item->set_checked(0, dock->get_layout()->is_bus_effect_enabled(bus_id, slot));
			return;
		}
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		copy->set_bus_effect_enabled(bus_id, slot, item->is_checked(0));
		dock->commit_edit("Toggle Audio Bus Effect", copy);
	}

	void FmodAudioBusEditor::_effect_added(int index) {
		if (dock->is_read_only()) {
			return;
		}
		Ref<FmodAudioEffect> effect = ClassDBSingleton::get_singleton()->instantiate(effect_menu->get_item_metadata(index));
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		copy->add_bus_effect(bus_id, effect);
		dock->commit_edit("Add Audio Bus Effect", copy);
	}

	void FmodAudioBusEditor::_effect_rmb(const Vector2& pos, int button) {
		if (button != MOUSE_BUTTON_RIGHT || dock->is_read_only() || !effects->get_selected() || !int64_t(effects->get_selected()->get_metadata(0))) {
			return;
		}
		effect_options->set_position(Vector2i(effects->get_screen_position() + pos));
		effect_options->popup();
	}

	void FmodAudioBusEditor::_effect_option(int index) {
		if (dock->is_read_only() || !effects->get_selected()) {
			return;
		}
		const int64_t slot = effects->get_selected()->get_metadata(0);
		if (!slot) {
			return;
		}
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		if (index == 0) {
			copy->remove_bus_effect(bus_id, slot);
		}
		else {
			Ref<FmodAudioEffect> effect = copy->get_bus_effect(bus_id, slot)->duplicate(true);
			copy->add_bus_effect(bus_id, effect);
		}
		dock->commit_edit(index == 0 ? "Delete Bus Effect" : "Duplicate Bus Effect", copy);
	}

	void FmodAudioBusEditor::_bus_option(int index) {
		if (dock->is_read_only()) {
			return;
		}
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		if (index == 0) {
			String base = String(copy->get_bus_name(bus_id)) + " Copy", name = base;
			int n = 2;
			while (copy->find_bus_id(name) >= 0) {
				name = base + " " + String::num_int64(n++);
			}
			int64_t id = copy->add_bus(name, bus_id == 0 ? 0 : copy->get_bus_send(bus_id));
			const auto* b = dock->get_layout()->find_bus(bus_id);
			copy->set_bus_volume_db(id, b->volume_db);
			copy->set_bus_mute(id, b->mute);
			copy->set_bus_solo(id, b->solo);
			copy->set_bus_bypass(id, b->bypass);
			for (const auto& s : b->effects) {
				Ref<FmodAudioEffect> effect = s.effect->duplicate(true);
				int64_t slot = copy->add_bus_effect(id, effect);
				copy->set_bus_effect_enabled(id, slot, s.enabled);
			}
		}
		else if (index == 1) {
			if (copy->remove_bus(bus_id) != OK) {
				return;
			}
		}
		else {
			copy->set_bus_volume_db(bus_id, 0.0f);
		}
		dock->commit_edit(index == 0 ? "Duplicate Audio Bus" : index == 1 ? "Delete Audio Bus"
			: "Reset Bus Volume",
			copy);
	}

	void FmodAudioBusEditor::_effects_input(const Ref<InputEvent>& input) {
		Ref<InputEventMouseButton> mouse = input;
		if (mouse.is_valid() && mouse->is_pressed() && mouse->get_button_index() == MOUSE_BUTTON_LEFT) {
			// The engine's checkbox-only option is private. Preserve that behavior
			// through the public input signal instead of toggling on label clicks.
			checkbox_click = mouse->get_position().x < float(effects->get_theme_icon("checked", "Tree")->get_width()) + 8.0f * scale();
		}
		Ref<InputEventKey> key = input;
		if (key.is_valid() && key->is_pressed() && !key->is_echo()) {
			checkbox_click = true;
			if (key->get_keycode() == KEY_DELETE) {
				_effect_option(0);
				effects->accept_event();
			}
			else if (key->get_keycode() == KEY_D && key->is_command_or_control_pressed()) {
				_effect_option(1);
				effects->accept_event();
			}
		}
	}

	int FmodAudioBusEditor::_drop_index(const Vector2& pos) const {
		TreeItem* item = effects->get_item_at_position(pos);
		if (!item) {
			return int(dock->get_layout()->get_bus_effect_ids(bus_id).size());
		}
		int index = item->get_index();
		return index + (effects->get_drop_section_at_position(pos) > 0 && int64_t(item->get_metadata(0)) ? 1 : 0);
	}

	Variant FmodAudioBusEditor::_get_drag_data(const Vector2&) {
		if (!bus_id || dock->is_read_only()) {
			return {};
		}
		Dictionary d;
		d["type"] = "fmod_audio_bus";
		d["layout"] = dock->get_layout()->get_instance_id();
		d["id"] = bus_id;
		auto label = memnew(Label);
		label->set_text(track->get_text());
		set_drag_preview(label);
		return d;
	}

	bool FmodAudioBusEditor::_can_drop_data(const Vector2&, const Variant& data) const {
		if (!bus_id || dock->is_read_only() || data.get_type() != Variant::DICTIONARY) {
			return false;
		}
		Dictionary d = data;
		return d.get("type", "") == Variant("fmod_audio_bus") && uint64_t(d.get("layout", 0)) == dock->get_layout()->get_instance_id() && dock->get_layout()->find_bus(int64_t(d.get("id", -1)));
	}

	void FmodAudioBusEditor::_drop_data(const Vector2&, const Variant& data) {
		Dictionary d = data;
		Ref<FmodAudioBusLayout> copy = dock->get_layout()->duplicate(false);
		PackedInt64Array ids = copy->get_bus_ids();
		for (int i = 1; i < ids.size(); i++) {
			if (ids[i] == bus_id) {
				copy->move_bus(d["id"], i);
				break;
			}
		}
		dock->commit_edit("Move Audio Bus", copy);
	}

	void FmodAudioBusEditor::_gui_input(const Ref<InputEvent>& input) {
		Ref<InputEventKey> key = input;
		if (key.is_valid() && key->is_pressed() && !key->is_echo()) {
			if (key->get_keycode() == KEY_DELETE) {
				_bus_option(1);
				accept_event();
			}
			else if (key->get_keycode() == KEY_D && key->is_command_or_control_pressed()) {
				_bus_option(0);
				accept_event();
			}
		}
		Ref<InputEventMouseButton> mouse = input;
		if (mouse.is_valid() && mouse->is_pressed() && mouse->get_button_index() == MOUSE_BUTTON_RIGHT) {
			options->get_popup()->set_position(Vector2i(get_screen_position() + mouse->get_position()));
			options->get_popup()->popup();
			accept_event();
		}
	}
} //namespace godot
