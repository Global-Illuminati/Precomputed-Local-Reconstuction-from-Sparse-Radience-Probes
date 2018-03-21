#version 300 es
precision highp float;

uniform sampler2D pc_sh_coeffs; // num_principal_components * num_sh_coeffs/4
uniform sampler2D rec_sh_coeffs;
flat in int sh_index;
out float o_light;
void main()
{
	int num_pc_probes = 16;
	
	vec2 px = gl_FragCoord.xy;
	vec4 ack = vec4(0);
	for(int i = 0; i< num_pc_probes;i++){
		for(int j=0;j<4;j++){
			 // we might want to calculate this earlier, dependency for read, want to allow for the driver to hide latency etc.
			 // but then we need it stored so maybe not?
			ivec2 rec_loc = ivec2((sh_index + i*4+j)&1023, (sh_index+i*4+j)>>10);
			ivec2 pc_loc  = ivec2(i,j);

			ack += texelFetch(pc_sh_coeffs,pc_loc,0)*texelFetch(rec_sh_coeffs,rec_loc,0);
		}
	}
	o_light = dot(ack,vec4(1)); // == ack.x+ack.y+ack.z+ack.w
}
