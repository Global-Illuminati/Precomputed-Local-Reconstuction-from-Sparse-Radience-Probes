
//#define LIVING_ROOM
//#define T_SCENE
#ifdef T_SCENE
#define RHO_PROBES 7.0f //15.0f //7.0f//15.0f
#define PRECOMP_ASSET_FOLDER "../../assets/t_scene/precompute/"
#define OBJ_FILE_PATH "../../assets/t_scene/t_scene.obj"
#elif defined LIVING_ROOM
#define RHO_PROBES 15.0f
#define PRECOMP_ASSET_FOLDER "../../assets/living_room/precompute/"
#define OBJ_FILE_PATH "../../assets/living_room/living_room.obj"
#else
#define RHO_PROBES 15.0f
#define PRECOMP_ASSET_FOLDER "../../assets/sponza/precompute/"
#define OBJ_FILE_PATH "../../assets/sponza/sponza.obj"
#endif


#pragma warning(disable:4996)
#include "stdafx.h"
#include "string.h"

#include "thekla_atlas.h"
using namespace Thekla;

#include <stdio.h>
#include <assert.h>
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"
#include "stdint.h"
#include "math.h"

//#define eigen_assert(x) do{if(!(x)){printf(#x);__debugbreak();}}while(0)
#include "Eigen\Dense"
#include "Eigen\Sparse"
typedef Eigen::Vector2f vec2;
typedef Eigen::Vector3f vec3;
typedef Eigen::Vector2i ivec2;
typedef Eigen::Vector3i ivec3;
typedef Eigen::Matrix<int16_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXi16;

const char *get_file_data(size_t *data_len, const char *file_path) {
	FILE *file = fopen(file_path, "rb");
	if (!file) return NULL;

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);
	char *data = (char *)malloc(file_size + 1);
	data[file_size] = '\0';
	if (data) {
		fread(data, 1, file_size, file);
		*data_len = file_size;
	}
	fclose(file);
	return data;
}

int min(int a, int b) {
	return a < b ? a : b;
}
int max(int a, int b) {
	return a > b ? a : b;
}

float min(float a, float b) {
	return a < b ? a : b;
}
float max(float a, float b) {
	return a > b ? a : b;
}

#include "voxelizer.hpp"
#include "probe_reducer.hpp"
#include "ray_tracer.hpp"
#include "relight_rays.hpp"

// simple write rutine for OBJs, supports the baaare minimum.
// and of course two uvs ;)

//also fucks up if on vert has multiple uvs... but that shouldn't be super common anyway, albeit obviously allowed in the format..
void write_obj(tinyobj_shape_t *shapes, size_t num_shapes, Mesh *mesh, const char *output_file_name) {
	FILE *f = fopen(output_file_name, "w");
	fprintf(f, "# OBJ (ish) File with two set of uvs, vt & vt2\n");
	fprintf(f, "# Regenerated as part of precomputation chain in Global Illuminati\n");
	fprintf(f, "# Yes it's horrible to create a slight modification to an existing file format\n");
	fprintf(f, "# I should fix this up at some point and export using a better format that actually supports multiple uvs...\n");
	fprintf(f, "# // Daniel\n");

	for (int i = 0; i < mesh->num_verts; i++) {
		vec3 v = mesh->verts[i];
		fprintf(f, "v %f %f %f\n", v.x(), v.y(), v.z());
	}
#if 0
	// we don't support uv2s and normals yet anyway... need to update obj loader
	for (int i = 0; i < mesh->num_verts; i++) {
		vec3 n = mesh->normals[i];
		fprintf(f, "vn %f %f %f\n", n.x(), n.y(), n.z());
	}
#endif
	for (int i = 0; i < mesh->num_verts; i++) {
		vec2 uv = mesh->uv[i];
		fprintf(f, "vt %f %f\n", uv.x(), uv.y());
	}

	for (int i = 0; i < mesh->num_verts; i++) {
		vec2 luv = mesh->lightmap_uv[i];
		fprintf(f, "vt2 %f %f\n", luv.x(), luv.y());
	}

	int shape_idx = -1;

	for (int face_idx = 0; face_idx < mesh->num_indices / 3; face_idx++) {
		
		int ia = mesh->indices[face_idx*3 + 0];
		int ib = mesh->indices[face_idx*3 + 1];
		int ic = mesh->indices[face_idx*3 + 2];

		if (mesh->shape_idx[ia] != shape_idx) { // new shape need to output the name and material
			shape_idx = mesh->shape_idx[ia];
			tinyobj_shape_t shape = shapes[shape_idx];
			fprintf(f, "o %s\n", shape.name);
			fprintf(f, "usemtl %s\n", shape.material_name);
			fprintf(f, "s off\n"); // probably not needed.
		}

		fprintf(f, "f %d/%d//%d %d/%d//%d %d/%d//%d\n",
			ia + 1, ia + 1, ia + 1,
			ib + 1, ib + 1, ib + 1,
			ic + 1, ic + 1, ic + 1);
	}
	fclose(f);
}

