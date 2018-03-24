#version 300 es
precision highp float;
uniform sampler2D sigma_vt; // num_probes * num_principal_components
uniform sampler2D sh_coeffs; // num_probes 
out vec4 o_sh;
void main()
{
	ivec2 px = ivec2(gl_FragCoord.xy);
	int pc_index = px.x;
	int major_sh_index = px.y;
	vec3 c = vec3(0);
	for(int p = 0; p < 16;p++){
		float a = texelFetch(sigma_vt,ivec2(p,pc_index),0).r;
		for(int i = 0; i<4;i++){
			vec3 b = texelFetch(sh_coeffs,ivec2(p,pc_index*4+i),0).rgb;  
			c += a*b;
		}
	}
	o_sh = vec4(c,1);
}
