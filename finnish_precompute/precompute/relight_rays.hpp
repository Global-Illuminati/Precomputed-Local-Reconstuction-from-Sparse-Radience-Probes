
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