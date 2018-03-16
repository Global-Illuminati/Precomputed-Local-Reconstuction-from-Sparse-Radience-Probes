#version 420
precision highp float;

in vec3 world_pos;
in vec3 normal;
in vec3 forward;

layout(location = 0) out vec4[8] out_value; // 16 first coeffs for the spherical harmonics output of two probes

layout(binding = 0) uniform samplerCube probe_depth[8];
uniform vec3 probe_pos[8];
uniform vec3 receiver_normal;
uniform vec3 receiver_pos;
uniform float probe_weight[8];
void main()
{

	float ack_probe_weight = 0.0;
	for(int i = 0; i < 8; i++) 
	{
		vec3 probe_dir = probe_pos[i]-world_pos; 
		float sampled_light_dist = texture(probe_depth[i], -probe_dir).r; // why the minus here?? 
		float real_distance = length(probe_dir);
		out_value[i] = (real_distance >= sampled_light_dist + 0.03) ? vec4(probe_dir,probe_weight[i]) : vec4(0.0);
		ack_probe_weight += out_value[i].a;
	}
	float inv_ack_probe_weight = (ack_probe_weight == 0.0) ? 0.0 : (1.0/ack_probe_weight);

	// correcting for non uniform sampling on the sphere, w = percentage of area we sample.
	vec2 u = gl_FragCoord.xy*2-1;
	float sample_weight = (u.x*u.x+1)/(2*u.x*u.x+1) * (u.y*u.y+1)/(2*u.y*u.y+1)/2.808;

	float cos_factor = max(0.0f,dot(normalize(receiver_pos-world_pos),receiver_normal));

	for(int i = 0; i<8;i++)
		out_value[i].a *= sample_weight*inv_ack_probe_weight*cos_factor;
}
