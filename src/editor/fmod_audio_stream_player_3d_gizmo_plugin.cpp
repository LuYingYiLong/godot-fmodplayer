#include "editor/fmod_audio_stream_player_3d_gizmo_plugin.h"

#include "nodes/fmod_audio_stream_player_3d.h"
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/geometry3d.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/editor_settings.hpp>

namespace godot {
	FmodAudioStreamPlayer3DGizmoPlugin::FmodAudioStreamPlayer3DGizmoPlugin() {
		Color gizmo_color = EditorInterface::get_singleton()->get_editor_settings()->get_setting("editors/3d_gizmos/gizmo_colors/stream_player_3d");

		create_icon_material("stream_player_3d_icon", FmodUtils::get_editor_theme_icon("Gizmo3DSamplePlayer"));
		create_material("stream_player_3d_material_primary", gizmo_color);
		create_material("stream_player_3d_material_secondary", gizmo_color * Color(1, 1, 1, 0.35));
		// 为广告牌材质启用顶点颜色，因为控件颜色取决于
		// FmodAudioStreamPlayer3D 的衰减类型和来源 (单位大小或最大距离)
		create_material("stream_player_3d_material_billboard", Color(1, 1, 1), true, false, true);
		create_handle_material("handles");
	}

	bool FmodAudioStreamPlayer3DGizmoPlugin::_has_gizmo(Node3D* p_for_node_3d) const {
		return Object::cast_to<FmodAudioStreamPlayer3D>(p_for_node_3d) != nullptr;
	}

	String FmodAudioStreamPlayer3DGizmoPlugin::_get_gizmo_name() const {
		return "FmodAudioStreamPlayer3D";
	}

	int32_t FmodAudioStreamPlayer3DGizmoPlugin::_get_priority() const {
		return -1;
	}

