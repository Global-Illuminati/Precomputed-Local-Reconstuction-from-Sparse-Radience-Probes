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
uniform int num_sh_samples;


void main()
{

	for(int i = 0; i < 8; i++) {
		out_value[i] = vec4(0.0);
	}
	if(!gl_FrontFacing) return;

	float ack_probe_weight = 0.0;
	vec3 receiver_dir = world_pos-receiver_pos;
	for(int i = 0; i < 8; i++) 
	{
		
		vec3 probe_dir = world_pos-probe_pos[i]; 
		float sampled_light_dist = texture(probe_depth[i], probe_dir).r; 
		float real_distance = length(probe_dir);

		if(real_distance < sampled_light_dist + 0.01 && dot(probe_dir,normal) < 0){
			out_value[i] = vec4(probe_dir,probe_weight[i]);
			ack_probe_weight += probe_weight[i];
		}
	}
	float inv_ack_probe_weight = (ack_probe_weight == 0.0) ? 0.0 : (1.0/ack_probe_weight);

	// correcting for non uniform sampling on sphere
	vec2 u = (gl_FragCoord.xy+vec2(0.5))/float(num_sh_samples)*2.0f-1.0f;
	float tmp = 1.0f+dot(u,u);
	float sample_weight = 4.0f/(sqrt(tmp)*tmp); 

	float cos_factor = max(0.0f,dot(normalize(receiver_dir),normalize(receiver_normal)));
	//float cos_factor = 1.0;

	for(int i = 0; i<8;i++)
		out_value[i].a *= sample_weight*inv_ack_probe_weight*cos_factor;
}
