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

	//@CLEANUP HARDCODED must be sent in!
	const float inv_radius = 1.0/10.7330141;

	float ack_probe_weight = 0.0;
	for(int i = 0; i < 8; i++) 
	{
		vec3 probe_dir = world_pos-probe_pos[i]; 
		float sampled_light_dist = texture(probe_depth[i], probe_dir).r; 
		float real_distance = length(probe_dir);

		if(real_distance < sampled_light_dist + 0.01)
		{
			float t = real_distance*inv_radius;
			float weight = 2 * t*t*t - 3 * t*t + 1;

			out_value[i] = vec4(probe_dir,weight);
			ack_probe_weight += weight;	
		}
		else
		{
			out_value[i] = vec4(0.0);
		}
	}
	float inv_ack_probe_weight = (ack_probe_weight == 0.0) ? 0.0 : (1.0/ack_probe_weight);

	// correcting for non uniform sampling on the sphere
	
	vec2 u = (gl_FragCoord.xy)/float(num_sh_samples-1)*2.0f-1.0f;
	float tmp = 1.0f+dot(u,u);
	float sample_weight = 4.0f/(sqrt(tmp)*tmp); // appearenlty this is the right thing...

	float cos_factor = max(0.0f,dot(normalize(world_pos-receiver_pos),receiver_normal));
	//float cos_factor = 1.0;

	for(int i = 0; i<8;i++)
		out_value[i].a *= sample_weight*inv_ack_probe_weight*cos_factor;
}
