#version 300 es
precision highp float;

uniform sampler2D probes_sh_coeffs; 
uniform sampler2D rec_sh_coeffs;
flat in int sh_index;
out vec4 o_light;
void main()
{
	int num_probes = 6;
	
	vec3 ret = vec3(0);
	int log2_tex_size = 12;
	int mask = (1<<log2_tex_size)-1;
	int num_shs = 16;

	for(int i = 0; i< num_probes;i++){
		for(int j=0;j<num_shs/4;j++){
			int rec_index = (sh_index + i*4+j);
			ivec2 rec_loc = ivec2(rec_index & mask, rec_index>>log2_tex_size);
			vec4 a = texelFetch(rec_sh_coeffs,rec_loc,0);

			ret += a.x * texelFetch(probes_sh_coeffs,ivec2(j*4+0,i),0).xyz;
			ret += a.y * texelFetch(probes_sh_coeffs,ivec2(j*4+1,i),0).xyz;
			ret += a.z * texelFetch(probes_sh_coeffs,ivec2(j*4+2,i),0).xyz;
			ret += a.w * texelFetch(probes_sh_coeffs,ivec2(j*4+3,i),0).xyz;
		}
	}
	o_light = vec4(ret,1);
}
