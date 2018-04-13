#version 300 es
precision highp float;

#include <common.glsl>


in vec3 v_position;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_tex_coord;
in vec4 v_light_space_position;
in vec3 v_world_position;

#include <scene_uniforms.glsl>

uniform sampler2D u_probe_sh_texture;

uniform sampler2D u_diffuse_map;
uniform sampler2D u_specular_map;
uniform sampler2D u_normal_map;
uniform lowp sampler2DShadow u_shadow_map;
uniform sampler2D u_probe_pos_texture;

uniform vec3 u_dir_light_color;
uniform vec3 u_dir_light_view_direction;

uniform int u_num_probes;

uniform float u_probe_support_radius_squared;

layout(location = 0) out vec4 o_color;

/*
 Wow, this was easier than I thought. It turns out that the ZH for the clamped cosine
  is zero in the entire third band, which means that the irradiance does not depend on the SH coefficients in that band.
  (I guess it is kind of intuitive because the cosine convolution has a blurring effect,
  so it's natural that some higher frequency bands disappear)

  The following code combines the rotated clamped-cosine convolution and evaluation in the normal direction into one surprisingly simple expression.
  See equation (13) of this paper: (http://graphics.stanford.edu/papers/envmap/)
*/
vec3 irradianceFromProbe(int probe_index) {
    return 0.429043 *
        texelFetch(u_probe_sh_texture, ivec2(8,  probe_index), 0).xyz
         * (v_normal.x*v_normal.x - v_normal.y*v_normal.y) + 0.743125 *
         texelFetch(u_probe_sh_texture, ivec2(6,  probe_index), 0).xyz
         * v_normal.z * v_normal.z + 0.886227 *
         texelFetch(u_probe_sh_texture, ivec2(0,  probe_index), 0).xyz
         - 0.247708 *
         texelFetch(u_probe_sh_texture, ivec2(6,  probe_index), 0).xyz

        + 2.0 * 0.429043 * (
        texelFetch(u_probe_sh_texture, ivec2(4,  probe_index), 0).xyz
         * v_normal.x * v_normal.y +
         texelFetch(u_probe_sh_texture, ivec2(7,  probe_index), 0).xyz
         * v_normal.x * v_normal.z +
         texelFetch(u_probe_sh_texture, ivec2(5,  probe_index), 0).xyz
         * v_normal.y * v_normal.z)

        + 2.0 * 0.511664 * (
        texelFetch(u_probe_sh_texture, ivec2(3,  probe_index), 0).xyz
         * v_normal.x +
         texelFetch(u_probe_sh_texture, ivec2(1,  probe_index), 0).xyz
         * v_normal.y +
         texelFetch(u_probe_sh_texture, ivec2(2,  probe_index), 0).xyz
         * v_normal.z);
}

float w(float t) {
	return 2.0 * pow(t, 3.0) - 3.0 * pow(t, 2.0) + 1.0;
}

float w_optimized(float t_squared) {
	return 2.0 * pow(t_squared, 1.5) - 3.0 * t_squared + 1.0;
}

void main()
{
	vec3 N = normalize(v_normal);
	vec3 T = normalize(v_tangent);
	vec3 B = normalize(v_bitangent);

	// NOTE: We probably don't really need all (or any) of these
	reortogonalize(N, T);
	reortogonalize(N, B);
	reortogonalize(T, B);
	mat3 tbn = mat3(T, B, N);

	// Rotate normal map normals from tangent space to view space (normal mapping)
	vec3 mapped_normal = texture(u_normal_map, v_tex_coord).xyz;
	mapped_normal = normalize(mapped_normal * vec3(2.0) - vec3(1.0));
	N = tbn * mapped_normal;

	vec3 diffuse = texture(u_diffuse_map, v_tex_coord).rgb;
	float shininess = texture(u_specular_map, v_tex_coord).r;

	vec3 wi = normalize(-u_dir_light_view_direction);
	vec3 wo = normalize(-v_position);

	float lambertian = saturate(dot(N, wi));

	//////////////////////////////////////////////////////////
	// indirect light

	vec3 indirectLight = vec3(0.0, 0.0, 0.0);

	float totalWeight = 0.0;

	for (int i = 0; i < u_num_probes; i++) {
	    vec3 probePos = texelFetch(u_probe_pos_texture, ivec2(i, 0), 0).xyz;
	    vec3 distVec = probePos - v_world_position;
	    float squaredDist = dot(distVec, distVec);
	    if (squaredDist < u_probe_support_radius_squared) {
	        float weight = w_optimized(squaredDist / u_probe_support_radius_squared);
            totalWeight += weight;
//             if (i % 3 == 2) {
            indirectLight += weight * irradianceFromProbe(i);
//             }
	    }
	}

	indirectLight /= totalWeight;

    vec3 color = indirectLight;// * 3.14 * 2.0;

    // Should probably render this indirect light to a low-res texture instead since it is so slow-varying

	//////////////////////////////////////////////////////////
	// directional light

	// shadow visibility
	const float bias = 0.0029;
	vec2 texel_size = vec2(1.0) / vec2(textureSize(u_shadow_map, 0));
	vec3 light_space = v_light_space_position.xyz / v_light_space_position.w;
	float visibility = sample_shadow_map_pcf(u_shadow_map, light_space, texel_size, bias);

	if (lambertian > 0.0 && visibility > 0.0)
	{
		vec3 wh = normalize(wi + wo);

		// diffuse
		color += visibility * diffuse * lambertian * u_dir_light_color;

		// specular
		float specular_angle = saturate(dot(N, wh));
		float specular_power = pow(2.0, 13.0 * shininess); // (fake glossiness from the specular map)
		float specular = pow(specular_angle, specular_power);
		color += visibility * shininess * specular * u_dir_light_color;
	}

	// output tangents
	o_color = vec4(color, 1.0);

}
