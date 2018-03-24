#version 300 es
precision highp float;

uniform sampler2D pc_sh_coeffs; 
uniform sampler2D rec_sh_coeffs;
flat in int sh_index;
out vec4 o_light;
void main()
{
	int num_pc_probes = 16;
	
	vec2 px = gl_FragCoord.xy;
	vec3 ret = vec3(0);
	for(int i = 0; i< num_pc_probes;i++){
		for(int j=0;j<4;j++){  // 4 = num_sh_probes / 4 = 16 / 4
			 // we might want to calculate this earlier, dependency for read, want to allow for the driver to hide latency etc.
			 // but then we need it stored so maybe not?
			ivec2 rec_loc = ivec2((sh_index + i*4+j)&4094, (sh_index+i*4+j)>>12);
			vec4 a = texelFetch(rec_sh_coeffs,rec_loc,0);

			ret += a.x * texelFetch(pc_sh_coeffs,ivec2(i,j*4+0),0).xyz;
			ret += a.y * texelFetch(pc_sh_coeffs,ivec2(i,j*4+1),0).xyz;
			ret += a.z * texelFetch(pc_sh_coeffs,ivec2(i,j*4+2),0).xyz;
			ret += a.w * texelFetch(pc_sh_coeffs,ivec2(i,j*4+3),0).xyz;
		}
	}
	o_light = vec4(ret,1);
	
}
