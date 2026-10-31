#include "nodes/fmod_geometry_instance_3d.h"

#include "core/fmod_server.h"
#include "core/fmod_system.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/classes/convex_polygon_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/classes/sphere_shape3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	void FmodGeometryInstance3D::_bind_methods() {
		BIND_ENUM_CONSTANT(SOURCE_AUTO);
		BIND_ENUM_CONSTANT(SOURCE_COLLISION_SHAPES);
		BIND_ENUM_CONSTANT(SOURCE_MESH_INSTANCE);
		BIND_ENUM_CONSTANT(SOURCE_MESH_RESOURCE);

		ClassDB::bind_method(D_METHOD("set_source_mode", "mode"), &FmodGeometryInstance3D::set_source_mode);
		ClassDB::bind_method(D_METHOD("get_source_mode"), &FmodGeometryInstance3D::get_source_mode);

		ClassDB::bind_method(D_METHOD("set_source_node_path", "path"), &FmodGeometryInstance3D::set_source_node_path);
		ClassDB::bind_method(D_METHOD("get_source_node_path"), &FmodGeometryInstance3D::get_source_node_path);

		ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &FmodGeometryInstance3D::set_mesh);
		ClassDB::bind_method(D_METHOD("get_mesh"), &FmodGeometryInstance3D::get_mesh);

		ClassDB::bind_method(D_METHOD("set_direct_occlusion", "value"), &FmodGeometryInstance3D::set_direct_occlusion);
		ClassDB::bind_method(D_METHOD("get_direct_occlusion"), &FmodGeometryInstance3D::get_direct_occlusion);

		ClassDB::bind_method(D_METHOD("set_reverb_occlusion", "value"), &FmodGeometryInstance3D::set_reverb_occlusion);
		ClassDB::bind_method(D_METHOD("get_reverb_occlusion"), &FmodGeometryInstance3D::get_reverb_occlusion);

		ClassDB::bind_method(D_METHOD("set_double_sided", "value"), &FmodGeometryInstance3D::set_double_sided);
		ClassDB::bind_method(D_METHOD("get_double_sided"), &FmodGeometryInstance3D::get_double_sided);

		ClassDB::bind_method(D_METHOD("set_active", "value"), &FmodGeometryInstance3D::set_active);
		ClassDB::bind_method(D_METHOD("get_active"), &FmodGeometryInstance3D::get_active);

		ClassDB::bind_method(D_METHOD("set_auto_rebuild", "value"), &FmodGeometryInstance3D::set_auto_rebuild);
		ClassDB::bind_method(D_METHOD("get_auto_rebuild"), &FmodGeometryInstance3D::get_auto_rebuild);

		ClassDB::bind_method(D_METHOD("set_sync_transform", "value"), &FmodGeometryInstance3D::set_sync_transform);
		ClassDB::bind_method(D_METHOD("get_sync_transform"), &FmodGeometryInstance3D::get_sync_transform);

		ClassDB::bind_method(D_METHOD("set_recursive_source_scan", "value"), &FmodGeometryInstance3D::set_recursive_source_scan);
		ClassDB::bind_method(D_METHOD("get_recursive_source_scan"), &FmodGeometryInstance3D::get_recursive_source_scan);

		ClassDB::bind_method(D_METHOD("set_primitive_segments", "segments"), &FmodGeometryInstance3D::set_primitive_segments);
		ClassDB::bind_method(D_METHOD("get_primitive_segments"), &FmodGeometryInstance3D::get_primitive_segments);

		ClassDB::bind_method(D_METHOD("set_show_debug_gizmo", "value"), &FmodGeometryInstance3D::set_show_debug_gizmo);
		ClassDB::bind_method(D_METHOD("get_show_debug_gizmo"), &FmodGeometryInstance3D::get_show_debug_gizmo);

		ClassDB::bind_method(D_METHOD("rebuild_geometry"), &FmodGeometryInstance3D::rebuild_geometry);
		ClassDB::bind_method(D_METHOD("clear_geometry"), &FmodGeometryInstance3D::clear_geometry);
		ClassDB::bind_method(D_METHOD("has_valid_geometry"), &FmodGeometryInstance3D::has_valid_geometry);
		ClassDB::bind_method(D_METHOD("sync_geometry_transform"), &FmodGeometryInstance3D::sync_geometry_transform);
		ClassDB::bind_method(D_METHOD("get_polygon_count"), &FmodGeometryInstance3D::get_polygon_count);
		ClassDB::bind_method(D_METHOD("get_vertex_count"), &FmodGeometryInstance3D::get_vertex_count);
		ClassDB::bind_method(D_METHOD("get_geometry"), &FmodGeometryInstance3D::get_geometry);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "source_mode", PROPERTY_HINT_ENUM, "Auto,Collision Shapes,Mesh Instance,Mesh Resource"), "set_source_mode", "get_source_mode");
		ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "source_node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D,MeshInstance3D,CollisionShape3D"), "set_source_node_path", "get_source_node_path");
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "direct_occlusion", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_direct_occlusion", "get_direct_occlusion");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reverb_occlusion", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_reverb_occlusion", "get_reverb_occlusion");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "double_sided"), "set_double_sided", "get_double_sided");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "get_active");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_rebuild"), "set_auto_rebuild", "get_auto_rebuild");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sync_transform"), "set_sync_transform", "get_sync_transform");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "recursive_source_scan"), "set_recursive_source_scan", "get_recursive_source_scan");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "primitive_segments", PROPERTY_HINT_RANGE, "6,64,1"), "set_primitive_segments", "get_primitive_segments");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug_gizmo"), "set_show_debug_gizmo", "get_show_debug_gizmo");

		ADD_SIGNAL(MethodInfo("geometry_created"));
		ADD_SIGNAL(MethodInfo("geometry_cleared"));
		ADD_SIGNAL(MethodInfo("geometry_rebuilt"));
	}

	FmodGeometryInstance3D::FmodGeometryInstance3D() {
		set_process(true);
	}

	FmodGeometryInstance3D::~FmodGeometryInstance3D() {
		clear_geometry();
	}

	void FmodGeometryInstance3D::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_READY: {
			if (auto_rebuild) {
				call_deferred("rebuild_geometry");
			}
		} break;

		case NOTIFICATION_PROCESS: {
			if (sync_transform && has_valid_geometry()) {
				sync_geometry_transform();
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			clear_geometry();
		} break;

#ifdef TOOLS_ENABLED
		case NOTIFICATION_EDITOR_PRE_SAVE: {
			if (auto_rebuild && is_inside_tree() && !has_valid_geometry()) {
				rebuild_geometry();
			}
		} break;
#endif
		}
	}

	void FmodGeometryInstance3D::rebuild_geometry() {
		if (!is_inside_tree()) {
			return;
		}

		clear_geometry();

		std::vector<PackedVector3Array> polygons;
		bool success = false;

		Node* configured_source = _get_configured_source_node();
		SourceMode effective_mode = source_mode;

		if (effective_mode == SOURCE_AUTO) {
			if (configured_source) {
				if (Object::cast_to<MeshInstance3D>(configured_source)) {
					effective_mode = SOURCE_MESH_INSTANCE;
				}
				else {
					effective_mode = SOURCE_COLLISION_SHAPES;
				}
			}
			else if (mesh.is_valid()) {
				effective_mode = SOURCE_MESH_RESOURCE;
			}
			else {
				effective_mode = SOURCE_COLLISION_SHAPES;
			}
		}

		if (effective_mode == SOURCE_MESH_RESOURCE) {
			success = _build_from_mesh(mesh, Transform3D(), polygons);
		}
		else if (effective_mode == SOURCE_MESH_INSTANCE) {
			MeshInstance3D* mesh_instance = Object::cast_to<MeshInstance3D>(configured_source);
			if (!mesh_instance && source_node_path.is_empty()) {
				mesh_instance = Object::cast_to<MeshInstance3D>(get_parent());
			}
			success = _build_from_mesh_instance(mesh_instance, polygons);
		}
		else {
			Node* root = configured_source ? configured_source : this;
			success = _build_from_collision_shapes(root, polygons);

			if (!success && source_mode == SOURCE_AUTO) {
				MeshInstance3D* mesh_instance = Object::cast_to<MeshInstance3D>(get_parent());
				success = _build_from_mesh_instance(mesh_instance, polygons);
			}
		}

		if (!success || polygons.empty()) {
			UtilityFunctions::push_warning("FmodGeometryInstance3D found no usable geometry.");
			return;
		}

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized.");

		int max_vertices = 0;
		for (const PackedVector3Array& polygon : polygons) {
			max_vertices += polygon.size();
		}

			geometry = system->create_geometry(static_cast<int>(polygons.size()), max_vertices);
		ERR_FAIL_COND_MSG(geometry.is_null(), "Failed to create FMOD geometry.");

		for (const PackedVector3Array& polygon : polygons) {
			int index = geometry->add_polygon(direct_occlusion, reverb_occlusion, double_sided, polygon);
			if (index >= 0) {
				polygon_count++;
				vertex_count += polygon.size();
			}
		}

		geometry->set_active(active);
		sync_geometry_transform();

		emit_signal("geometry_created");
		emit_signal("geometry_rebuilt");

#ifdef TOOLS_ENABLED
		update_gizmos();
#endif
	}

	void FmodGeometryInstance3D::clear_geometry() {
		if (geometry.is_valid()) {
			geometry->release();
			geometry.unref();
			polygon_count = 0;
			vertex_count = 0;
			emit_signal("geometry_cleared");
		}
	}

	bool FmodGeometryInstance3D::has_valid_geometry() const {
		return geometry.is_valid() && geometry->geometry_is_valid();
	}

	void FmodGeometryInstance3D::sync_geometry_transform() {
		if (!is_inside_tree() || !has_valid_geometry()) {
			return;
		}
		geometry->set_transform(get_global_transform());
		geometry->set_active(active);
	}

	PackedVector3Array FmodGeometryInstance3D::get_mesh_triangles(Ref<Mesh> p_mesh) {
		PackedVector3Array result;

		if (!p_mesh.is_valid()) {
			return result;
		}

		for (int surface = 0; surface < p_mesh->get_surface_count(); surface++) {
			Array arrays = p_mesh->surface_get_arrays(surface);
			if (arrays.is_empty()) {
				continue;
			}

			PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
			PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];

			if (vertices.is_empty()) {
				continue;
			}

			if (indices.is_empty()) {
				for (int i = 0; i + 2 < vertices.size(); i += 3) {
					result.append(vertices[i]);
					result.append(vertices[i + 1]);
					result.append(vertices[i + 2]);
				}
			}
			else {
				for (int i = 0; i + 2 < indices.size(); i += 3) {
					result.append(vertices[indices[i]]);
					result.append(vertices[indices[i + 1]]);
					result.append(vertices[indices[i + 2]]);
				}
			}
		}

		return result;
	}

	Node* FmodGeometryInstance3D::_get_configured_source_node() const {
		if (source_node_path.is_empty() || !is_inside_tree()) {
			return nullptr;
		}
		return get_node_or_null(source_node_path);
	}

	void FmodGeometryInstance3D::_collect_collision_shapes(Node* p_root, std::vector<CollisionShape3D*>& r_shapes) const {
		if (!p_root) {
			return;
		}

		CollisionShape3D* shape = Object::cast_to<CollisionShape3D>(p_root);
		if (shape && !shape->is_disabled()) {
			r_shapes.push_back(shape);
			if (!recursive_source_scan) {
				return;
			}
		}

		for (int i = 0; i < p_root->get_child_count(); i++) {
			Node* child = p_root->get_child(i);
			if (!child) {
				continue;
			}

			if (recursive_source_scan) {
				_collect_collision_shapes(child, r_shapes);
			}
			else {
				CollisionShape3D* child_shape = Object::cast_to<CollisionShape3D>(child);
				if (child_shape && !child_shape->is_disabled()) {
					r_shapes.push_back(child_shape);
				}
			}
		}
	}

	bool FmodGeometryInstance3D::_build_from_collision_shapes(Node* p_root, std::vector<PackedVector3Array>& r_polygons) const {
		std::vector<CollisionShape3D*> shapes;
		_collect_collision_shapes(p_root, shapes);

		for (CollisionShape3D* shape : shapes) {
			_build_from_collision_shape(shape, r_polygons);
		}

		return !r_polygons.empty();
	}

	bool FmodGeometryInstance3D::_build_from_collision_shape(CollisionShape3D* p_collision_shape, std::vector<PackedVector3Array>& r_polygons) const {
		if (!p_collision_shape) {
			return false;
		}
		if (!is_inside_tree() || !p_collision_shape->is_inside_tree()) {
			return false;
		}

		Ref<Shape3D> shape = p_collision_shape->get_shape();
		if (!shape.is_valid()) {
			UtilityFunctions::push_warning("CollisionShape3D has no Shape3D: " + p_collision_shape->get_name());
			return false;
		}

		Transform3D to_geometry_local = get_global_transform().affine_inverse() * p_collision_shape->get_global_transform();
		return _build_from_shape(shape, to_geometry_local, r_polygons);
	}

	bool FmodGeometryInstance3D::_build_from_mesh_instance(MeshInstance3D* p_mesh_instance, std::vector<PackedVector3Array>& r_polygons) const {
		if (!p_mesh_instance) {
			return false;
		}
		if (!is_inside_tree() || !p_mesh_instance->is_inside_tree()) {
			return false;
		}

		Ref<Mesh> source_mesh = p_mesh_instance->get_mesh();
		if (!source_mesh.is_valid()) {
			UtilityFunctions::push_warning("MeshInstance3D has no Mesh: " + p_mesh_instance->get_name());
			return false;
		}

		Transform3D to_geometry_local = get_global_transform().affine_inverse() * p_mesh_instance->get_global_transform();
		return _build_from_mesh(source_mesh, to_geometry_local, r_polygons);
	}

	bool FmodGeometryInstance3D::_build_from_mesh(const Ref<Mesh>& p_mesh, const Transform3D& p_to_geometry_local, std::vector<PackedVector3Array>& r_polygons) const {
		PackedVector3Array triangles = get_mesh_triangles(p_mesh);
		if (triangles.size() < 3) {
			return false;
		}

		for (int i = 0; i + 2 < triangles.size(); i += 3) {
			_append_triangle(triangles[i], triangles[i + 1], triangles[i + 2], p_to_geometry_local, r_polygons);
		}

		return true;
	}

	bool FmodGeometryInstance3D::_build_from_shape(const Ref<Shape3D>& p_shape, const Transform3D& p_to_geometry_local, std::vector<PackedVector3Array>& r_polygons) const {
		if (!p_shape.is_valid()) {
			return false;
		}

		BoxShape3D* box = Object::cast_to<BoxShape3D>(p_shape.ptr());
		if (box) {
			_append_box(box->get_size(), p_to_geometry_local, r_polygons);
			return true;
		}

		SphereShape3D* sphere = Object::cast_to<SphereShape3D>(p_shape.ptr());
		if (sphere) {
			_append_sphere(sphere->get_radius(), p_to_geometry_local, r_polygons);
			return true;
		}

		CylinderShape3D* cylinder = Object::cast_to<CylinderShape3D>(p_shape.ptr());
		if (cylinder) {
			_append_cylinder(cylinder->get_radius(), cylinder->get_height(), p_to_geometry_local, r_polygons);
			return true;
		}

		ConcavePolygonShape3D* concave = Object::cast_to<ConcavePolygonShape3D>(p_shape.ptr());
		if (concave) {
			PackedVector3Array faces = concave->get_faces();
			for (int i = 0; i + 2 < faces.size(); i += 3) {
				_append_triangle(faces[i], faces[i + 1], faces[i + 2], p_to_geometry_local, r_polygons);
			}
			return faces.size() >= 3;
		}

		ConvexPolygonShape3D* convex = Object::cast_to<ConvexPolygonShape3D>(p_shape.ptr());
		if (convex) {
			Ref<ArrayMesh> debug_mesh = p_shape->get_debug_mesh();
			if (debug_mesh.is_valid()) {
				Ref<Mesh> debug_as_mesh = debug_mesh;
				return _build_from_mesh(debug_as_mesh, p_to_geometry_local, r_polygons);
			}

			PackedVector3Array points = convex->get_points();
			if (!points.is_empty()) {
				Vector3 min = points[0];
				Vector3 max = points[0];
				for (int i = 1; i < points.size(); i++) {
					min.x = MIN(min.x, points[i].x);
					min.y = MIN(min.y, points[i].y);
					min.z = MIN(min.z, points[i].z);
					max.x = MAX(max.x, points[i].x);
					max.y = MAX(max.y, points[i].y);
					max.z = MAX(max.z, points[i].z);
				}
				_append_box(max - min, p_to_geometry_local * Transform3D(Basis(), (min + max) * 0.5f), r_polygons);
				return true;
			}
		}

		Ref<ArrayMesh> debug_mesh = p_shape->get_debug_mesh();
		if (debug_mesh.is_valid()) {
			Ref<Mesh> debug_as_mesh = debug_mesh;
			return _build_from_mesh(debug_as_mesh, p_to_geometry_local, r_polygons);
		}

		UtilityFunctions::push_warning("Unsupported Shape3D for FmodGeometryInstance3D: " + p_shape->get_class());
		return false;
	}

	void FmodGeometryInstance3D::_append_box(const Vector3& p_size, const Transform3D& p_xform, std::vector<PackedVector3Array>& r_polygons) const {
		Vector3 h = p_size * 0.5f;
		Vector3 v[8] = {
			Vector3(-h.x, -h.y, -h.z),
			Vector3(h.x, -h.y, -h.z),
			Vector3(h.x, h.y, -h.z),
			Vector3(-h.x, h.y, -h.z),
			Vector3(-h.x, -h.y, h.z),
			Vector3(h.x, -h.y, h.z),
			Vector3(h.x, h.y, h.z),
			Vector3(-h.x, h.y, h.z),
		};

		_append_triangle(v[0], v[1], v[2], p_xform, r_polygons);
		_append_triangle(v[0], v[2], v[3], p_xform, r_polygons);
		_append_triangle(v[5], v[4], v[7], p_xform, r_polygons);
		_append_triangle(v[5], v[7], v[6], p_xform, r_polygons);
		_append_triangle(v[4], v[0], v[3], p_xform, r_polygons);
		_append_triangle(v[4], v[3], v[7], p_xform, r_polygons);
		_append_triangle(v[1], v[5], v[6], p_xform, r_polygons);
		_append_triangle(v[1], v[6], v[2], p_xform, r_polygons);
		_append_triangle(v[3], v[2], v[6], p_xform, r_polygons);
		_append_triangle(v[3], v[6], v[7], p_xform, r_polygons);
		_append_triangle(v[4], v[5], v[1], p_xform, r_polygons);
		_append_triangle(v[4], v[1], v[0], p_xform, r_polygons);
	}

	void FmodGeometryInstance3D::_append_sphere(float p_radius, const Transform3D& p_xform, std::vector<PackedVector3Array>& r_polygons) const {
		int segments = CLAMP(primitive_segments, 6, 64);
		int rings = MAX(3, segments / 2);

		for (int y = 0; y < rings; y++) {
			float phi0 = -Math::PI * 0.5f + Math::PI * static_cast<float>(y) / static_cast<float>(rings);
			float phi1 = -Math::PI * 0.5f + Math::PI * static_cast<float>(y + 1) / static_cast<float>(rings);

			for (int x = 0; x < segments; x++) {
				float theta0 = Math::TAU * static_cast<float>(x) / static_cast<float>(segments);
				float theta1 = Math::TAU * static_cast<float>(x + 1) / static_cast<float>(segments);

				Vector3 a(Math::cos(phi0) * Math::cos(theta0), Math::sin(phi0), Math::cos(phi0) * Math::sin(theta0));
				Vector3 b(Math::cos(phi0) * Math::cos(theta1), Math::sin(phi0), Math::cos(phi0) * Math::sin(theta1));
				Vector3 c(Math::cos(phi1) * Math::cos(theta1), Math::sin(phi1), Math::cos(phi1) * Math::sin(theta1));
				Vector3 d(Math::cos(phi1) * Math::cos(theta0), Math::sin(phi1), Math::cos(phi1) * Math::sin(theta0));

				_append_triangle(a * p_radius, b * p_radius, c * p_radius, p_xform, r_polygons);
				_append_triangle(a * p_radius, c * p_radius, d * p_radius, p_xform, r_polygons);
			}
		}
	}

	void FmodGeometryInstance3D::_append_cylinder(float p_radius, float p_height, const Transform3D& p_xform, std::vector<PackedVector3Array>& r_polygons) const {
		int segments = CLAMP(primitive_segments, 6, 64);
		float half_height = p_height * 0.5f;
		Vector3 top_center(0.0f, half_height, 0.0f);
		Vector3 bottom_center(0.0f, -half_height, 0.0f);

		for (int i = 0; i < segments; i++) {
			float a0 = Math::TAU * static_cast<float>(i) / static_cast<float>(segments);
			float a1 = Math::TAU * static_cast<float>(i + 1) / static_cast<float>(segments);
			Vector3 bottom0(Math::cos(a0) * p_radius, -half_height, Math::sin(a0) * p_radius);
			Vector3 bottom1(Math::cos(a1) * p_radius, -half_height, Math::sin(a1) * p_radius);
			Vector3 top0(Math::cos(a0) * p_radius, half_height, Math::sin(a0) * p_radius);
			Vector3 top1(Math::cos(a1) * p_radius, half_height, Math::sin(a1) * p_radius);

			_append_triangle(bottom0, bottom1, top1, p_xform, r_polygons);
			_append_triangle(bottom0, top1, top0, p_xform, r_polygons);
			_append_triangle(top_center, top0, top1, p_xform, r_polygons);
			_append_triangle(bottom_center, bottom1, bottom0, p_xform, r_polygons);
		}
	}

	void FmodGeometryInstance3D::_append_triangle(const Vector3& p_a, const Vector3& p_b, const Vector3& p_c, const Transform3D& p_xform, std::vector<PackedVector3Array>& r_polygons) const {
		Vector3 a = p_xform.xform(p_a);
		Vector3 b = p_xform.xform(p_b);
		Vector3 c = p_xform.xform(p_c);
		if ((b - a).cross(c - a).length_squared() < 0.00000001f) {
			return;
		}

		PackedVector3Array polygon;
		polygon.resize(3);
		polygon[0] = a;
		polygon[1] = b;
		polygon[2] = c;
		r_polygons.push_back(polygon);
	}

	void FmodGeometryInstance3D::_rebuild_if_live() {
		if (auto_rebuild && is_inside_tree()) {
			rebuild_geometry();
		}
	}

	void FmodGeometryInstance3D::set_source_mode(SourceMode p_mode) {
		if (p_mode < SOURCE_AUTO || p_mode > SOURCE_MESH_RESOURCE) {
			return;
		}
		source_mode = p_mode;
		_rebuild_if_live();
	}

	FmodGeometryInstance3D::SourceMode FmodGeometryInstance3D::get_source_mode() const {
		return source_mode;
	}

	void FmodGeometryInstance3D::set_source_node_path(const NodePath& p_path) {
		source_node_path = p_path;
		_rebuild_if_live();
	}

	NodePath FmodGeometryInstance3D::get_source_node_path() const {
		return source_node_path;
	}

	void FmodGeometryInstance3D::set_mesh(const Ref<Mesh>& p_mesh) {
		mesh = p_mesh;
		_rebuild_if_live();
	}

	Ref<Mesh> FmodGeometryInstance3D::get_mesh() const {
		return mesh;
	}

	void FmodGeometryInstance3D::set_direct_occlusion(float p_value) {
		direct_occlusion = CLAMP(p_value, 0.0f, 1.0f);
		_rebuild_if_live();
	}

	float FmodGeometryInstance3D::get_direct_occlusion() const {
		return direct_occlusion;
	}

	void FmodGeometryInstance3D::set_reverb_occlusion(float p_value) {
		reverb_occlusion = CLAMP(p_value, 0.0f, 1.0f);
		_rebuild_if_live();
	}

	float FmodGeometryInstance3D::get_reverb_occlusion() const {
		return reverb_occlusion;
	}

	void FmodGeometryInstance3D::set_double_sided(bool p_value) {
		double_sided = p_value;
		_rebuild_if_live();
	}

	bool FmodGeometryInstance3D::get_double_sided() const {
		return double_sided;
	}

	void FmodGeometryInstance3D::set_active(bool p_value) {
		active = p_value;
		if (has_valid_geometry()) {
			geometry->set_active(active);
		}
	}

	bool FmodGeometryInstance3D::get_active() const {
		return active;
	}

	void FmodGeometryInstance3D::set_auto_rebuild(bool p_value) {
		auto_rebuild = p_value;
	}

	bool FmodGeometryInstance3D::get_auto_rebuild() const {
		return auto_rebuild;
	}

	void FmodGeometryInstance3D::set_sync_transform(bool p_value) {
		sync_transform = p_value;
		set_process(sync_transform);
		if (sync_transform && is_inside_tree()) {
			sync_geometry_transform();
		}
	}

	bool FmodGeometryInstance3D::get_sync_transform() const {
		return sync_transform;
	}

	void FmodGeometryInstance3D::set_recursive_source_scan(bool p_value) {
		recursive_source_scan = p_value;
		_rebuild_if_live();
	}

	bool FmodGeometryInstance3D::get_recursive_source_scan() const {
		return recursive_source_scan;
	}

	void FmodGeometryInstance3D::set_primitive_segments(int p_segments) {
		primitive_segments = CLAMP(p_segments, 6, 64);
		_rebuild_if_live();
	}

	int FmodGeometryInstance3D::get_primitive_segments() const {
		return primitive_segments;
	}

	void FmodGeometryInstance3D::set_show_debug_gizmo(bool p_value) {
		show_debug_gizmo = p_value;
#ifdef TOOLS_ENABLED
		update_gizmos();
#endif
	}

	bool FmodGeometryInstance3D::get_show_debug_gizmo() const {
		return show_debug_gizmo;
	}

	int FmodGeometryInstance3D::get_polygon_count() const {
		return polygon_count;
	}

	int FmodGeometryInstance3D::get_vertex_count() const {
		return vertex_count;
	}

	Ref<FmodGeometry> FmodGeometryInstance3D::get_geometry() const {
		return geometry;
	}
}
