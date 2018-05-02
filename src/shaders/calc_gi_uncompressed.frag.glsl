#version 300 es
precision highp float;
precision highp int;

uniform sampler2D probes_sh_coeffs; 
uniform sampler2D rec_sh_coeffs;
flat in int receiver_index;
flat in ivec4 probe_indices;

out vec4 o_light;
void main()
{
	int num_probes = 72;
	
	vec3 ret = vec3(0);
	const int log2_tex_size = 14;
	const int mask = (1<<log2_tex_size)-1;
	const int num_shs = 16;
	const int max_num_probes_per_receiver=8;
	//@Cleanup send these in as int16_t-s directly. This is ugly 
	int probe_indices_expanded[8];
	probe_indices_expanded[0] = probe_indices.x & 0xffff;
	probe_indices_expanded[1] = probe_indices.x >> 16;
	probe_indices_expanded[2] = probe_indices.y & 0xffff;
	probe_indices_expanded[3] = probe_indices.y >> 16;
	probe_indices_expanded[4] = probe_indices.z & 0xffff;
	probe_indices_expanded[5] = probe_indices.z >> 16;
	probe_indices_expanded[6] = probe_indices.w & 0xffff;
	probe_indices_expanded[7] = probe_indices.w >> 16;

	for(int i = 0; i< max_num_probes_per_receiver;i++){
		int probe_index = probe_indices_expanded[i];
		if(probe_index == -1) break;
		for(int j=0;j<4;j++){
			int coeff_index = (receiver_index*max_num_probes_per_receiver*4 + i*4+j);
			ivec2 rec_loc = ivec2(coeff_index & mask, coeff_index>>log2_tex_size);
			vec4 a = texelFetch(rec_sh_coeffs,rec_loc,0);
			ret += a.x * texelFetch(probes_sh_coeffs,ivec2(j*4+0,probe_index),0).xyz;
			ret += a.y * texelFetch(probes_sh_coeffs,ivec2(j*4+1,probe_index),0).xyz;
			ret += a.z * texelFetch(probes_sh_coeffs,ivec2(j*4+2,probe_index),0).xyz;
			ret += a.w * texelFetch(probes_sh_coeffs,ivec2(j*4+3,probe_index),0).xyz;
		}
	}
	o_light = vec4(ret,1);
}
