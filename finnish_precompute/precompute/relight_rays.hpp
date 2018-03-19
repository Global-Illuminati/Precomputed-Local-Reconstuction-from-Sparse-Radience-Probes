
#define RELIGHT_RAYS_PER_PROBE 100 //8000

#define PI 3.1415926535898

#include <vector>
#include "Eigen\Dense"
typedef Eigen::Vector2f vec2;
typedef Eigen::Vector3f vec3;
typedef Eigen::Vector2i ivec2;
typedef Eigen::Vector3i ivec3;

#include <random>
#include<cmath>



void generate_relight_ray_directions(std::vector<vec3> &relight_ray_directions, int num_relight_rays) {
	std::default_random_engine generator;
	std::uniform_real_distribution<float> uniform01(0.0, 1.0);
	for (int i = 0; i < num_relight_rays; i++) {
		float theta = 2 * PI * uniform01(generator);
		float phi = acos(1 - 2 * uniform01(generator));
		float x = sin(phi)*cos(theta);
		float y = sin(phi)*sin(theta);
		float z = cos(phi);
		relight_ray_directions.push_back(vec3(x, y, z));
	}
}

struct ProbeData {
	vec2 relight_rays_uv[RELIGHT_RAYS_PER_PROBE];
};

void precompute_lightmap_uvs(std::vector<ProbeData> &probe_data, const std::vector<vec3> &probes, const std::vector<vec3> &relight_ray_directions, Atlas_Output_Mesh *light_map_mesh, Mesh mesh) {
	int debug_tenth = 0;
	
	int num_probes = probes.size();
	int num_relight_rays = relight_ray_directions.size();
	for (int probe_index = 0; probe_index < num_probes; probe_index++) {
		printf("Precalculating relight rays for probe %d", probe_index);
		for (int relight_ray_index = 0; relight_ray_index < num_relight_rays; relight_ray_index++) {
			Ray ray = {probes[probe_index], relight_ray_directions[relight_ray_index]};
			vec2 hit_uv;
			if (lightmap_uv_of_closest_intersection(ray, light_map_mesh, mesh, &hit_uv)) {
				probe_data[probe_index].relight_rays_uv[relight_ray_index] = hit_uv;
			}
			else {
				probe_data[probe_index].relight_rays_uv[relight_ray_index] = vec2(-1.0f,-1.0f);
			}
			if (10 * relight_ray_index / num_relight_rays != debug_tenth) {
				debug_tenth = 10 * relight_ray_index / num_relight_rays;
				printf(" . ");
			}
		}
		printf("\n");
	}
}


void write_relight_uvs(const std::vector<ProbeData> &probe_data, char *file_path) {
	FILE *f = fopen(file_path, "w");

	fprintf(f, "%d %d\n", probe_data.size(), RELIGHT_RAYS_PER_PROBE);
	for (int i = 0; i < probe_data.size(); i++) {
		for (int j = 0; j < RELIGHT_RAYS_PER_PROBE; j++) {
			vec2 uv = probe_data[i].relight_rays_uv[j];
			fprintf(f, "%f %f ", uv.x(), uv.y());
		}
		fprintf(f,"\n");
	}
	
	fclose(f);
}

extern const int num_sh_coeffs;
void write_relight_shs(std::vector<vec3> &relight_dirs, char *file_path) {
	FILE *f = fopen(file_path, "w");

	fprintf(f, "%d, %d\n",relight_dirs.size(), num_sh_coeffs);
	for (int i = 0; i < relight_dirs.size(); i++) {
		vec3 v = relight_dirs[i].normalized();
		float x = v.x();
		float y = v.y();
		float z = v.z();

		fprintf(f, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
			0.282098949,
			-0.488609731* y,
			0.488609731 * z,
			-0.488609731* x,

			1.09256458	 * y*x,
			-1.09256458 * y*z,
			0.315396219 * (3 * z*z - 1),
			-1.09256458	 * x*z,

			0.546282291	*(x*x - y * y),
			-0.590052307 *y*(3 * x*x - y * y),
			2.89065409 * x*y*z,
			-0.457052559 * y*(-1 + 5 * z*z),

			0.373181850	*z*(5 * z*z - 3),
			-0.457052559*x*(-1 + 5 * z*z),
			1.44532704	*(x*x - y * y)*z,
			-0.590052307*x*(x*x - 3 * y*y));
	}

	fclose(f);
}