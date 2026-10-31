#include "editor/fmod_reverb_zone_3d_gizmo_plugin.h"

#include "core/fmod_utils.hpp"
#include "nodes/fmod_reverb_zone_3d.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
	FmodReverbZone3DGizmoPlugin::FmodReverbZone3DGizmoPlugin() {
		Color outer_color = Color(0.25f, 0.8f, 1.0f, 0.65f);
		Color inner_color = Color(1.0f, 0.85f, 0.25f, 0.9f);
		create_icon_material("reverb_zone_icon", FmodUtils::get_editor_theme_icon("GizmoReflectionProbe"));
		create_material("reverb_zone_outer", outer_color);
		create_material("reverb_zone_inner", inner_color);
	}

	bool FmodReverbZone3DGizmoPlugin::_has_gizmo(Node3D* p_for_node_3d) const {
		return Object::cast_to<FmodReverbZone3D>(p_for_node_3d) != nullptr;
	}

	String FmodReverbZone3DGizmoPlugin::_get_gizmo_name() const {
		return "FmodReverbZone3D";
	}

	int32_t FmodReverbZone3DGizmoPlugin::_get_priority() const {
		return -1;
	}

	PackedVector3Array FmodReverbZone3DGizmoPlugin::_make_sphere_lines(float p_radius) const {
		PackedVector3Array points;
		const int segments = 64;
		points.resize(segments * 2 * 3);
		Vector3* ptr = points.ptrw();
		int index = 0;

		for (int i = 0; i < segments; i++) {
			float a0 = Math::TAU * static_cast<float>(i) / static_cast<float>(segments);
			float a1 = Math::TAU * static_cast<float>(i + 1) / static_cast<float>(segments);
			float x0 = Math::cos(a0) * p_radius;
			float y0 = Math::sin(a0) * p_radius;
			float x1 = Math::cos(a1) * p_radius;
			float y1 = Math::sin(a1) * p_radius;

			ptr[index++] = Vector3(x0, y0, 0.0f);
			ptr[index++] = Vector3(x1, y1, 0.0f);
			ptr[index++] = Vector3(x0, 0.0f, y0);
			ptr[index++] = Vector3(x1, 0.0f, y1);
			ptr[index++] = Vector3(0.0f, x0, y0);
			ptr[index++] = Vector3(0.0f, x1, y1);
		}

		return points;
	}

	void FmodReverbZone3DGizmoPlugin::_redraw(const Ref<EditorNode3DGizmo>& p_gizmo) {
		p_gizmo->clear();

		FmodReverbZone3D* zone = Object::cast_to<FmodReverbZone3D>(p_gizmo->get_node_3d());
		if (!zone) return;

		Ref<Material> outer = get_material("reverb_zone_outer", p_gizmo);
		Ref<Material> inner = get_material("reverb_zone_inner", p_gizmo);

		p_gizmo->add_lines(_make_sphere_lines(zone->get_max_distance()), outer);
		if (zone->get_min_distance() > CMP_EPSILON) {
			p_gizmo->add_lines(_make_sphere_lines(zone->get_min_distance()), inner);
		}

		Ref<Material> icon = get_material("reverb_zone_icon", p_gizmo);
		p_gizmo->add_unscaled_billboard(icon, 0.05f);
	}
}
