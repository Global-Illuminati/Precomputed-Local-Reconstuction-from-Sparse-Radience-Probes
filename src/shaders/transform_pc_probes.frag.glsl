#version 300 es
precision highp float;
uniform sampler2D sigma_vt; // num_probes * num_principal_components
uniform sampler2D sh_coeffs; // num_shs *  num_probes
out vec4 o_sh;
void main()
{
	int num_probes = textureSize(sigma_vt, 0).x;
	int pc_index = int(gl_FragCoord.x);
	int sh_index = int(gl_FragCoord.y);
	vec3 c = vec3(0);
	for(int p = 0; p < num_probes;p++){
		float a = texelFetch(sigma_vt,ivec2(p, pc_index),0).r;
		vec3 b = texelFetch(sh_coeffs,ivec2(sh_index,p),0).rgb;  
		c += a*b;
	}
	o_sh = vec4(c,1);
}
