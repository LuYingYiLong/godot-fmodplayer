#ifndef FMOD_AUDIO_BUSES_EDITOR_H
#define FMOD_AUDIO_BUSES_EDITOR_H

#include "mixer/fmod_audio_bus_layout.h"
#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/editor_dock.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>
#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/texture_progress_bar.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_slider.hpp>
#include <set>

namespace godot {
	String fmod_editor_translate(const String& text);
	float fmod_bus_db_to_position(float db);
	float fmod_bus_position_to_db(float value);
	class FmodAudioBusesDock;
	class FmodAudioBusEditor;

	class FmodAudioBusMeter : public HBoxContainer {
		GDCLASS(FmodAudioBusMeter, HBoxContainer)
		
		int64_t bus_id = 0;
		FMOD::DSP* metering = nullptr;
		bool previous_input = false, previous_output = false;
		struct Channel {
			TextureProgressBar* bar = nullptr;
			Panel* peak = nullptr;
			Timer* hold = nullptr;
			Ref<StyleBoxFlat> peak_style;
			float level_db = -100.0f;
			bool falling = false;
		};
		std::vector<Channel> channels;
		Ref<Gradient> active_gradient, inactive_gradient;
		Ref<GradientTexture2D> active_texture, inactive_texture;
		float pending_peaks[8] = {};
		bool pending_valid = false, pending_active = false;
		bool last_active = false;
		float last_peaks[8] = {};
		uint64_t last_valid_ticks = 0;
		void _release_meter();
		void _resize_channels(int count);
		void _collect_metering();
		void _update_colors();
		void _enable_indicator_fall();
		void _reset_levels();

	protected:
		static void _bind_methods();
		void _notification(int what);

	public:
		~FmodAudioBusMeter();
		void setup(int64_t id);
		void sample(float delta);
	};

	class FmodAudioBusMeterNotches : public Control {
		GDCLASS(FmodAudioBusMeterNotches, Control)

	protected:
		static void _bind_methods();
		void _notification(int what);

	public:
		Vector2 _get_minimum_size() const override;
		void _draw() override;
	};

	class FmodAudioBusEffectsTree : public Tree {
		GDCLASS(FmodAudioBusEffectsTree, Tree)

	protected:
		static void _bind_methods();

	public:
		FmodAudioBusEditor* row = nullptr;
		Variant _get_drag_data(const Vector2& pos) override;
		bool _can_drop_data(const Vector2& pos, const Variant& data) const override;
		void _drop_data(const Vector2& pos, const Variant& data) override;
	};

	class FmodAudioBusEditor : public PanelContainer {
		GDCLASS(FmodAudioBusEditor, PanelContainer)

		friend class FmodAudioBusEffectsTree;
		FmodAudioBusesDock* dock = nullptr;
		int64_t bus_id = 0;
		bool updating = false;
		bool changing_theme = false;
		bool checkbox_click = true;
		LineEdit* track = nullptr;
		VSlider* volume = nullptr;
		Button* solo = nullptr, * mute = nullptr, * bypass = nullptr;
		MenuButton* options = nullptr;
		OptionButton* send = nullptr;
		FmodAudioBusEffectsTree* effects = nullptr;
		PopupMenu* effect_menu = nullptr, * effect_options = nullptr;
		Label* volume_label = nullptr;
		Panel* volume_panel = nullptr;
		Timer* tooltip_timer = nullptr;
		Array drag_before;

		void _theme();
		void _renamed(const String& name);
		void _name_exited();
		void _volume_changed(double value);
		void _volume_input(const Ref<InputEvent>& input);
		void _drag_started();
		void _drag_ended(bool changed);
		void _toggle(int flag);
		void _send_selected(int index);
		void _effect_selected();
		void _effect_edited();
		void _effect_added(int index);
		void _effect_rmb(const Vector2& pos, int button);
		void _effect_option(int index);
		void _bus_option(int index);
		void _effects_input(const Ref<InputEvent>& input);
		int _drop_index(const Vector2& pos) const;

	protected:
		static void _bind_methods();
		void _notification(int what);

