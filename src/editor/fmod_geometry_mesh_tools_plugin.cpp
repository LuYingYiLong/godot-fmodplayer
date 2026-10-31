#include "editor/fmod_editor_translation.h"
#include "editor/fmod_geometry_mesh_tools_plugin.h"

#include "nodes/fmod_geometry_instance_3d.h"

#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/convex_polygon_shape3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/menu_button.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	void FmodGeometryMeshToolsPlugin::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_on_menu_id_pressed", "id"), &FmodGeometryMeshToolsPlugin::_on_menu_id_pressed);
	}

	void FmodGeometryMeshToolsPlugin::_notification(int p_what) {
		switch (p_what) {
            case NOTIFICATION_TRANSLATION_CHANGED: {
                if (menu_button) {
                    PopupMenu* popup = menu_button->get_popup();
                    const char* labels[] = {
                        FMOD_EDITOR_TEXT("Create Trimesh Occluder"),
                        FMOD_EDITOR_TEXT("Create Single Convex Occluder"),
                        FMOD_EDITOR_TEXT("Create Simplified Convex Occluder"),
                        FMOD_EDITOR_TEXT("Create Bounding Box Occluder"),
                    };
                    for (int i = 0; i < 4; ++i) popup->set_item_text(i, fmod_editor_translate(labels[i]));
                    popup->set_item_text(5, fmod_editor_translate("Create Mesh Resource Occluder"));
                    menu_button->set_tooltip_text(fmod_editor_translate("Create FMOD occlusion geometry from the selected MeshInstance3D."));
                }
            } break;
			case NOTIFICATION_ENTER_TREE: {
				_create_toolbar_menu();
			} break;

			case NOTIFICATION_EXIT_TREE: {
				_free_toolbar_menu();
			} break;
		}
	}

	String FmodGeometryMeshToolsPlugin::_get_plugin_name() const {
		return "FMOD Geometry Mesh Tools";
	}

	bool FmodGeometryMeshToolsPlugin::_has_main_screen() const {
		return false;
	}

	bool FmodGeometryMeshToolsPlugin::_handles(Object* p_object) const {
		return Object::cast_to<MeshInstance3D>(p_object) != nullptr;
	}

	void FmodGeometryMeshToolsPlugin::_edit(Object* p_object) {
		edited_mesh = Object::cast_to<MeshInstance3D>(p_object);
	}

	void FmodGeometryMeshToolsPlugin::_make_visible(bool p_visible) {
		if (menu_button) {
			menu_button->set_visible(p_visible);
		}
	}

	void FmodGeometryMeshToolsPlugin::_create_toolbar_menu() {
		if (menu_button) {
			return;
		}

		menu_button = memnew(MenuButton);
		menu_button->set_translation_domain("godot.editor");
		menu_button->set_text("FMOD Geometry");
		menu_button->set_tooltip_text(fmod_editor_translate("Create FMOD occlusion geometry from the selected MeshInstance3D."));
		menu_button->show();

		PopupMenu* popup = menu_button->get_popup();
		popup->add_item(fmod_editor_translate("Create Trimesh Occluder"), MENU_CREATE_TRIMESH);
		popup->add_item(fmod_editor_translate("Create Single Convex Occluder"), MENU_CREATE_CONVEX);
		popup->add_item(fmod_editor_translate("Create Simplified Convex Occluder"), MENU_CREATE_SIMPLIFIED_CONVEX);
		popup->add_item(fmod_editor_translate("Create Bounding Box Occluder"), MENU_CREATE_BOUNDING_BOX);
		popup->add_separator();
		popup->add_item(fmod_editor_translate("Create Mesh Resource Occluder"), MENU_CREATE_MESH_RESOURCE);
		popup->connect("id_pressed", callable_mp(this, &FmodGeometryMeshToolsPlugin::_on_menu_id_pressed));

		add_control_to_container(EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU, menu_button);

		error_dialog = memnew(AcceptDialog);
		error_dialog->set_translation_domain("godot.editor");
		error_dialog->set_title("FMOD Geometry");
		EditorInterface::get_singleton()->get_base_control()->add_child(error_dialog);
	}

	void FmodGeometryMeshToolsPlugin::_free_toolbar_menu() {
		if (menu_button) {
			remove_control_from_container(EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU, menu_button);
			menu_button->queue_free();
			menu_button = nullptr;
		}

		if (error_dialog) {
			error_dialog->queue_free();
			error_dialog = nullptr;
		}

		edited_mesh = nullptr;
	}

	void FmodGeometryMeshToolsPlugin::_show_error(const String& p_text) {
		if (!error_dialog) {
			UtilityFunctions::push_error(p_text);
			return;
		}

		error_dialog->set_text(p_text);
		error_dialog->popup_centered();
	}

	void FmodGeometryMeshToolsPlugin::_on_menu_id_pressed(int p_id) {
		_create_fmod_geometry_from_selection(static_cast<MenuOption>(p_id));
	}

	void FmodGeometryMeshToolsPlugin::_create_fmod_geometry_from_selection(MenuOption p_option) {
		std::vector<MeshInstance3D*> selected_meshes = _get_selected_mesh_instances();
		if (selected_meshes.empty()) {
			_show_error(fmod_editor_translate("Select one or more MeshInstance3D nodes first."));
			return;
		}

		Node* scene_root = EditorInterface::get_singleton()->get_edited_scene_root();
		if (!scene_root) {
			_show_error(fmod_editor_translate("No edited scene root is available."));
			return;
		}

		String action_name;
		switch (p_option) {
			case MENU_CREATE_TRIMESH:
				action_name = fmod_editor_translate("Create FMOD Trimesh Occluder");
				break;
			case MENU_CREATE_CONVEX:
				action_name = fmod_editor_translate("Create FMOD Convex Occluder");
				break;
			case MENU_CREATE_SIMPLIFIED_CONVEX:
				action_name = fmod_editor_translate("Create FMOD Simplified Convex Occluder");
				break;
			case MENU_CREATE_BOUNDING_BOX:
				action_name = fmod_editor_translate("Create FMOD Bounding Box Occluder");
				break;
			case MENU_CREATE_MESH_RESOURCE:
			action_name = fmod_editor_translate("Create FMOD Mesh Resource Occluder");
				break;
			default:
				action_name = fmod_editor_translate("Create FMOD Geometry Occluder");
				break;
		}

		EditorUndoRedoManager* undo_redo = get_undo_redo();
		undo_redo->create_action(action_name);

		int created_count = 0;
		for (MeshInstance3D* mesh_instance : selected_meshes) {
			if (!mesh_instance) {
				continue;
			}

			Ref<Mesh> mesh = mesh_instance->get_mesh();
			if (mesh.is_null()) {
				continue;
			}

			Node* parent = mesh_instance;

			FmodGeometryInstance3D* occluder = memnew(FmodGeometryInstance3D);
			occluder->set_name("FmodGeometryOccluder");
			occluder->set_direct_occlusion(1.0f);
			occluder->set_reverb_occlusion(0.75f);
			occluder->set_double_sided(true);
			occluder->set_active(true);
			occluder->set_auto_rebuild(true);
			occluder->set_sync_transform(true);
			occluder->set_recursive_source_scan(true);

			Transform3D local_transform;
			if (p_option == MENU_CREATE_MESH_RESOURCE) {
				occluder->set_source_mode(FmodGeometryInstance3D::SOURCE_MESH_RESOURCE);
				occluder->set_mesh(mesh);
				local_transform = Transform3D();
			} else {
				Transform3D shape_offset;
				Ref<Shape3D> shape = _create_shape_from_mesh(mesh, p_option, shape_offset);
				if (shape.is_null()) {
					memdelete(occluder);
					continue;
				}

				CollisionShape3D* collision_shape = memnew(CollisionShape3D);
				collision_shape->set_name("CollisionShape3D");
				collision_shape->set_shape(shape);
				occluder->add_child(collision_shape, true);

				occluder->set_source_mode(FmodGeometryInstance3D::SOURCE_COLLISION_SHAPES);
				local_transform = shape_offset;
			}

			occluder->set_transform(local_transform);

			undo_redo->add_do_method(parent, "add_child", occluder, true);
			undo_redo->add_do_method(occluder, "set_owner", scene_root);

			for (int i = 0; i < occluder->get_child_count(); i++) {
				Node* child = occluder->get_child(i);
				undo_redo->add_do_method(child, "set_owner", scene_root);
			}

			undo_redo->add_do_reference(occluder);
			undo_redo->add_undo_method(parent, "remove_child", occluder);
			created_count++;
		}

		if (created_count == 0) {
			undo_redo->commit_action();
			_show_error(fmod_editor_translate("Could not create FMOD geometry from the selected MeshInstance3D nodes."));
			return;
		}

		undo_redo->commit_action();
	}

	std::vector<MeshInstance3D*> FmodGeometryMeshToolsPlugin::_get_selected_mesh_instances() const {
		std::vector<MeshInstance3D*> result;

		EditorInterface* editor_interface = EditorInterface::get_singleton();
		if (!editor_interface) {
			return result;
		}

		EditorSelection* selection = editor_interface->get_selection();
		if (selection) {
			TypedArray<Node> selected_nodes = selection->get_selected_nodes();
			for (int i = 0; i < selected_nodes.size(); i++) {
				MeshInstance3D* mesh_instance = Object::cast_to<MeshInstance3D>(selected_nodes[i]);
				if (mesh_instance) {
					result.push_back(mesh_instance);
				}
			}
		}

		if (result.empty() && edited_mesh) {
			result.push_back(edited_mesh);
		}

		return result;
	}

	Ref<Shape3D> FmodGeometryMeshToolsPlugin::_create_shape_from_mesh(const Ref<Mesh>& p_mesh, MenuOption p_option, Transform3D& r_shape_offset) const {
		r_shape_offset = Transform3D();

		switch (p_option) {
			case MENU_CREATE_TRIMESH:
				return p_mesh->create_trimesh_shape();

			case MENU_CREATE_CONVEX:
				return p_mesh->create_convex_shape(true, false);

			case MENU_CREATE_SIMPLIFIED_CONVEX:
				return p_mesh->create_convex_shape(true, true);

			case MENU_CREATE_BOUNDING_BOX: {
				Ref<BoxShape3D> box_shape;
				box_shape.instantiate();
				AABB aabb = p_mesh->get_aabb();
				box_shape->set_size(aabb.get_size().maxf(0.001f));
				r_shape_offset.origin = aabb.get_center();
				return box_shape;
			}

			default:
				break;
		}

		return Ref<Shape3D>();
	}
}
