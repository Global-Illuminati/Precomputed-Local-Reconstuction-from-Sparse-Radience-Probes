#version 300 es
precision highp float;
uniform sampler2D sigma_v; //  num_principal_components * num_probes 
uniform sampler2D sh_coeffs; // num_shs *  num_probes
out vec4 o_sh;
void main()
{
	int num_probes = textureSize(sigma_v, 0).y;
	int pc_index = int(gl_FragCoord.x);
	int sh_index = int(gl_FragCoord.y);
	vec3 c = vec3(0);
	for(int p = 0; p < num_probes;p++){
		float a = texelFetch(sigma_v,ivec2(pc_index,p),0).r;
		vec3 b = texelFetch(sh_coeffs,ivec2(sh_index,p),0).rgb;  
		c += a*b;
	}
	o_sh = vec4(c,1);
}
