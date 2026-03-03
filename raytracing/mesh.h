#pragma once
#include <vector>
#include <map>
#include <string>
#include <cstdio>
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"
#include "point2D.h"
#include "vec3.h"
#include "aabb.h"
#include "ray.h"
#include "hittable.h"
#include "interval.h"
#include "material.h"
#include "texture.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define TINYBVH_IMPLEMENTATION
#include "tiny_bvh.h"

using namespace std;

class mesh : public hittable {
public:
	vector<point3> vertices;              
	vector<vec3> normals;                
	vector<point2> textures;              
	vector<vector<int>> vertex_faces;     
	vector<int> face_material_ids;        

	int num_vertices;
	int num_normals;
	int num_textures;
	int num_shapes;
	int num_faces;
	aabb aabb_mesh;

	bool areNormals = false;

	vector<material*> materials;          
	material* default_material;           

	bool UsaBVH = true;
	tinybvh::bvhvec4* triangles;
	tinybvh::BVH bvh;

	
	mesh(const char* filename, const char* basepath = NULL, bool triangulate = true) {
		default_material = new material();
		default_material->texture = new constant_texture(color(0.8f, 0.8f, 0.8f));
		default_material->ka = color(0.3f, 0.3f, 0.3f);
		default_material->ks = color(0.5f, 0.5f, 0.5f);
		default_material->alpha = 32.0f;

		triangles = nullptr;

		if (!load_mesh(filename, basepath, triangulate)) {
			throw std::runtime_error("Error: mesh loading failed.");
		}
	}

	~mesh() {
		if (UsaBVH && triangles) delete[] triangles;


		for (auto mat : materials) {
			if (mat && mat->texture) delete mat->texture;
			if (mat) delete mat;
		}

		if (default_material) {
			if (default_material->texture) delete default_material->texture;
			delete default_material;
		}
	}

	bool load_mesh(const char* filename, const char* basepath, bool triangulate);
	void load_materials(const vector<tinyobj::material_t>& obj_materials, const char* basepath);

	virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const;
	virtual bool hit_shadow(const ray& r, interval ray_t) const;
	virtual bool bounding_box(aabb& box) const;
};

bool mesh::bounding_box(aabb& box) const {
	box = aabb_mesh;
	return true;
}


