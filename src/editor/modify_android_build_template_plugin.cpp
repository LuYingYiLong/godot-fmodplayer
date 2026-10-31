#include "editor/fmod_editor_translation.h"
#include "modify_android_build_template_plugin.h"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/file_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot {
	static constexpr const char* TOOL_MENU_NAME = FMOD_EDITOR_TEXT("Configure FMOD Android Export");
	static constexpr const char* FMOD_JAR_GRADLE_LINE = "    implementation files(\"libs/fmod.jar\")";
	static constexpr const char* FMOD_JAVA_MARKER = "// FMOD Android runtime.";

	void ModifyAndroidBuildTemplatePlugin::_bind_methods() {
	}

	void ModifyAndroidBuildTemplatePlugin::_notification(int p_what) {
		switch (p_what) {
            case NOTIFICATION_TRANSLATION_CHANGED: {
                if (sdk_select_dialog) {
                    bool visible = sdk_select_dialog->is_visible();
                    if (file_path) selected_fmod_path = file_path->get_text();
                    _free_sdk_select_dialog();
                    _create_sdk_select_dialog();
                    if (visible) sdk_select_dialog->popup_centered();
                }
                if (review_dialog) {
                    bool visible = review_dialog->is_visible();
                    _create_review_dialog(_build_review_summary(pending_sdk));
                    if (visible) review_dialog->popup_centered();
                }
            } break;
			case NOTIFICATION_ENTER_TREE: {
				add_tool_menu_item(TOOL_MENU_NAME, callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_open_configure_dialog));
			} break;

			case NOTIFICATION_EXIT_TREE: {
				remove_tool_menu_item(TOOL_MENU_NAME);
				_free_sdk_select_dialog();
				_free_review_dialog();
			} break;
		}
	}

	void ModifyAndroidBuildTemplatePlugin::_open_configure_dialog() {
		String error;
		if (!_validate_android_template(error)) {
			_create_dialog_to_editor(fmod_editor_translate("Android Build Template Missing"),
				error + "\n\n" + fmod_editor_translate("Install it from: Project > Install Android Build Template...")
			);
			return;
		}

		_create_sdk_select_dialog();
		sdk_select_dialog->popup_centered(Vector2(820, 220));
	}

	void ModifyAndroidBuildTemplatePlugin::_create_sdk_select_dialog() {
		_free_sdk_select_dialog();

		sdk_select_dialog = memnew(ConfirmationDialog);
		sdk_select_dialog->set_title(fmod_editor_translate("Configure FMOD Android Export"));
		sdk_select_dialog->set_min_size(Vector2(820, 220));
		sdk_select_dialog->get_ok_button()->set_text(fmod_editor_translate("Review Changes"));

		VBoxContainer* main_vbox = memnew(VBoxContainer);
		main_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		sdk_select_dialog->add_child(main_vbox);

		Label* info_label = memnew(Label);
		info_label->set_text(fmod_editor_translate("Select the extracted FMOD Core Android SDK folder, or the downloaded .tar.gz/.tgz archive.\n"
			"The tool will configure the installed Godot Android build template for FMOD Core."
		));
		main_vbox->add_child(info_label);

		HBoxContainer* path_row = memnew(HBoxContainer);
		path_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		main_vbox->add_child(path_row);

		file_path = memnew(LineEdit);
		file_path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		file_path->set_placeholder(fmod_editor_translate("Path to FMOD Core Android SDK folder or archive..."));
		if (!selected_fmod_path.is_empty()) {
			file_path->set_text(selected_fmod_path);
		}
		path_row->add_child(file_path);

		Button* browse_button = memnew(Button);
		browse_button->set_text(fmod_editor_translate("Browse..."));
		browse_button->connect("pressed", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_browse_pressed));
		path_row->add_child(browse_button);

		file_dialog = memnew(FileDialog);
		file_dialog->set_access(FileDialog::ACCESS_FILESYSTEM);
		file_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_ANY);
		file_dialog->set_title(fmod_editor_translate("Select FMOD Core Android SDK"));
		PackedStringArray filters;
		filters.append(String("*.tar.gz,*.tgz ; ") + fmod_editor_translate("FMOD Android archive"));
		filters.append(String("* ; ") + fmod_editor_translate("All files"));
		file_dialog->set_filters(filters);
		file_dialog->connect("file_selected", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_file_dialog_path_selected));
		file_dialog->connect("dir_selected", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_file_dialog_path_selected));
		sdk_select_dialog->add_child(file_dialog);

		sdk_select_dialog->connect("confirmed", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_sdk_select_confirmed));
		sdk_select_dialog->get_cancel_button()->connect("pressed", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_sdk_select_canceled));

		EditorInterface::get_singleton()->get_base_control()->add_child(sdk_select_dialog);
	}

	void ModifyAndroidBuildTemplatePlugin::_create_review_dialog(const String& p_summary) {
		_free_review_dialog();

		review_dialog = memnew(ConfirmationDialog);
		review_dialog->set_title(fmod_editor_translate("Review FMOD Android Export Changes"));
		review_dialog->set_min_size(Vector2(860, 560));
		review_dialog->get_ok_button()->set_text(fmod_editor_translate("Apply Changes"));

		review_text = memnew(RichTextLabel);
		review_text->set_use_bbcode(false);
		review_text->set_scroll_active(true);
		review_text->set_selection_enabled(true);
		review_text->set_custom_minimum_size(Vector2(820, 440));
		review_text->set_text(p_summary);
		review_dialog->add_child(review_text);

		review_dialog->connect("confirmed", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_review_confirmed));
		review_dialog->get_cancel_button()->connect("pressed", callable_mp(this, &ModifyAndroidBuildTemplatePlugin::_on_review_canceled));

		EditorInterface::get_singleton()->get_base_control()->add_child(review_dialog);
	}

	void ModifyAndroidBuildTemplatePlugin::_create_dialog_to_editor(const String& p_title, const String& p_text) {
		AcceptDialog* dialog = memnew(AcceptDialog);
		dialog->set_translation_domain("godot.editor");
		dialog->set_title(p_title);
		dialog->set_text(p_text);
		EditorInterface::get_singleton()->get_base_control()->add_child(dialog);
		dialog->popup_centered();
	}

	void ModifyAndroidBuildTemplatePlugin::_free_sdk_select_dialog() {
		if (sdk_select_dialog) {
			sdk_select_dialog->queue_free();
			sdk_select_dialog = nullptr;
			file_path = nullptr;
			file_dialog = nullptr;
		}
	}

	void ModifyAndroidBuildTemplatePlugin::_free_review_dialog() {
		if (review_dialog) {
			review_dialog->queue_free();
			review_dialog = nullptr;
			review_text = nullptr;
		}
	}

	void ModifyAndroidBuildTemplatePlugin::_on_browse_pressed() {
		ERR_FAIL_NULL(file_dialog);
		file_dialog->popup_centered(Vector2(900, 620));
	}

	void ModifyAndroidBuildTemplatePlugin::_on_file_dialog_path_selected(const String& p_path) {
		selected_fmod_path = p_path;
		if (file_path) {
			file_path->set_text(p_path);
		}
	}

	void ModifyAndroidBuildTemplatePlugin::_on_sdk_select_confirmed() {
		ERR_FAIL_NULL(file_path);

		selected_fmod_path = file_path->get_text().strip_edges();
		if (selected_fmod_path.is_empty()) {
			_create_dialog_to_editor(fmod_editor_translate("Path Required"), fmod_editor_translate("Please select the FMOD Core Android SDK folder or archive."));
			return;
		}

		String error;
		FmodAndroidSdkInfo info;
		if (!_resolve_fmod_sdk(selected_fmod_path, info, error)) {
			_create_dialog_to_editor(fmod_editor_translate("Invalid FMOD Android SDK"), error);
			return;
		}

		pending_sdk = info;
		_free_sdk_select_dialog();
		_create_review_dialog(_build_review_summary(pending_sdk));
		review_dialog->popup_centered(Vector2(860, 560));
	}

	void ModifyAndroidBuildTemplatePlugin::_on_sdk_select_canceled() {
		_free_sdk_select_dialog();
	}

	void ModifyAndroidBuildTemplatePlugin::_on_review_confirmed() {
		String report;
		Error err = _apply_android_template_changes(pending_sdk, report);
		_free_review_dialog();

		if (err == OK) {
			_create_dialog_to_editor(fmod_editor_translate("FMOD Android Export Configured"), fmod_editor_translate("FMOD Android export setup completed successfully.") + "\n\n" + report);
		}
		else {
			_create_dialog_to_editor(fmod_editor_translate("FMOD Android Export Setup Failed"), fmod_editor_translate("The Android build template was not fully configured.") + "\n\n" + report);
		}
	}

	void ModifyAndroidBuildTemplatePlugin::_on_review_canceled() {
		_free_review_dialog();
	}

	bool ModifyAndroidBuildTemplatePlugin::_validate_android_template(String& r_error) const {
		String global_template_path = ProjectSettings::get_singleton()->globalize_path(template_path);
		if (!DirAccess::dir_exists_absolute(global_template_path)) {
			r_error = fmod_editor_translate("The project does not have an installed Android build template.");
			return false;
		}

		PackedStringArray required_files;
		required_files.append(template_path.path_join("build.gradle"));
		required_files.append(template_path.path_join("gradle.properties"));
		required_files.append(template_path.path_join("src/main/java/com/godot/game/GodotApp.java"));

		for (int i = 0; i < required_files.size(); i++) {
			if (!FileAccess::file_exists(required_files[i])) {
				r_error = fmod_editor_translate("Missing required Android build template file:\n") + required_files[i];
				return false;
			}
		}

		return true;
	}

	bool ModifyAndroidBuildTemplatePlugin::_resolve_fmod_sdk(const String& p_path, FmodAndroidSdkInfo& r_info, String& r_error) {
		String input_path = ProjectSettings::get_singleton()->globalize_path(p_path).replace("\\", "/");
		r_info = FmodAndroidSdkInfo();
		r_info.input_path = input_path;

		String search_root = input_path;
		String lower = input_path.to_lower();
		if (FileAccess::file_exists(input_path) && (lower.ends_with(".tar.gz") || lower.ends_with(".tgz"))) {
			if (!_extract_archive(input_path, r_info.extracted_root, r_error)) {
				return false;
			}
			search_root = r_info.extracted_root;
		}
		else if (!DirAccess::dir_exists_absolute(input_path)) {
			r_error = fmod_editor_translate("The selected path is not a directory or a supported .tar.gz/.tgz archive:\n") + input_path;
			return false;
		}

		r_info.core_lib_path = _find_core_lib_path(search_root);
		if (r_info.core_lib_path.is_empty()) {
			r_error = fmod_editor_translate("Cannot find api/core/lib/fmod.jar under:\n") + search_root + "\n\n" + fmod_editor_translate("Select the FMOD Core Android SDK root folder or archive.");
			return false;
		}

		if (!_has_any_fmod_native_library(r_info.core_lib_path)) {
			r_error = fmod_editor_translate("Cannot find FMOD native libraries under:\n") + r_info.core_lib_path + "\n\n" + fmod_editor_translate("Expected ABI folders: arm64-v8a, armeabi-v7a, x86, x86_64.");
			return false;
		}

		return true;
	}

	bool ModifyAndroidBuildTemplatePlugin::_extract_archive(const String& p_archive_path, String& r_extract_root, String& r_error) const {
		String temp_root = OS::get_singleton()->get_temp_dir().path_join("godot_fmod_android_" + String::num_uint64(Time::get_singleton()->get_ticks_msec())).replace("\\", "/");
		Error err = DirAccess::make_dir_recursive_absolute(temp_root);
		if (err != OK) {
			r_error = fmod_editor_translate("Failed to create temporary extraction directory:\n") + temp_root;
			return false;
		}

		PackedStringArray args;
		args.append(p_archive_path.to_lower().ends_with(".tgz") ? "-xzf" : "-xzf");
		args.append(p_archive_path);
		args.append("-C");
		args.append(temp_root);

		Array output;
		int exit_code = OS::get_singleton()->execute("tar", args, output, true, false);
		if (exit_code != 0) {
			String output_text;
			if (!output.is_empty()) {
				output_text = String(output[0]);
			}
			r_error = fmod_editor_translate("Failed to extract the FMOD archive. Extract it manually and select the extracted folder.")
                + "\n" + p_archive_path + "\n\n" + output_text;
			return false;
		}

		r_extract_root = temp_root;
		return true;
	}

	String ModifyAndroidBuildTemplatePlugin::_find_core_lib_path(const String& p_root, int p_depth) const {
		if (p_depth > 5) {
			return String();
		}

		String normalized_root = p_root.replace("\\", "/");
		String direct = normalized_root.path_join("api/core/lib");
		if (FileAccess::file_exists(direct.path_join("fmod.jar"))) {
			return direct;
		}

		Ref<DirAccess> dir = DirAccess::open(normalized_root);
		if (dir.is_null()) {
			return String();
		}

		dir->list_dir_begin();
		String entry = dir->get_next();
		while (!entry.is_empty()) {
			if (dir->current_is_dir() && !entry.begins_with(".")) {
				String found = _find_core_lib_path(normalized_root.path_join(entry), p_depth + 1);
				if (!found.is_empty()) {
					return found;
				}
			}
			entry = dir->get_next();
		}

		return String();
	}

	bool ModifyAndroidBuildTemplatePlugin::_has_any_fmod_native_library(const String& p_core_lib_path) const {
		PackedStringArray abis;
		abis.append("arm64-v8a");
		abis.append("armeabi-v7a");
		abis.append("x86");
		abis.append("x86_64");

		for (int i = 0; i < abis.size(); i++) {
			if (FileAccess::file_exists(p_core_lib_path.path_join(abis[i]).path_join("libfmod.so")) ||
					FileAccess::file_exists(p_core_lib_path.path_join(abis[i]).path_join("libfmodL.so"))) {
				return true;
			}
		}
		return false;
	}

	String ModifyAndroidBuildTemplatePlugin::_build_review_summary(const FmodAndroidSdkInfo& p_info) const {
		String summary;
		summary += fmod_editor_translate("FMOD Android export setup will apply the following changes.\n\n");
		summary += fmod_editor_translate("Android build template:\n  ") + template_path + "\n\n";
		summary += fmod_editor_translate("FMOD Core Android SDK source:\n  ") + p_info.input_path + "\n";
		if (!p_info.extracted_root.is_empty()) {
			summary += fmod_editor_translate("Temporary extraction folder:\n  ") + p_info.extracted_root + "\n";
		}
		summary += fmod_editor_translate("Resolved FMOD Core library folder:\n  ") + p_info.core_lib_path + "\n\n";

		summary += fmod_editor_translate("Files to modify:\n");
		summary += "  - res://android/build/gradle.properties\n";
		summary += fmod_editor_translate("      Ensure android.overridePathCheck=true.\n");
		summary += "  - res://android/build/build.gradle\n";
		summary += fmod_editor_translate("      Ensure implementation files(\"libs/fmod.jar\") is present.\n");
		summary += "  - res://android/build/src/main/java/com/godot/game/GodotApp.java\n";
		summary += fmod_editor_translate("      Ensure FMOD is loaded before Godot starts.\n");
		summary += fmod_editor_translate("      Debug/dev builds load fmodL; release builds load fmod.\n\n");

		summary += fmod_editor_translate("Files to copy:\n");
		summary += "  - fmod.jar -> res://android/build/libs/fmod.jar\n";
		summary += "  - libfmodL.so -> res://android/build/libs/debug/<ABI>/libfmodL.so\n";
		summary += "  - libfmodL.so -> res://android/build/libs/dev/<ABI>/libfmodL.so\n";
		summary += "  - libfmod.so -> res://android/build/libs/release/<ABI>/libfmod.so\n";
		summary += "  - libfmod_player.android.template_debug.arm64.so -> res://android/build/libs/debug/arm64-v8a/\n";
		summary += "  - libfmod_player.android.template_debug.arm64.so -> res://android/build/libs/dev/arm64-v8a/\n";
		summary += "  - libfmod_player.android.template_release.arm64.so -> res://android/build/libs/release/arm64-v8a/\n\n";

		summary += fmod_editor_translate("This operation is idempotent: existing matching lines will not be duplicated, and copied files will be overwritten.");
		return summary;
	}

	Error ModifyAndroidBuildTemplatePlugin::_apply_android_template_changes(const FmodAndroidSdkInfo& p_info, String& r_report) {
		Error err = OK;
		Error step_err = _copy_fmod_libraries(p_info.core_lib_path, r_report);
		if (step_err != OK) {
			err = step_err;
		}
		step_err = _copy_plugin_android_libraries(r_report);
		if (step_err != OK) {
			err = step_err;
		}
		step_err = _patch_gradle_properties(r_report);
		if (step_err != OK) {
			err = step_err;
		}
		step_err = _patch_build_gradle(r_report);
		if (step_err != OK) {
			err = step_err;
		}
		step_err = _patch_godot_app_java(r_report);
		if (step_err != OK) {
			err = step_err;
		}
		return err;
	}

	Error ModifyAndroidBuildTemplatePlugin::_copy_fmod_libraries(const String& p_core_lib_path, String& r_report) {
		Error result = OK;

		Error err = _ensure_dir_exists(template_path.path_join("libs"));
		if (err != OK) {
			r_report += fmod_editor_translate("- Failed to create res://android/build/libs.\n");
			return err;
		}

		err = _copy_file(p_core_lib_path.path_join("fmod.jar"), template_path.path_join("libs/fmod.jar"));
		if (err == OK) {
			r_report += fmod_editor_translate("- Copied fmod.jar.\n");
		}
		else {
			r_report += fmod_editor_translate("- Failed to copy fmod.jar.\n");
			result = err;
		}

		PackedStringArray abis;
		abis.append("arm64-v8a");
		abis.append("armeabi-v7a");
		abis.append("x86");
		abis.append("x86_64");

		for (int i = 0; i < abis.size(); i++) {
			String abi = abis[i];
			String source_debug = p_core_lib_path.path_join(abi).path_join("libfmodL.so");
			String source_release = p_core_lib_path.path_join(abi).path_join("libfmod.so");

			if (FileAccess::file_exists(source_debug)) {
				_copy_file(source_debug, template_path.path_join("libs/debug").path_join(abi).path_join("libfmodL.so"));
				_copy_file(source_debug, template_path.path_join("libs/dev").path_join(abi).path_join("libfmodL.so"));
				r_report += fmod_editor_translate("- Copied debug/dev FMOD runtime for ") + abi + ".\n";
			}
			if (FileAccess::file_exists(source_release)) {
				_copy_file(source_release, template_path.path_join("libs/release").path_join(abi).path_join("libfmod.so"));
				r_report += fmod_editor_translate("- Copied release FMOD runtime for ") + abi + ".\n";
			}
		}

		return result;
	}

	Error ModifyAndroidBuildTemplatePlugin::_copy_plugin_android_libraries(String& r_report) {
		Error result = OK;
		String debug_src = "res://addons/fmod_player/bin/libfmod_player.android.template_debug.arm64.so";
		String release_src = "res://addons/fmod_player/bin/libfmod_player.android.template_release.arm64.so";

		if (FileAccess::file_exists(debug_src)) {
			Error err = _copy_file(debug_src, template_path.path_join("libs/debug/arm64-v8a/libfmod_player.android.template_debug.arm64.so"));
			if (err == OK) {
				_copy_file(debug_src, template_path.path_join("libs/dev/arm64-v8a/libfmod_player.android.template_debug.arm64.so"));
				r_report += fmod_editor_translate("- Copied FMOD GDExtension debug/dev Android library for arm64-v8a.\n");
			}
			else {
				r_report += fmod_editor_translate("- Failed to copy FMOD GDExtension debug Android library.\n");
				result = err;
			}
		}
		else {
			r_report += fmod_editor_translate("- Skipped FMOD GDExtension debug Android library; source file was not found.\n");
		}

		if (FileAccess::file_exists(release_src)) {
			Error err = _copy_file(release_src, template_path.path_join("libs/release/arm64-v8a/libfmod_player.android.template_release.arm64.so"));
			if (err == OK) {
				r_report += fmod_editor_translate("- Copied FMOD GDExtension release Android library for arm64-v8a.\n");
			}
			else {
				r_report += fmod_editor_translate("- Failed to copy FMOD GDExtension release Android library.\n");
				result = err;
			}
		}
		else {
			r_report += fmod_editor_translate("- Skipped FMOD GDExtension release Android library; source file was not found.\n");
		}

		return result;
	}

	Error ModifyAndroidBuildTemplatePlugin::_patch_gradle_properties(String& r_report) {
		String path = template_path.path_join("gradle.properties");
		String text = _read_text_file(path);
		if (text.is_empty()) {
			r_report += fmod_editor_translate("- Failed to read gradle.properties.\n");
			return FAILED;
		}

		if (text.contains("android.overridePathCheck=true")) {
			r_report += fmod_editor_translate("- gradle.properties already enables android.overridePathCheck.\n");
			return OK;
		}

		if (text.contains("android.overridePathCheck=false")) {
			text = text.replace("android.overridePathCheck=false", "android.overridePathCheck=true");
		}
		else {
			if (!text.ends_with("\n")) {
				text += "\n";
			}
			text += "\n# Allows Gradle builds when the project path contains non-ASCII characters.\n";
			text += "android.overridePathCheck=true\n";
		}

		if (!_write_text_file(path, text)) {
			r_report += fmod_editor_translate("- Failed to update gradle.properties.\n");
			return FAILED;
		}
		r_report += fmod_editor_translate("- Updated gradle.properties.\n");
		return OK;
	}

	Error ModifyAndroidBuildTemplatePlugin::_patch_build_gradle(String& r_report) {
		String path = template_path.path_join("build.gradle");
		String text = _read_text_file(path);
		if (text.is_empty()) {
			r_report += fmod_editor_translate("- Failed to read build.gradle.\n");
			return FAILED;
		}

		if (text.contains("implementation files(\"libs/fmod.jar\")") || text.contains("implementation files('libs/fmod.jar')")) {
			r_report += fmod_editor_translate("- build.gradle already includes fmod.jar.\n");
			return OK;
		}

		String needle = "\n}\n\nandroid {";
		int idx = text.find(needle);
		if (idx < 0) {
			r_report += fmod_editor_translate("- Failed to locate the dependencies block in build.gradle.\n");
			return FAILED;
		}

		text = text.substr(0, idx) + "\n\n" + FMOD_JAR_GRADLE_LINE + text.substr(idx);
		if (!_write_text_file(path, text)) {
			r_report += fmod_editor_translate("- Failed to update build.gradle.\n");
			return FAILED;
		}
		r_report += fmod_editor_translate("- Updated build.gradle with fmod.jar dependency.\n");
		return OK;
	}

	Error ModifyAndroidBuildTemplatePlugin::_patch_godot_app_java(String& r_report) {
		String path = template_path.path_join("src/main/java/com/godot/game/GodotApp.java");
		String text = _read_text_file(path);
		if (text.is_empty()) {
			r_report += fmod_editor_translate("- Failed to read GodotApp.java.\n");
			return FAILED;
		}

		if (text.contains(FMOD_JAVA_MARKER)) {
			r_report += fmod_editor_translate("- GodotApp.java already contains the FMOD Android runtime loader.\n");
			return OK;
		}

		String load_block =
			"\t\t// FMOD Android runtime.\n"
			"\t\tif (BuildConfig.DEBUG) {\n"
			"\t\t\tSystem.loadLibrary(\"fmodL\");\n"
			"\t\t} else {\n"
			"\t\t\tSystem.loadLibrary(\"fmod\");\n"
			"\t\t}\n";

		PackedStringArray lines = text.split("\n", true);
		String cleaned;
		for (int i = 0; i < lines.size(); i++) {
			String line = lines[i];
			if (line.contains("System.loadLibrary(\"fmod\")") || line.contains("System.loadLibrary(\"fmodL\")") || line.contains(FMOD_JAVA_MARKER)) {
				continue;
			}
			cleaned += line;
			if (i < lines.size() - 1) {
				cleaned += "\n";
			}
		}

		String anchor = "\t\tSplashScreen.installSplashScreen(this);";
		int idx = cleaned.find(anchor);
		if (idx < 0) {
			anchor = "\t\tsuper.onCreate(savedInstanceState);";
			idx = cleaned.find(anchor);
		}
		if (idx < 0) {
			r_report += fmod_editor_translate("- Failed to locate GodotApp.onCreate insertion point.\n");
			return FAILED;
		}

		cleaned = cleaned.substr(0, idx) + load_block + cleaned.substr(idx);
		if (!_write_text_file(path, cleaned)) {
			r_report += fmod_editor_translate("- Failed to update GodotApp.java.\n");
			return FAILED;
		}
		r_report += fmod_editor_translate("- Updated GodotApp.java to load FMOD before Godot starts.\n");
		return OK;
	}

	Error ModifyAndroidBuildTemplatePlugin::_copy_file(const String& p_from, const String& p_to) {
		Error err = _ensure_dir_exists(p_to.get_base_dir());
		if (err != OK) {
			return err;
		}

		Ref<FileAccess> src = FileAccess::open(p_from, FileAccess::READ);
		ERR_FAIL_COND_V_MSG(src.is_null(), FAILED, "Cannot open source file: " + p_from);

		Ref<FileAccess> dst = FileAccess::open(p_to, FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(dst.is_null(), FAILED, "Cannot create destination file: " + p_to);

		PackedByteArray buffer = src->get_buffer(src->get_length());
		dst->store_buffer(buffer);
		dst->flush();
		return OK;
	}

	Error ModifyAndroidBuildTemplatePlugin::_ensure_dir_exists(const String& p_path) const {
		String global_path = ProjectSettings::get_singleton()->globalize_path(p_path);
		return DirAccess::make_dir_recursive_absolute(global_path);
	}

	bool ModifyAndroidBuildTemplatePlugin::_write_text_file(const String& p_path, const String& p_text) const {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
		if (file.is_null()) {
			return false;
		}
		file->store_string(p_text);
		file->flush();
		return true;
	}

	String ModifyAndroidBuildTemplatePlugin::_read_text_file(const String& p_path) const {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		if (file.is_null()) {
			return String();
		}
		return file->get_as_text();
	}
}
