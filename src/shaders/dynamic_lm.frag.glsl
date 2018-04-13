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

layout(location = 0) out vec4 o_color;

//void convolveCosineInZ(float *result, ) {
//void CosMult(float *R, float *L) { // constants to make code fit
//	 const float T1 = 0.09547032698f;
//	 const float T2 = 0.1169267933f;
//	 const float T3 = 0.2581988897f;
//	 const float T4 = 0.2886751347f;
//	 const float T5 = 0.2390457218f;
//	 const float T6 = 0.2535462764f;
//	 const float T7 = 0.2182178903f;
//	 const float T8 = 0.1083940384f;
//	 const float T9 = 0.2519763153f;
//	 const float TA = 0.2439750183f;
//	 const float TB = 0.3115234375f;
//	 const float TC = 0.2512594538f;
//	 const float TD = 0.31640625f;
//	 R[0] = 0.25f * L[0] - 0.03125f * L[20] + T4 * L[2] + 0.1397542486f * L[6];
//	 R[1] = 0.2236067977f * L[5] + T2* L[11] - 0.02896952533f * L[29] + 0.1875f * L[1];
//	 R[2] = 0.375f * L[2] + T4 * L[0] + T3 * L[6] - 0.01495979856f * L[30] + T1 * L[12];
//	 R[3] = 0.2236067977f * L[7] - 0.02896952533f * L[31] + T2* L[13] + 0.1875f * L[3];
//	 R[4] = 0.101487352f * L[18] + 0.1889822365f * L[10] + 0.15625f * L[4];
//	 R[5] = 0.2236067977f * L[1] + T5 * L[11] + 0.3125f * L[5] + 0.09568319309f * L[19];
//	 R[6] = T6 * L[12] + T3 * L[2] + 0.3125f * L[6] + 0.113550327f * L[20] + 0.1397542486f * L[0];
//	 R[7] = T5 * L[13] + 0.09568319309f * L[21] + 0.2236067977f * L[3] + 0.3125f * L[7];
//	 R[8] = 0.15625f * L[8] + 0.101487352f * L[22] + 0.1889822365f * L[14];
//	 R[9] = 0.09068895910f * L[27] + 0.1666666667f * L[17] + 0.13671875f * L[9];
//	 R[10] = T7 * L[18] + 0.2734375f * L[10] + 0.09068895910f * L[28] + 0.1889822365f * L[4];
//	 R[11] = T8 * L[29] + TA * L[19] + T5 * L[5] + 0.30078125f * L[11] + T2* L[1];
//	 R[12] = T9 * L[20] + 0.328125f * L[12] + T6 * L[6] + 0.1028316139f * L[30] + T1 * L[2];
//	 R[13] = 0.30078125f * L[13] + T8 * L[31] + TA * L[21] + T5 * L[7] + T2* L[3];
//	 R[14] = 0.2734375f * L[14] + 0.09068895910f * L[32] + 0.1889822365f * L[8] + T7 * L[22];
//	 R[15] = 0.0906889591f * L[33] + 0.13671875f * L[15] + 0.1666666667f * L[23];
// }
//}

//vec3 irradianceFromProbe(int probe_index) {
//
//}

vec3 radianceFromProbe(int probe_index) {
    return (texelFetch(u_probe_sh_texture, ivec2(0,  probe_index), 0) * 0.282098949
    + texelFetch(u_probe_sh_texture, ivec2(1,  probe_index), 0) * -0.488609731* v_normal.y
    + texelFetch(u_probe_sh_texture, ivec2(2,  probe_index), 0) * 0.488609731 * v_normal.z
    + texelFetch(u_probe_sh_texture, ivec2(3,  probe_index), 0) * -0.488609731* v_normal.x

    + texelFetch(u_probe_sh_texture, ivec2(4,  probe_index), 0) * 1.09256458	 * v_normal.y*v_normal.x
    + texelFetch(u_probe_sh_texture, ivec2(5,  probe_index), 0) * -1.09256458 * v_normal.y*v_normal.z
    + texelFetch(u_probe_sh_texture, ivec2(6,  probe_index), 0) * 0.315396219 * (3.0 * v_normal.z*v_normal.z - 1.0)
    + texelFetch(u_probe_sh_texture, ivec2(7,  probe_index), 0) * -1.09256458 * v_normal.x*v_normal.z

    + texelFetch(u_probe_sh_texture, ivec2(8,  probe_index), 0) * 0.546282291 * (v_normal.x*v_normal.x - v_normal.y * v_normal.y)
    + texelFetch(u_probe_sh_texture, ivec2(9,  probe_index), 0) * -0.590052307* v_normal.y*(3.0 * v_normal.x*v_normal.x - v_normal.y * v_normal.y)
    + texelFetch(u_probe_sh_texture, ivec2(10, probe_index), 0) * 2.89065409  * v_normal.x*v_normal.y*v_normal.z
    + texelFetch(u_probe_sh_texture, ivec2(11, probe_index), 0) * -0.457052559* v_normal.y*(-1.0 + 5.0 * v_normal.z*v_normal.z)

    + texelFetch(u_probe_sh_texture, ivec2(12, probe_index), 0) * 0.373181850 * v_normal.z*(5.0 * v_normal.z*v_normal.z - 3.0)
    + texelFetch(u_probe_sh_texture, ivec2(13, probe_index), 0) * -0.457052559* v_normal.x*(-1.0 + 5.0 * v_normal.z*v_normal.z)
    + texelFetch(u_probe_sh_texture, ivec2(14, probe_index), 0) * 1.44532704	 * (v_normal.x*v_normal.x - v_normal.y * v_normal.y)*v_normal.z
    + texelFetch(u_probe_sh_texture, ivec2(15, probe_index), 0) * -0.590052307*v_normal.x*(v_normal.x*v_normal.x - 3.0 * v_normal.y*v_normal.y)).xyz;
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
	    float weight = 1.0 / (1.0 + dot(distVec, distVec));
	    totalWeight += weight;
//	    if (i % 3 == 2) {
        indirectLight += weight * radianceFromProbe(i); // TODO: Use irradiance instead of radiance
//	    }

	}

	indirectLight /= totalWeight;

    vec3 color = indirectLight * 2.0;// * 3.14 * 2.0;


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