void mesh::load_materials(const vector<tinyobj::material_t>& obj_materials, const char* basepath) {
	materials.clear();

	std::cout << "[Mesh] Loading materials: " << obj_materials.size() << std::endl;
	std::cout << "[Mesh] Base texture path: " << (basepath ? basepath : "NONE") << std::endl;

	for (size_t i = 0; i < obj_materials.size(); i++) {
		const tinyobj::material_t& mat = obj_materials[i];
		material* new_mat = new material();

		std::cout << "\n--- Material [" << i << "]: " << mat.name << " ---" << std::endl;
		std::cout << "  Illumination mode: " << mat.illum << std::endl;

		if (!mat.diffuse_texname.empty()) {
			string texture_path = string(basepath) + mat.diffuse_texname;
			std::cout << "  Attempting to load diffuse texture: " << texture_path << std::endl;

			SDL_Surface* test_surface = IMG_Load(texture_path.c_str());

			if (test_surface != nullptr) {
				std::cout << "  -> Success: Texture loaded." << std::endl;
				SDL_FreeSurface(test_surface);

				new_mat->texture = new image_texture(texture_path.c_str());
				std::cout << "  -> image_texture instance created." << std::endl;
			}
			else {
				std::cerr << "  -> ERROR: Failed to load texture." << std::endl;
				std::cerr << "     SDL_image Error: " << IMG_GetError() << std::endl;

				color diffuse_color(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
				std::cout << "  -> Falling back to diffuse color: RGB("
					<< mat.diffuse[0] << ", " << mat.diffuse[1] << ", " << mat.diffuse[2] << ")" << std::endl;
				new_mat->texture = new constant_texture(diffuse_color);
			}
		}
		else {
			std::cout << "  No texture found. Using constant color." << std::endl;
			color diffuse_color(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
			new_mat->texture = new constant_texture(diffuse_color);
		}

		// Set material properties
		new_mat->ka = color(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
		new_mat->kd = color(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
		new_mat->ks = color(mat.specular[0], mat.specular[1], mat.specular[2]);
		new_mat->alpha = (mat.shininess > 0) ? mat.shininess : 32.0f;

		std::cout << "  Material Properties:" << std::endl;
		std::cout << "    - Ka (Ambient):  RGB(" << mat.ambient[0] << ", " << mat.ambient[1] << ", " << mat.ambient[2] << ")" << std::endl;
		std::cout << "    - Kd (Diffuse):  RGB(" << mat.diffuse[0] << ", " << mat.diffuse[1] << ", " << mat.diffuse[2] << ")" << std::endl;
		std::cout << "    - Ks (Specular): RGB(" << mat.specular[0] << ", " << mat.specular[1] << ", " << mat.specular[2] << ")" << std::endl;
		std::cout << "    - Shininess: " << mat.shininess << " (Alpha set to: " << new_mat->alpha << ")" << std::endl;

		if (mat.illum >= 3) {
			new_mat->reflective = 0.3f;
			std::cout << "    - Reflection: ENABLED (illum >= 3, value = 0.3)" << std::endl;
		}
		else {
			new_mat->reflective = 0.0f;
			std::cout << "    - Reflection: DISABLED" << std::endl;
		}

		materials.push_back(new_mat);
	}

	std::cout << "\n========================================" << std::endl;
	std::cout << "     MATERIAL LOADING COMPLETED" << std::endl;
	std::cout << "     Total materials: " << materials.size() << std::endl;
	std::cout << "========================================\n" << std::endl;
}


bool mesh::load_mesh(const char* filename, const char* basepath, bool triangulate) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> obj_materials;
	std::string err;
	std::string war;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &obj_materials, &war, &err, filename, basepath, triangulate);

	if (!ret) {
		std::cerr << "[Error] Failed to load OBJ file: " << filename << std::endl;
		return false;
	}

	std::cout << "[Success] OBJ file loaded correctly." << std::endl;
	std::cout << "  - Materials found: " << obj_materials.size() << std::endl;

	if (obj_materials.empty()) {
		std::cerr << "[Warning] No materials found in the OBJ file!" << std::endl;
		std::cerr << "  - Check if the .mtl file exists and is in the correct directory." << std::endl;
	}

	if (!obj_materials.empty()) {
		load_materials(obj_materials, basepath);
	}

	num_vertices = (attrib.vertices.size() / 3);
	num_normals = (attrib.normals.size() / 3);
	num_textures = (attrib.texcoords.size() / 2);
	num_shapes = shapes.size();

	aabb_mesh.x.min = aabb_mesh.y.min = aabb_mesh.z.min = FLT_MAX;
	aabb_mesh.x.max = aabb_mesh.y.max = aabb_mesh.z.max = -FLT_MAX;

	for (int v = 0; v < attrib.vertices.size() / 3; v++) {
		point3 p = point3(attrib.vertices[3 * v + 0], attrib.vertices[3 * v + 1], attrib.vertices[3 * v + 2]);
		aabb_mesh.x.min = ffmin(p[0], aabb_mesh.x.min);
		aabb_mesh.y.min = ffmin(p[1], aabb_mesh.y.min);
		aabb_mesh.z.min = ffmin(p[2], aabb_mesh.z.min);
		aabb_mesh.x.max = ffmax(p[0], aabb_mesh.x.max);
		aabb_mesh.y.max = ffmax(p[1], aabb_mesh.y.max);
		aabb_mesh.z.max = ffmax(p[2], aabb_mesh.z.max);
		vertices.push_back(p);
	}

	if (attrib.normals.size() > 0) {
		areNormals = true;
		for (int v = 0; v < attrib.normals.size() / 3; v++) {
			normals.push_back(vec3(attrib.normals[3 * v + 0], attrib.normals[3 * v + 1], attrib.normals[3 * v + 2]));
		}
	}

	if (attrib.texcoords.size() > 0) {
		for (int v = 0; v < attrib.texcoords.size() / 2; v++) {
			textures.push_back(point2(attrib.texcoords[2 * v + 0], attrib.texcoords[2 * v + 1]));
		}
	}

	vertex_faces.resize(3);
	face_material_ids.clear();
	num_faces = 0;

	for (int s = 0; s < shapes.size(); s++) {
		int index_offset = 0;
		for (int f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			int material_id = shapes[s].mesh.material_ids[f];

			for (int v = 1; v < fv - 1; v++) {
				num_faces++;
				tinyobj::index_t idx0 = shapes[s].mesh.indices[index_offset + 0];
				tinyobj::index_t idx1 = shapes[s].mesh.indices[index_offset + v];
				tinyobj::index_t idx2 = shapes[s].mesh.indices[index_offset + v + 1];

				vertex_faces[0].push_back(idx0.vertex_index);
				vertex_faces[0].push_back(idx1.vertex_index);
				vertex_faces[0].push_back(idx2.vertex_index);

				if (areNormals && idx0.normal_index >= 0) {
					vertex_faces[1].push_back(idx0.normal_index);
					vertex_faces[1].push_back(idx1.normal_index);
					vertex_faces[1].push_back(idx2.normal_index);
				}
				else {
					vertex_faces[1].push_back(-1); vertex_faces[1].push_back(-1); vertex_faces[1].push_back(-1);
				}

				if (idx0.texcoord_index >= 0) {
					vertex_faces[2].push_back(idx0.texcoord_index);
					vertex_faces[2].push_back(idx1.texcoord_index);
					vertex_faces[2].push_back(idx2.texcoord_index);
				}
				else {
					vertex_faces[2].push_back(-1); vertex_faces[2].push_back(-1); vertex_faces[2].push_back(-1);
				}
				face_material_ids.push_back(material_id);
			}
			index_offset += fv;
		}
	}

	if (!areNormals) {
		normals.resize(num_vertices, vec3(0, 0, 0));
		for (int i = 0; i < num_faces; i++) {
			int i0 = vertex_faces[0][3 * i + 0];
			int i1 = vertex_faces[0][3 * i + 1];
			int i2 = vertex_faces[0][3 * i + 2];
			vec3 face_normal = cross(vertices[i1] - vertices[i0], vertices[i2] - vertices[i0]);
			normals[i0] += face_normal; normals[i1] += face_normal; normals[i2] += face_normal;
		}
		for (int i = 0; i < num_vertices; i++) normals[i] = unit_vector(normals[i]);
		areNormals = true;
	}

	if (UsaBVH) {
		triangles = new tinybvh::bvhvec4[num_faces * 3];
		for (int i = 0; i < num_faces; i++) {
			int i0 = vertex_faces[0][3 * i + 0];
			int i1 = vertex_faces[0][3 * i + 1];
			int i2 = vertex_faces[0][3 * i + 2];
			triangles[3 * i + 0] = { vertices[i0].x(), vertices[i0].y(), vertices[i0].z(), 0 };
			triangles[3 * i + 1] = { vertices[i1].x(), vertices[i1].y(), vertices[i1].z(), 0 };
			triangles[3 * i + 2] = { vertices[i2].x(), vertices[i2].y(), vertices[i2].z(), 0 };
		}
		bvh.Build(triangles, num_faces);
		bvh.Convert(tinybvh::BVH::WALD_32BYTE, tinybvh::BVH::BASIC_BVH4);
		bvh.Convert(tinybvh::BVH::BASIC_BVH4, tinybvh::BVH::BVH4_GPU);
		bvh.Refit();
	}
	return true;
}

bool triangle_intersection(const ray& r, float tmin, float tmax, hit_record& rec,
	const point3& v0, const point3& v1, const point3& v2, float& u, float& v) {
	const float EPSILON = 0.0000001f;
	vec3 edge1 = v1 - v0;
	vec3 edge2 = v2 - v0;
	vec3 h = cross(r.d, edge2);
	float a = dot(edge1, h);

	if (a > -EPSILON && a < EPSILON) return false;

	float f = 1.0f / a;
	vec3 s = r.o - v0;
	u = f * dot(s, h);
	if (u < 0.0f || u > 1.0f) return false;

	vec3 q = cross(s, edge1);
	v = f * dot(r.d, q);
	if (v < 0.0f || u + v > 1.0f) return false;

	float t = f * dot(edge2, q);
	if (t > tmin && t < tmax && t > EPSILON) {
		rec.normal = unit_vector(cross(edge1, edge2));
		rec.t = t;
		rec.p = r.at(rec.t);
		return true;
	}
	return false;
}

bool mesh::hit(const ray& ray, interval ray_t, hit_record& rec) const {
	if (!aabb_mesh.hit(ray, ray_t)) return false;

	if (UsaBVH) {
		float lungDir = ray.d.length();
		point3 dir = ray.d / lungDir;
		tinybvh::bvhvec3 O(ray.o[0], ray.o[1], ray.o[2]);
		tinybvh::bvhvec3 D(dir[0], dir[1], dir[2]);
		tinybvh::Ray r(O, D);

		bvh.BatchIntersect(&r, 1, tinybvh::BVH::BVH4_GPU, tinybvh::BVH::USE_GPU);
		int primIdx = r.hit.prim;

		if (r.hit.t > 1e30f) return false; 

		float t = r.hit.t / lungDir;
		if (t <= ray_t.min || t >= ray_t.max) return false;

		tinybvh::bvhvec4 vet0 = triangles[3 * primIdx + 0];
		tinybvh::bvhvec4 vet1 = triangles[3 * primIdx + 1];
		tinybvh::bvhvec4 vet2 = triangles[3 * primIdx + 2];
		point3 v0(vet0.x, vet0.y, vet0.z);
		point3 v1(vet1.x, vet1.y, vet1.z);
		point3 v2(vet2.x, vet2.y, vet2.z);

		vec3 bary(1.0f - r.hit.u - r.hit.v, r.hit.u, r.hit.v);

		vec3 outward_normal;
		if (areNormals) {
			int i0 = vertex_faces[1][3 * primIdx + 0];
			int i1 = vertex_faces[1][3 * primIdx + 1];
			int i2 = vertex_faces[1][3 * primIdx + 2];
			if (i0 >= 0) {
				outward_normal = unit_vector(normals[i0] * bary[0] + normals[i1] * bary[1] + normals[i2] * bary[2]);
			}
			else {
				outward_normal = unit_vector(cross(v1 - v0, v2 - v0));
			}
		}
		else {
			outward_normal = unit_vector(cross(v1 - v0, v2 - v0));
		}

		rec.t = t;
		rec.p = ray.at(rec.t);

		rec.set_face_normal(ray, outward_normal);

		int i0 = vertex_faces[2][3 * primIdx + 0];
		int i1 = vertex_faces[2][3 * primIdx + 1];
		int i2 = vertex_faces[2][3 * primIdx + 2];
		if (i0 >= 0 && i0 < textures.size()) {
			point2 uv = textures[i0] * bary[0] + textures[i1] * bary[1] + textures[i2] * bary[2];
			rec.u = uv.x; rec.v = uv.y;
		}
		else {
			rec.u = 0.0f; rec.v = 0.0f;
		}

		int mat_id = face_material_ids[primIdx];
		rec.m = (mat_id >= 0 && mat_id < materials.size()) ? materials[mat_id] : default_material;
		return true;
	}

	bool hit_anything = false;
	hit_record temp_rec;
	float closest_so_far = ray_t.max;
	float u, v;

	for (int i = 0; i < num_faces; i++) {
		int i0 = vertex_faces[0][3 * i + 0];
		int i1 = vertex_faces[0][3 * i + 1];
		int i2 = vertex_faces[0][3 * i + 2];

		if (triangle_intersection(ray, ray_t.min, closest_so_far, temp_rec, vertices[i0], vertices[i1], vertices[i2], u, v)) {
			hit_anything = true;
			closest_so_far = temp_rec.t;
			rec = temp_rec;

			vec3 bary(1.0f - u - v, u, v);
			vec3 outward_normal;

			if (areNormals) {
				int n0 = vertex_faces[1][3 * i + 0];
				int n1 = vertex_faces[1][3 * i + 1];
				int n2 = vertex_faces[1][3 * i + 2];
				if (n0 >= 0) {
					outward_normal = unit_vector(normals[n0] * bary[0] + normals[n1] * bary[1] + normals[n2] * bary[2]);
				}
				else {
					outward_normal = temp_rec.normal;
				}
			}
			else {
				outward_normal = temp_rec.normal;
			}

			rec.set_face_normal(ray, outward_normal);

			int t0 = vertex_faces[2][3 * i + 0];
			int t1 = vertex_faces[2][3 * i + 1];
			int t2 = vertex_faces[2][3 * i + 2];
			if (t0 >= 0 && t0 < textures.size()) {
				point2 uv = textures[t0] * bary[0] + textures[t1] * bary[1] + textures[t2] * bary[2];
				rec.u = uv.x; rec.v = uv.y;
			}

			int mat_id = face_material_ids[i];
			rec.m = (mat_id >= 0 && mat_id < materials.size()) ? materials[mat_id] : default_material;
		}
	}
	return hit_anything;
}


bool mesh::hit_shadow(const ray& ray, interval ray_t) const {
	if (!aabb_mesh.hit(ray, ray_t)) return false;

	if (UsaBVH) {
		float lungDir = ray.d.length();
		point3 dir = ray.d / lungDir;
		tinybvh::bvhvec3 O(ray.o[0], ray.o[1], ray.o[2]);
		tinybvh::bvhvec3 D(dir[0], dir[1], dir[2]);
		tinybvh::Ray r(O, D);
		bvh.BatchIntersect(&r, 1, tinybvh::BVH::BVH4_GPU, tinybvh::BVH::USE_GPU);
		if (r.hit.t > 1e30f) return false;
		float t = r.hit.t / lungDir;
		if (t <= ray_t.min || t >= ray_t.max) return false;


		int primIdx = r.hit.prim;
		int i0 = vertex_faces[0][3 * primIdx + 0];
		int i1 = vertex_faces[0][3 * primIdx + 1];
		int i2 = vertex_faces[0][3 * primIdx + 2];
		vec3 face_normal = unit_vector(cross(
			vertices[i1] - vertices[i0],
			vertices[i2] - vertices[i0]
		));

		if (dot(face_normal, ray.d) > 0) return false;

		return true;
	}

	hit_record temp_rec;
	float u, v;
	for (int i = 0; i < num_faces; i++) {
		int i0 = vertex_faces[0][3 * i + 0];
		int i1 = vertex_faces[0][3 * i + 1];
		int i2 = vertex_faces[0][3 * i + 2];

		vec3 face_normal = unit_vector(cross(
			vertices[i1] - vertices[i0],
			vertices[i2] - vertices[i0]
		));

		if (dot(face_normal, ray.d) > 0) continue;

		if (triangle_intersection(ray, ray_t.min, ray_t.max, temp_rec,
			vertices[i0], vertices[i1], vertices[i2], u, v))
			return true;
	}
	return false;
}

