#version 420
precision highp float;

in vec3 world_pos;
in vec3 normal;
in vec3 forward;

layout(location = 0) out vec4[8] o_sh; // 16 first coeffs for the spherical harmonics output of two probes

layout(binding = 0) uniform samplerCube probe_depth[2];
uniform vec3 probe_pos[2];
uniform vec3 receiver_normal;
uniform vec3 receiver_pos;

void main()
{
	for(int i = 0; i<8;i++)
		o_sh[i] = vec4(0.0);

	for(int i = 0; i < 2; i++) // probe_index
	{
		vec3 probe_dir = probe_pos[i]-world_pos; 
		float sampled_light_dist = texture(probe_depth[i], -probe_dir).r; // why the minus here?? 
		float real_distance = length(probe_dir);
		if (real_distance >= sampled_light_dist + 0.04)
		{ // world_pos is a mutualy visible point 
			
			vec3 v = normalize(probe_dir); // vector used in sh calcs
			float x = v.x;
			float y = v.y;
			float z = v.z;

			o_sh[i*4+0] += 
				vec4(
					0.282098949	,
					-0.488609731* y,
					0.488609731 * z,
					-0.488609731* x
				);
			
			o_sh[i*4+1] += 
				vec4(
					1.09256458	 * y*x,
					-1.09256458 * y*z,
					0.315396219 * (3 * z*z - 1),
					-1.09256458	 * x*z
				);

			o_sh[i*4+2] +=
				vec4(
					0.546282291	*(x*x - y * y),
					-0.590052307 *y*(3 * x*x - y*y),
					2.89065409 * x*y*z,
					-0.457052559 * y*(-1 + 5 * z*z)
				);
			
			o_sh[i*4+3] +=
				vec4(		
					0.373181850	*z*(5 * z*z - 3),
					-0.457052559*x*(-1 + 5 * z*z),
					1.44532704	*(x*x - y * y)*z,
					-0.590052307*x*(x*x - 3 * y*y)
				);
		}
	}

	vec2 u = gl_FragCoord.xy*2-1;
	// correcting for non uniform sampling on the sphere, w = percentage of area we sample.
	float w = (u.x*u.x+1)/(2*u.x*u.x+1) * (u.y*u.y+1)/(2*u.y*u.y+1)/2.808;

	float cos_factor = dot(normalize(receiver_pos-world_pos),receiver_normal);
	w *= cos_factor;

	for(int i = 0; i<8;i++)
		o_sh[i] *= w;

}