iAABB2 transform_to_pixel_space(AABB2 bounding_box, Atlas_Output_Mesh *mesh) {
	iAABB2 ret;
	ivec2 atlas_size = ivec2(mesh->atlas_width, mesh->atlas_height);
	ivec2 fl = floor2(bounding_box.min);
	ivec2 cl = ceil2(bounding_box.max);
	ret.min = fl.cwiseMax(0);
	ret.max = cl.cwiseMin(atlas_size);

	return ret;
}


vec2 get_pixel_center(ivec2 pixel) {
	return vec2(pixel.x() + 0.5, pixel.y() + 0.5);
}

vec3 compute_barycentric_coords(vec2 p, Triangle2 &tri) {
	vec2 v0 = tri.b - tri.a;
	vec2 v1 = tri.c - tri.a;
	vec2 v2 = p - tri.a;
	float inv_denom = 1.0f / (v0.x() * v1.y() - v1.x() * v0.y());
	float v = (v2.x() * v1.y() - v1.x() * v2.y()) * inv_denom;
	float w = (v0.x() * v2.y() - v2.x() * v0.y()) * inv_denom;
	float u = 1.0f - v - w;
	return vec3(u, v, w);
}

struct Receiver {
	vec3 pos;
	vec3 norm;
	ivec2 px;
};

vec3 apply_baryc(vec3 b, Triangle &t) {
	return t.a *b.x() + t.b * b.y() + t.c * b.z();
}

#pragma optimize("", off)
vec3 project_onto_triangle(vec3 bc, Triangle &tri) {

	int num_positives = 0;
	int indices_of_positives[3];
	for (int i = 0; i < 3; i++) {
		if (bc(i) > 0) {
			indices_of_positives[num_positives++] = i;
		}
	}

	vec3 ret = vec3(0, 0, 0);
	if (num_positives == 1) {
		//point tri[non_zeros[0]] is closest
		ret(indices_of_positives[0]) = 1.0;
		return ret;
	} else if (num_positives == 2) {
		// edge between tri[ia] tri[ib] is closest  
		int ia = indices_of_positives[0];
		int ib = indices_of_positives[1];
		vec3 segment = (tri.points[ib] - tri.points[ia]);

		bc(ia) -= 1.0; // modify bary centric coordinates to be for p-tri[ia]

		vec3 pa = apply_baryc(bc, tri);
		float t = segment.dot(pa) / segment.squaredNorm();
		ret(ia) = 1.0f - t;
		ret(ib) = t;
		return ret;
	}

	//inside tri or invalid bccs
	return bc;
}

ivec3 round_to_ivec3(vec3 p) {
	return (p + vec3(0.5, 0.5, 0.5)).cast<int>();
}


#include <set>;

void compute_receiver_locations(Atlas_Output_Mesh *light_map_mesh, Mesh mesh, std::vector<Receiver> &receivers, VoxelScene *voxel_scene) {

	printf("computing receiver locations\n");
	static uint8_t pixel_is_processed[1024][1024];

#if 0
	std::set<Receiver, std::function<bool(Receiver, Receiver)>>
		theset(std::function<bool(Receiver, Receiver) >>
			vec3 da = probe_locations[ia] - receiver.pos;
	vec3 db = probe_locations[ib] - receiver.pos;
	return (da.dot(da) < db.dot(db));
});