	public:
		FmodAudioBusMeter* meter = nullptr;
		void setup(FmodAudioBusesDock* owner, int64_t id);
		void refresh();
		Variant _get_drag_data(const Vector2& pos) override;
		bool _can_drop_data(const Vector2& pos, const Variant& data) const override;
		void _drop_data(const Vector2& pos, const Variant& data) override;
		void _gui_input(const Ref<InputEvent>& input) override;

	};

	class FmodAudioBusesDock : public EditorDock {
		GDCLASS(FmodAudioBusesDock, EditorDock)
			
		Ref<FmodAudioBusLayout> layout;
		String edited_path;
		bool unsaved = false, refresh_pending = false, loading = false, save_blocked = false;
		int file_action = 0;
		HBoxContainer* toolbar = nullptr, * rows = nullptr;
		ScrollContainer* scroll = nullptr;
		MarginContainer* margin = nullptr;
		Label* file_label = nullptr, * status = nullptr;
		MenuButton* extra_menu = nullptr;
		Timer* save_timer = nullptr, * meter_timer = nullptr;
		uint64_t last_meter_ticks = 0;
		bool meter_polling = false;
		EditorFileDialog* file_dialog = nullptr;
		AcceptDialog* error_dialog = nullptr;
		std::vector<Ref<FmodAudioEffect>> observed_effects;
		std::vector<Callable> observers;
		std::vector<Button*> editing_buttons;

		void _changed();
		void _effect_changed();
		void _rebuild();
		void _observe_effects();
		void _unobserve_effects();
		void _activate(const Ref<FmodAudioBusLayout>& resource, const String& path);
		void _file_chosen(const String& path);
		void _file_command(int command);
		void _save_timeout();
		void _meter_timeout();
		void _visibility_changed();
		void _server_changed();
		void _set_default();
		void _open_godot();
		void _show_error(const String& message);
		void _bind_preview_runtime(int64_t bus, int64_t slot);
		void _refresh_header();

	protected:
		static void _bind_methods();
		void _notification(int what);

	public:
		FmodAudioBusesDock();
		~FmodAudioBusesDock();

		Ref<FmodAudioBusLayout> get_layout() const { return layout; }
		bool is_read_only() const;
		void commit_state(const String& action, const Array& before, const Array& after);
		void commit_edit(const String& action, const Ref<FmodAudioBusLayout>& edited);
		void add_bus();
		void select_effect(int64_t bus, int64_t slot);
		void drop_effect(int64_t bus, int index, const Variant& data);
		bool can_drop_effect(const Variant& data) const;
		Error save_layout(const String& path = String());
		Error open_layout(const String& path);
		bool has_unsaved_changes() const { return unsaved; }
		String get_edited_path() const { return edited_path; }
		void _update_layout(int32_t layout) override;
		void _save_layout_to_config(const Ref<ConfigFile>& config, const String& section) const override;
		void _load_layout_from_config(const Ref<ConfigFile>& config, const String& section) override;
		Variant _get_drag_data(const Vector2& pos) override;
		bool _can_drop_data(const Vector2& pos, const Variant& data) const override;
		void _drop_data(const Vector2& pos, const Variant& data) override;
	};

	class FmodAudioBusExportPlugin : public EditorExportPlugin {
		GDCLASS(FmodAudioBusExportPlugin, EditorExportPlugin)
			
		void _include_resource(const String& path, std::set<String>& visited);
		Variant default_reference;
		bool restore_default = false;

	protected:
		static void _bind_methods();

	public:
		String _get_name() const override { return "FmodAudioBusLayout"; }
		void _export_begin(const PackedStringArray& features, bool debug, const String& path, uint32_t flags) override;
		void _export_end() override;

	};

	class FmodAudioBusesEditorPlugin : public EditorPlugin {
		GDCLASS(FmodAudioBusesEditorPlugin, EditorPlugin)
		
		FmodAudioBusesDock* dock = nullptr;
		Ref<FmodAudioBusExportPlugin> exporter;

	protected:
		static void _bind_methods();
		void _notification(int what);

	public:
		bool _handles(Object* object) const override;
		void _edit(Object* object) override;
		void _make_visible(bool visible) override;

	};
} //namespace godot

#endif // !FMOD_AUDIO_BUSES_EDITOR_H
