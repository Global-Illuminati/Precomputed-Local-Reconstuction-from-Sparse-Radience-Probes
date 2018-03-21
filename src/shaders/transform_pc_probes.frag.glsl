#version 300 es
precision highp float;
uniform sampler2D sigma_vt; // num_probes * num_principal_components
uniform sampler2D sh_coeffs[4]; // num_probes 
out vec4 o_sh;
void main()
{
	ivec2 px = ivec2(gl_FragCoord.xy);
	int pc_index = px.x;
	int major_sh_index = px.y;
	o_sh = vec4(0);
	for(int p = 0; p < 16;p++){
		float a = texelFetch(sigma_vt,ivec2(p,pc_index),0).r;
		vec4 b = texelFetch(sh_coeffs[0],ivec2(p,0),0);  // wut do we have to have constant indicies into texture arrays? :( 
		o_sh += a*b;
	}
}