#endif
	// @Performance Runtime:
	//  Investigate if changing this to
	//  instead compare the morton code 
	//  causes any significant runtime performance difference,
	//  it will cause the receivers to be more spatially coherent.
	//  might have a slight benefit.

	auto cmp = [](Receiver a, Receiver b) {
		return
			(a.px.x() < b.px.x()) ||
			((a.px.x() == b.px.x()) && a.px.y() < b.px.y());
	};
	std::set<Receiver, decltype(cmp)> receiver_set(cmp);


	for (int face_idx = 0; face_idx < light_map_mesh->index_count / 3; face_idx++) {
		auto new_a_idx = light_map_mesh->index_array[face_idx * 3 + 0];
		auto new_b_idx = light_map_mesh->index_array[face_idx * 3 + 1];
		auto new_c_idx = light_map_mesh->index_array[face_idx * 3 + 2];

		int xref_a = light_map_mesh->vertex_array[new_a_idx].xref;
		int xref_b = light_map_mesh->vertex_array[new_b_idx].xref;
		int xref_c = light_map_mesh->vertex_array[new_c_idx].xref;

		Triangle vert_tri = { mesh.verts[xref_a], mesh.verts[xref_b], mesh.verts[xref_c] };
		Triangle norm_tri = { mesh.normals[xref_a], mesh.normals[xref_b], mesh.normals[xref_c] };

		vec2 uv_a = Eigen::Map<vec2>(light_map_mesh->vertex_array[new_a_idx].uv);
		vec2 uv_b = Eigen::Map<vec2>(light_map_mesh->vertex_array[new_b_idx].uv);
		vec2 uv_c = Eigen::Map<vec2>(light_map_mesh->vertex_array[new_c_idx].uv);

		Triangle2 uv_tri = { uv_a,uv_b,uv_c };

		iAABB2 pixel_bounds = transform_to_pixel_space(aabb_from_triangle(uv_tri), light_map_mesh);
		ivec2 min = pixel_bounds.min;
		ivec2 max = pixel_bounds.max;

		for (int x = min.x(); x < max.x(); x++) for (int y = min.y(); y < max.y(); y++) {
			if (pixel_is_processed[x][y])continue;
			ivec2 pixel = ivec2(x, y);

			vec2 px_center = get_pixel_center(pixel);
			vec3 box_center = vec3(px_center.x(), px_center.y(), 0.0f);
			vec3 half_size = vec3(1.0f, 1.0f, 1.0f);
			Triangle tri = {
				vec3(uv_tri.a.x(),uv_tri.a.y(),0.0f),
				vec3(uv_tri.b.x(),uv_tri.b.y(),0.0f),
				vec3(uv_tri.c.x(),uv_tri.c.y(),0.0f)
			};



			// @Performance 
			// we call a 3D aabb-triangle intersection routine
			// although we only need the 2D case
			// not performance critial though so meh.
			bool triangle_inside_px_influence = TriBoxOverlap(box_center.data(), half_size.data(), (float(*)[3])&tri);
			vec3 baryc = compute_barycentric_coords(get_pixel_center(pixel), uv_tri);
			bool px_center_inside_tri = (baryc.x() > 0 && baryc.y() > 0 && baryc.z() > 0);

			//if (triangle_inside_px_influence) {
			if (px_center_inside_tri) {
				pixel_is_processed[x][y] = px_center_inside_tri;

				// will get bad interpolations...
				// but so will most other things as well...

				//baryc = project_onto_triangle(baryc, vert_tri);


				// ugly hack scaling
				//vec3 bc = vec3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
				//baryc = bc + (baryc - bc)*0.90;



				vec3 pos = apply_baryc(baryc, vert_tri);
#if 0
				// haaaaaacky way to try to offset the receivers so that they're not
				// inside an object. or very close to inside of an object. 
				 
				vec3 unit = vec3(1, 1, 1);
				ivec3 voxel_pos = round_to_ivec3(transform_to_voxelspace(pos, voxel_scene));
				ivec3 min = voxel_pos + ivec3(2, 2, 2);
				ivec3 max = voxel_pos - ivec3(2, 2, 2);

				float min_dist_sq = FLT_MAX;
				bool is_set = false;
				for (int x = min.x(); x <= max.x(); x++) 
				for (int y = min.y(); y <= max.y(); y++) 
				for (int z = min.z(); z <= max.z(); z++) {
					if (voxel_is_empty(ivec3(x,y,z),voxel_scene)) {
						vec3 vx_pos = get_voxel_center(ivec3(x, y, z), voxel_scene);
						float dist_sq = (vx_pos - pos).squaredNorm();
						if (dist_sq < min_dist_sq) {
							min_dist_sq = dist_sq;
							pos = vx_pos;
						}
						is_set = true;
					}
				}
#endif

				vec3 norm = apply_baryc(baryc, norm_tri);
				norm.normalize();
				receiver_set.insert({ pos, norm, pixel });
			}
		}
	}

	int padding_extent = 1;
	// --- Add padding receivers by creating duplicates of adjacent receivers to empty cells ---
	for (int i = 0; i < padding_extent; i++) {
		int padding_duplicates = 0;
		for (int x = 0; x < 1024; x++) for (int y = 0; y < 1024; y++) {
			ivec2 pixel = ivec2(x, y);
			if (!pixel_is_processed[x][y]) {
				// left, top, right, bottom neighbors
				if (x > 0 && pixel_is_processed[x - 1][y]) {
					ivec2 neighbor_px = ivec2(x - 1, y);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (y > 0 && pixel_is_processed[x][y - 1]) {
					ivec2 neighbor_px = ivec2(x, y - 1);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (x <= 1024 && pixel_is_processed[x + 1][y]) {
					ivec2 neighbor_px = ivec2(x + 1, y);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (y <= 1024 && pixel_is_processed[x][y + 1]) {
					ivec2 neighbor_px = ivec2(x, y + 1);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (x > 0 && y > 0 && pixel_is_processed[x - 1][y - 1]) { // diagonal neighbors
					ivec2 neighbor_px = ivec2(x - 1, y - 1);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (x <= 1024 && y > 0 && pixel_is_processed[x + 1][y - 1]) {
					ivec2 neighbor_px = ivec2(x + 1, y - 1);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (x <= 1024 && y <= 1024 && pixel_is_processed[x + 1][y + 1]) {
					ivec2 neighbor_px = ivec2(x + 1, y + 1);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
				else if (x > 0 && y <= 1024 && pixel_is_processed[x - 1][y + 1]) {
					ivec2 neighbor_px = ivec2(x - 1, y + 1);
					Receiver neighbor = *(receiver_set.find({ vec3(0,0,0), vec3(0,0,0), neighbor_px }));
					receiver_set.insert({ neighbor.pos, neighbor.norm, pixel });
					padding_duplicates++;
				}
			}
		}
		for (Receiver r : receiver_set) {
			pixel_is_processed[r.px[0]][r.px[1]] = true;
		}
		printf("Added %d receiver duplicates for padding.\n", padding_duplicates);
	}
	// -------------------

	std::copy(receiver_set.begin(), receiver_set.end(), std::back_inserter(receivers));

	printf("got %d receivers\n", receivers.size());
}

#pragma optimize("", on)

void generate_normals(Mesh *mesh) {

	mesh->normals = (vec3 *)calloc(mesh->num_verts, sizeof(vec3));

	for (int i = 0; i < mesh->num_indices / 3; i++) {
		int ia = mesh->indices[i * 3 + 0];
		int ib = mesh->indices[i * 3 + 1];
		int ic = mesh->indices[i * 3 + 2];

		vec3 a = mesh->verts[ia];
		vec3 b = mesh->verts[ib];
		vec3 c = mesh->verts[ic];

		vec3 n = (b - a).cross(c - a);

		mesh->normals[ia] += n;
		mesh->normals[ib] += n;
		mesh->normals[ic] += n;
	}

	for (int i = 0; i < mesh->num_verts; i++) {
		mesh->normals[i].normalize();
	}

}


void resize_thekla_atlas(Atlas_Output_Mesh *light_map_mesh, float rescaleFactor) {
	for (int i = 0; i < light_map_mesh->vertex_count; i++) {
		light_map_mesh->vertex_array[i].uv[0] *= rescaleFactor;
		light_map_mesh->vertex_array[i].uv[1] *= rescaleFactor;
	}
}



// @NOTE: tinyobj loader is modified to avoid reading mtl file 
// because: hashtable implementation in the file was shit, ie. not at all working, (you can't link list quadratic probing dude...),
// and I didn't want to spend time fixing it 
// but in doing so we removed ability for object to have multiple materials
// this might be something that we want to fix eventually, but for now I just want to make it work 
// so just put the mtl on the shape, which should be fine as long as we don't have multple materials for a shape 
// which our blender exporter or three.js doesn't seem to support out of the box anyway. so mehhh...
// @NOTE might consider changing back to the c++ version and modify it so that it's not soooooo sloooow cause c version seems a bit unstable...
// or fix c-version cause I like the structure better... might be fine now though just maybe support multiple materials... maybe...
// Daniel, 11 Feb 2018 






#include "visibility.hpp"
#include "smooth_dictionary_learning.hpp"

Eigen::MatrixXf load_matrix(char *file) {
	FILE *f = fopen(file, "rb");
	int cols, rows;

	fread(&cols, sizeof(int), 1, f);
	fread(&rows, sizeof(int), 1, f);

	float *data = (float *)malloc(cols*rows * sizeof(float));
	fread(data, sizeof(float), cols*rows, f);
	fclose(f);

	return Eigen::Map<Eigen::MatrixXf>(data, rows, cols);
}


MatrixXi16 load_imatrix(char *file) {
	FILE *f = fopen(file, "rb");
	int cols, rows;

	fread(&cols, sizeof(int), 1, f);
	fread(&rows, sizeof(int), 1, f);

	int16_t *data = (int16_t *)malloc(cols*rows * sizeof(int));
	fread(data, sizeof(int16_t), cols*rows, f);
	fclose(f);

	return Eigen::Map<MatrixXi16>(data, rows, cols);
}

#pragma optimize("",off)
// No longer neccessary
// produces full_nz and probe_indices from full_matrix
// atleast keep until we verify that the direct export works.

void recompute_matrix_factorization() {
	auto full_mat_nz = load_matrix(PRECOMP_ASSET_FOLDER "full_nz.matrix");
	auto probe_indices = load_imatrix(PRECOMP_ASSET_FOLDER "probe_indices.imatrix");

	int num_probes = probe_indices.maxCoeff()+1;
	int num_receivers = probe_indices.cols();
	int col_length = probe_indices.rows();
	Eigen::MatrixXf full(num_probes * 16, num_receivers);
	full.setZero();

	int rec_index=0;
	for (int i = 0; i < num_receivers; i++) {
		bool non_zero = false;
		for (int c = 0; c < col_length; c++) {
			int16_t probe = probe_indices(c, i);
			if (probe == -1)continue;

			auto src = &full_mat_nz(c * 16, i);
			for (int i = 0; i < 16; i++) {
				if (src[i] != 0) {
					non_zero = true;
					break;
				}
			}
			auto dst = &full(probe * 16, rec_index);
			memmove(dst, src, sizeof(float) * 16);
		}
		if (non_zero)++rec_index;
	}

	full.conservativeResize(Eigen::NoChange_t(), rec_index);
	std::vector<Receiver> recs(rec_index);
	Eigen::SparseMatrix<float> f = full.sparseView();
	f.makeCompressed();
	smooth_dictionary_learning(&f, &recs, 0.01f, 1024);
}

#pragma optimize("",on)
#include <unordered_map>

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct Vert {
	vec3 vert;
	vec3 normal;
	vec2 uv;
	int shape;

	std::size_t hash(Vert v) const {
		size_t hash = 0;
		hash_combine<vec3>(hash, v.vert);
		hash_combine<vec3>(hash, v.normal);
		hash_combine<vec2>(hash, v.uv);
		hash_combine<int>(hash, v.shape);
		return hash;
	}

	bool operator==(const Vert &other) const {
		return (vert == other.vert
			&& normal == other.normal
			&& uv == other.uv
			&& shape == other.shape
			);
	}
};

namespace std {
	template<>
	struct hash<Vert> {
		std::size_t operator()(Vert v) const {
			size_t hash = 0;
			hash_combine<vec3>(hash, v.vert);
			hash_combine<vec3>(hash, v.normal);
			hash_combine<vec2>(hash, v.uv);
			hash_combine<int>(hash, v.shape);
			return hash;
		}
	};
	template<>
	struct hash<vec3> {
		std::size_t operator()(vec3 v) const {
			size_t hash = 0;
			hash_combine<float>(hash, v.x());
			hash_combine<float>(hash, v.y());
			hash_combine<float>(hash, v.z());
			return hash;
		}
	};
	template<>
	struct hash<vec2> {
		std::size_t operator()(vec2 v) const {
			size_t hash = 0;
			hash_combine<float>(hash, v.x());
			hash_combine<float>(hash, v.y());
			return hash;
		}
	};
}



int main(int argc, char * argv[]) {

#if 0
	recompute_matrix_factorization();
	{
		Eigen::MatrixXf m(40,20);
		m.setRandom();
		std::vector<Receiver> recs(m.cols());
		Eigen::SparseMatrix<float> f = m.sparseView();
		smooth_dictionary_learning(&f, &recs, 0.01, 60);
		int q = 0;
	}
#endif

	tinyobj_attrib_t attr;
	tinyobj_shape_t* shapes = NULL;
	size_t num_shapes;
	tinyobj_material_t* materials = NULL;
	size_t num_materials;

	{
		size_t data_len = 0;
		const char* data = get_file_data(&data_len, OBJ_FILE_PATH);
		if (data == NULL) {
			printf("Error loading obj file.\n");
			return(0);
		}

		unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
		int ret = tinyobj_parse_obj(&attr, &shapes, &num_shapes, &materials,
			&num_materials, data, data_len + 1, flags);
		if (ret != TINYOBJ_SUCCESS) {
			return 0;
		}

		printf("# of shapes_2    = %d\n", (int)num_shapes);
		free((void *)data);
	}


	Mesh m = {};
	{ // set up the mesh
		m.num_verts = attr.num_vertices;
		m.num_indices = attr.num_faces;
		m.verts = (vec3 *)attr.vertices;
		m.normals = (vec3 *)attr.normals;

		int *indices = (int *)malloc(m.num_indices * sizeof(int));
		for (int i = 0; i < m.num_indices; i++) {
			indices[i] = attr.faces[i].v_idx;
		}
		m.indices = indices;
		if(attr.num_normals == 0) generate_normals(&m);
		
		free(m.indices);


		// lazy code
		// rebuild the mesh form scratch.
		{
			
			std::vector<int> indices;

			std::unordered_map<Vert, int> vertex_map;
			std::vector<Vert> vertices;

			for (int shape_idx = 0; shape_idx < num_shapes; shape_idx++) {
				tinyobj_shape_t shape = shapes[shape_idx];
				for (int i = 0; i < shape.length * 3; i++) {
					auto attr_indices = attr.faces[(shapes[shape_idx].face_offset * 3 + i)];

					int v_idx = attr_indices.v_idx;
					int vt_idx = attr_indices.vt_idx;
					int vn_idx = (attr.num_normals == 0) ? vt_idx : attr_indices.vn_idx;

					vec2 uv;
					uv.x() = attr.texcoords[vt_idx * 2];
					uv.y() = attr.texcoords[vt_idx * 2 + 1];

					Vert v = {};
					v.vert = m.verts[v_idx];
					v.normal = m.normals[vn_idx];
					v.uv = uv;
					v.shape = shape_idx;
					auto it = vertex_map.find(v);
					if (it != vertex_map.end()) {
						indices.push_back(it->second);
					} else {
						indices.push_back(vertices.size());
						vertex_map.insert(std::make_pair(v, vertices.size()));
						vertices.push_back(v);
					}
				}
			}

			m.verts = (vec3 *)malloc(vertices.size() * sizeof(vec3));
			m.normals = (vec3 *)malloc(vertices.size() * sizeof(vec3));
			m.uv = (vec2 *)malloc(vertices.size() * sizeof(vec2));
			m.shape_idx = (int *)malloc(vertices.size() * sizeof(int));
			m.num_verts = vertices.size();
			
			m.indices = (int *)malloc(indices.size() * sizeof(int));
			m.num_indices = indices.size(); 
			// why the fuck is this needed. surely we shouldnt change the number of triangles
			// but appearently shapes doesn't reference all of the triangles... wut?
			// are we missing something here?
			memcpy(m.indices, &indices[0], indices.size() * sizeof(int));

			for (int i = 0; i < vertices.size(); i++) {
				Vert v = vertices[i];
				m.verts[i] = vertices[i].vert;
				m.normals[i] = v.normal;
				m.uv[i] = v.uv;
				m.shape_idx[i] = v.shape;
			}
		}

	}




	
	Mesh m2;
	

	Atlas_Output_Mesh *output_mesh = NULL;
#if 1
	{
		
		// convert to theklas input format
		

		Atlas_Input_Mesh input_mesh;
		input_mesh.vertex_count = m.num_verts;
		input_mesh.vertex_array = (Atlas_Input_Vertex*)malloc(sizeof(Atlas_Input_Vertex)*m.num_verts);

		input_mesh.face_count = m.num_indices / 3;
		input_mesh.face_array = (Atlas_Input_Face  *)malloc(sizeof(Atlas_Input_Face)*m.num_indices/3);
		
		for (int i = 0; i < input_mesh.face_count; i++) {
			input_mesh.face_array[i].vertex_index[0] = m.indices[i * 3 + 0];
			input_mesh.face_array[i].vertex_index[1] = m.indices[i * 3 + 1];
			input_mesh.face_array[i].vertex_index[2] = m.indices[i * 3 + 2];
		}

		for (int i = 0; i < input_mesh.vertex_count; i++) {
			input_mesh.vertex_array[i] = {};
			input_mesh.vertex_array[i].position[0] = m.verts[i].x();
			input_mesh.vertex_array[i].position[1] = m.verts[i].y();
			input_mesh.vertex_array[i].position[2] = m.verts[i].z();
			
			input_mesh.vertex_array[i].normal[0] = m.normals[i].x();
			input_mesh.vertex_array[i].normal[1] = m.normals[i].y();
			input_mesh.vertex_array[i].normal[2] = m.normals[i].z();
			
			input_mesh.vertex_array[i].uv[0] = m.uv[i].x();
			input_mesh.vertex_array[i].uv[1] = m.uv[i].y();
			
			input_mesh.vertex_array[i].first_colocal = i;
		}
		

		// Generate Atlas_Output_Mesh.
		Atlas_Options atlas_options;
		atlas_set_default_options(&atlas_options);


		// Avoid brute force packing, since it can be unusably slow in some situations.
		atlas_options.packer_options.witness.packing_quality = 1;
		atlas_options.packer_options.witness.conservative = false;
		atlas_options.packer_options.witness.texel_area = 2; // approx the size we want 
		atlas_options.packer_options.witness.block_align = false;
		atlas_options.charter_options.witness.max_chart_area = 100;



		Atlas_Error error = Atlas_Error_Success;
		output_mesh = atlas_generate(&input_mesh, &atlas_options, &error);

		printf("Atlas mesh has %d verts\n", output_mesh->vertex_count);
		printf("Atlas mesh has %d triangles\n", output_mesh->index_count / 3);
		printf("Produced debug_packer_final.tga\n");

		// Fix varying output sizes so that we get 1024*1024
		printf("Atlas size: %d * %d\n", output_mesh->atlas_width, output_mesh->atlas_height);
		printf("No rescaling\n");
		//float rescaleFactor = 1024.0 / max(output_mesh->atlas_width, output_mesh->atlas_height);
		//printf("Rescale factor: %f\n", rescaleFactor);
		//resize_thekla_atlas(output_mesh, rescaleFactor);

		
		{

			m2.num_indices = output_mesh->index_count;
			m2.indices = output_mesh->index_array;
			m2.num_verts = output_mesh->vertex_count;
			m2.verts = (vec3 *)malloc(m2.num_verts * sizeof(vec3));
			m2.normals = (vec3 *)malloc(m2.num_verts * sizeof(vec3));
			m2.lightmap_uv = (vec2 *)malloc(m2.num_verts * sizeof(vec2));
			m2.uv= (vec2 *)malloc(m2.num_verts * sizeof(vec2));
			m2.shape_idx = (int *)malloc(m2.num_verts * sizeof(int));

			for (int i = 0; i < m2.num_verts; i++) {
				auto v_ref = output_mesh->vertex_array[i].xref;
				m2.verts[i] = m.verts[v_ref];
				m2.normals[i] = m.normals[v_ref];
				m2.lightmap_uv[i].x() = output_mesh->vertex_array[i].uv[0];
				m2.lightmap_uv[i].y() = output_mesh->vertex_array[i].uv[1];
				m2.uv[i] = m.uv[v_ref];
				m2.shape_idx[i] = m.shape_idx[v_ref];

			}
		}



		write_obj(shapes, num_shapes, &m2, OBJ_FILE_PATH "_2xuv");

		free(input_mesh.face_array);
		free(input_mesh.vertex_array);
	}
#endif

	static VoxelScene data;


	std::vector<vec3>probes;
	{//voxelize and generate probes
#if 1
		voxelize_scene(m, &data);
		std::vector<ivec3>probe_voxels;
		flood_fill_voxel_scene(&data, probe_voxels);
		write_voxel_data(&data, "../voxels.dat");

		get_voxel_centers(probe_voxels, &data, probes);

#ifdef T_SCENE
		reduce_probes(probes, &data, RHO_PROBES);
#else
		reduce_probes(probes, &data, RHO_PROBES / 4);
		reduce_probes(probes, &data, RHO_PROBES / 2);
		reduce_probes(probes, &data, RHO_PROBES);
#endif

#endif	

		write_probe_data(probes, PRECOMP_ASSET_FOLDER "probes.dat");
		printf("Probes saved to " PRECOMP_ASSET_FOLDER "probes.dat" );
	}

	std::vector<ProbeData> probe_data(probes.size());
#if 1
	{
		std::vector<vec3> relight_ray_directions;

		printf("\nGenerating relight ray directions...\n");
		generate_relight_ray_directions_spaced(relight_ray_directions, RELIGHT_RAYS_PER_PROBE);
		write_probe_data(relight_ray_directions, PRECOMP_ASSET_FOLDER "relight_directions.dat");
		printf("\nPrecomputing relight uvs...\n");

		precompute_lightmap_uvs(probe_data, probes, relight_ray_directions, output_mesh, m);
		for (int i = 0; i < probes.size(); i++) {
			for (int j = 0; j < RELIGHT_RAYS_PER_PROBE; j++) {
				vec2 uv = probe_data[i].relight_rays_uv[j];
				printf("Probe %d, relight ray %d:       %f %f\n", i, j, uv[0], uv[1]);
			}
		}

		write_relight_uvs(probe_data, PRECOMP_ASSET_FOLDER "relight_uvs.dat");
		write_relight_shs(relight_ray_directions, PRECOMP_ASSET_FOLDER "relight_shs.dat");
	}
#endif


#if 0
	std::vector<Receiver>receivers;

	{ // generate receivers
		compute_receiver_locations(output_mesh, m, receivers, &data);
		FILE *f = fopen(PRECOMP_ASSET_FOLDER "receiver_px_map.imatrix", "wb");
		int num_recs = receivers.size();
		int num_comps = 2;

		fwrite(&num_comps, sizeof(num_comps), 1, f);
		fwrite(&num_recs, sizeof(num_recs), 1, f);
		for (Receiver rec : receivers) {
			fwrite(rec.px.data(), sizeof(int), 2, f);
		}
		fclose(f);
	}
#endif

	{ // compute local transport

		

		visibility(probes, &m2);
	}



	// Free stuff
	atlas_free(output_mesh);
	tinyobj_attrib_free(&attr);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
	return 0;
}
