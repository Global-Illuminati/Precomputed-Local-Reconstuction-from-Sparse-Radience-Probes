#version 300 es
precision highp float;
uniform sampler2D sigma_v; //  num_principal_components * num_probes 
uniform sampler2D sh_coeffs; // num_shs *  num_probes
out vec4 o_sh;
void main()
{
	int num_c = textureSize(sigma_v, 0).x;
	int pc_index = int(gl_FragCoord.x);
	vec3 o = vec3(0);
	for(int c = 0; c < num_c; c++){
		float a = texelFetch(sigma_v,ivec2(c,pc_index),0).r;
		vec3 b = texelFetch(sh_coeffs,ivec2(c&15,c>>4),0).rgb;  
		o += a*b;
	}
	o_sh = vec4(o,1.0);
}
