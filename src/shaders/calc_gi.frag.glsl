#version 300 es
precision highp float;
precision highp int;

uniform sampler2D pc_sh_coeffs; 
uniform sampler2D rec_sh_coeffs;
flat in int receiver_index;
out vec4 o_light;
void main()
{
	vec3 ret = vec3(0);
	int log2_tex_size = 14;
	int mask = (1<<log2_tex_size)-1;

	for(int i = 0; i< 64;i++){
		int offset = (receiver_index*64 + i);
		ivec2 rec_loc = ivec2(offset & mask, offset>>log2_tex_size);
		ret += texelFetch(rec_sh_coeffs,rec_loc,0).x * texelFetch(pc_sh_coeffs,ivec2(i,0),0).xyz;
	}
	o_light = vec4(ret*10.0	,1);
}
