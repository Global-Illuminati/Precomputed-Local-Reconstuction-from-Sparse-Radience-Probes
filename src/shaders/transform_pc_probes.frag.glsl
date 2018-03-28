#version 300 es
precision highp float;
uniform sampler2D sigma_v; //  num_principal_components * num_probes 
uniform sampler2D sh_coeffs; // num_shs *  num_probes
out vec4 o_sh;
void main()
{
	int num_probes = textureSize(sigma_v, 0).y/16; // = 72
	int num_pc = textureSize(sigma_v, 0).x; // = 64
	int pc_index = int(gl_FragCoord.x);
	vec3 c = vec3(0);
	for(int p = 0; p < num_probes;p++){
		for(int s = 0; s < 16; s++)
		{
			// 1152 iterations here. might be quite slow,
			// but not very many pixels,
			// might split it into multiple passes or what ever.  
			float a = texelFetch(sigma_v,ivec2(p*16+s,pc_index),0).r;
			vec3 b = texelFetch(sh_coeffs,ivec2(s,p),0).rgb;  
			c += a*b;
		}
	}
	o_sh = vec4(c,1);
}