	void FmodAudioStreamPlayer3DGizmoPlugin::_redraw(const Ref<EditorNode3DGizmo>& p_gizmo) {
		p_gizmo->clear();

		FmodAudioStreamPlayer3D* player = Object::cast_to<FmodAudioStreamPlayer3D>(p_gizmo->get_node_3d());
		if (!player) return;

		if (player->get_attenuation_model() != FmodAudioStreamPlayer3D::ATTENUATION_DISABLED || player->get_max_distance() > CMP_EPSILON) {
			// 绘制一个圆来表示音量衰减
			// 使用仅包含广告牌的圆来表示半径
			// 这有助于区分 AudioStreamPlayer3D 小工具和 OmniLight3D 小工具
			const Ref<Material> lines_billboard_material = get_material("stream_player_3d_material_billboard", p_gizmo);

			// 软距离上限取决于衰减模型，因为有些模型的衰减比其他模型更快
			// 系数是通过测试经验确定的
			float soft_multiplier = 0.0f;
			switch (player->get_attenuation_model()) {
			case FmodAudioStreamPlayer3D::ATTENUATION_INVERSE_DISTANCE:
				soft_multiplier = 12.0f;
				break;

			case FmodAudioStreamPlayer3D::ATTENUATION_INVERSE_SQUARE_DISTANCE:
				soft_multiplier = 4.0f;
				break;

			case FmodAudioStreamPlayer3D::ATTENUATION_LOGARITHMIC:
				// 确保最大距离的半径可视化不会被单位大小限制
				// (当衰减模式被禁用时)
				soft_multiplier = 10000.0f;
				break;
			}

			// 绘制声音可以合理听到的距离
			// 这可以是通过最大距离属性 (如果设置为大于 0.0) 实现的硬距离上限
			// 或者通过单位大小属性实现的软距离上限 (声音永远不会真正降为零)
			// 当最大距离为 0.0 时，`r` 表示在大多数 (但不是全部) 情况下声音无法听到的距离
			float radius = player->get_unit_size() * soft_multiplier;
			if (player->get_max_distance() > CMP_EPSILON) {
				radius = MIN(radius, player->get_max_distance());
			}

#define PUSH_QUARTER_XY(m_from_x, m_from_y, m_to_x, m_to_y, m_y)   \
	points_ptrw[index++] = Vector3(m_from_x, -m_from_y - m_y, 0);  \
	points_ptrw[index++] = Vector3(m_to_x, -m_to_y - m_y, 0);      \
	points_ptrw[index++] = Vector3(m_from_x, m_from_y + m_y, 0);   \
	points_ptrw[index++] = Vector3(m_to_x, m_to_y + m_y, 0);       \
	points_ptrw[index++] = Vector3(-m_from_x, -m_from_y - m_y, 0); \
	points_ptrw[index++] = Vector3(-m_to_x, -m_to_y - m_y, 0);     \
	points_ptrw[index++] = Vector3(-m_from_x, m_from_y + m_y, 0);  \
	points_ptrw[index++] = Vector3(-m_to_x, m_to_y + m_y, 0);

			// 八分之一象限中的点数，因此总共有 8 * points_in_octant 个点
			// 这对应于圆的平滑度
			const uint32_t points_in_octant = 15;
			const real_t octant_angle = Math::PI / 4;
			const real_t inc = (Math::PI / (4 * points_in_octant));
			const real_t radius_squared = radius * radius;
			real_t r = 0.0f;

			PackedVector3Array points_billboard;
			points_billboard.resize(8 * points_in_octant * 2);
			Vector3* points_ptrw = points_billboard.ptrw();

			uint32_t index = 0;
			float previous_x = radius;
			float previous_y = 0.0f;

			for (uint32_t i = 0; i < points_in_octant; i++) {
				r += inc;
				real_t x = Math::cos((i == points_in_octant - 1) ? octant_angle : r) * radius;
				real_t y = Math::sqrt(radius_squared - (x * x));

				PUSH_QUARTER_XY(previous_x, previous_y, x, y, 0);
				PUSH_QUARTER_XY(previous_y, previous_x, y, x, 0);
				previous_x = x;
				previous_y = y;
			}

#undef PUSH_QUARTER_XY

			Color color;
			switch (player->get_attenuation_model()) {
				// 为所有衰减模型 (除了禁用) 选择冷色调
				// 这样软上限就可以很容易地区分硬上限
				// (硬上限使用暖色调)
			case FmodAudioStreamPlayer3D::ATTENUATION_INVERSE_DISTANCE:
				color = Color(0.4, 0.8, 1);
				break;

			case FmodAudioStreamPlayer3D::ATTENUATION_INVERSE_SQUARE_DISTANCE:
				color = Color(0.4, 0.5, 1);
				break;

			case FmodAudioStreamPlayer3D::ATTENUATION_LOGARITHMIC:
				color = Color(0.4, 0.2, 1);
				break;

			default:
				// 禁用衰减模式
				// 当最大距离为 0 时，这段代码不会被执行
				// 但如果最大距离大于 0，将使用该颜色的色相反转形式
				color = Color(1, 1, 1);
				break;
			}

			if (player->get_max_distance() > CMP_EPSILON) {
				// 声音被最大距离严格限制，衰减模型仍然很重要
				// 因此反转上面选择的颜色的色相
				color.set_h(color.get_h() + 0.5);
			}

			p_gizmo->add_lines(points_billboard, lines_billboard_material, true, color);
		}

		if (player->is_emission_angle_enabled()) {
			const float ha = Math::deg_to_rad(player->get_emission_angle());
			const float ofs = -Math::cos(ha);
			const float radius = Math::sin(ha);

			const uint32_t points_in_octant = 7;
			const real_t octant_angle = Math::PI / 4;
			const real_t inc = (Math::PI / (4 * points_in_octant));
			const real_t radius_squared = radius * radius;
			real_t r = 0;

			PackedVector3Array points_primary;
			points_primary.resize(8 * points_in_octant * 2);
			Vector3* points_ptrw = points_primary.ptrw();

			uint32_t index = 0;
			float previous_x = radius;
			float previous_y = 0.f;

#define PUSH_QUARTER(m_from_x, m_from_y, m_to_x, m_to_y, m_y)  \
	points_ptrw[index++] = Vector3(m_from_x, -m_from_y, m_y);  \
	points_ptrw[index++] = Vector3(m_to_x, -m_to_y, m_y);      \
	points_ptrw[index++] = Vector3(m_from_x, m_from_y, m_y);   \
	points_ptrw[index++] = Vector3(m_to_x, m_to_y, m_y);       \
	points_ptrw[index++] = Vector3(-m_from_x, -m_from_y, m_y); \
	points_ptrw[index++] = Vector3(-m_to_x, -m_to_y, m_y);     \
	points_ptrw[index++] = Vector3(-m_from_x, m_from_y, m_y);  \
	points_ptrw[index++] = Vector3(-m_to_x, m_to_y, m_y);

			for (uint32_t i = 0; i < points_in_octant; i++) {
				r += inc;
				real_t x = Math::cos((i == points_in_octant - 1) ? octant_angle : r) * radius;
				real_t y = Math::sqrt(radius_squared - (x * x));

				PUSH_QUARTER(previous_x, previous_y, x, y, ofs);
				PUSH_QUARTER(previous_y, previous_x, y, x, ofs);

				previous_x = x;
				previous_y = y;
			}
#undef PUSH_QUARTER

			const Ref<Material> material_primary = get_material("stream_player_3d_material_primary", p_gizmo);
			p_gizmo->add_lines(points_primary, material_primary);

			PackedVector3Array points_secondary;
			points_secondary.resize(16);
			Vector3* points_second_ptrw = points_secondary.ptrw();
			uint32_t index2 = 0;
			// 指向圆的线
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(radius, 0, ofs);
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(-radius, 0, ofs);
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(0, radius, ofs);
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(0, -radius, ofs);
			real_t octant_value = Math::cos(octant_angle) * radius;
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(octant_value, octant_value, ofs);
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(-octant_value, octant_value, ofs);
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(-octant_value, -octant_value, ofs);
			points_second_ptrw[index2++] = Vector3();
			points_second_ptrw[index2++] = Vector3(octant_value, -octant_value, ofs);

			const Ref<Material> material_secondary = get_material("stream_player_3d_material_secondary", p_gizmo);
			p_gizmo->add_lines(points_secondary, material_secondary);

			PackedVector3Array handles;
			handles.push_back(Vector3(Math::sin(ha), 0, -Math::cos(ha)));
			p_gizmo->add_handles(handles, get_material("handles"), PackedInt32Array());
		}

		const Ref<Material> icon = get_material("stream_player_3d_icon", p_gizmo);
		p_gizmo->add_unscaled_billboard(icon, 0.05);
	}
}
