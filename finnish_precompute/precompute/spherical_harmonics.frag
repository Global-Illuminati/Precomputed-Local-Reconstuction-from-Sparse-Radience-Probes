#version 420
precision highp float;

layout(binding = 0) uniform sampler2D[2] in_texture;
layout(location = 0) out vec4[8] o_sh;

void main()
{
	
	for(int i = 0; i<8; i++) o_sh[i] = vec4(0.0f);

	for(int i = 0; i<2; i++)
	{
		vec4 in_value = texelFetch(in_texture[i],ivec2(gl_FragCoord.xy),0);
		float weight = in_value.a;
		vec3 probe_dir = in_value.xyz;
		if(probe_dir == vec3(0,0,0)) continue;
		vec3 v = normalize(probe_dir); 
		float x = v.x;
		float y = v.y;
		float z = v.z;

		o_sh[i*4+0] += 
			vec4(
				0.282098949	,
				-0.488609731* y,
				0.488609731 * z,
				-0.488609731* x
			);
			
		o_sh[i*4+1] += 
			vec4(
				1.09256458	 * y*x,
				-1.09256458 * y*z,
				0.315396219 * (3 * z*z - 1),
				-1.09256458	 * x*z
			);

		o_sh[i*4+2] +=
			vec4(
				0.546282291	*(x*x - y * y),
				-0.590052307 *y*(3 * x*x - y*y),
				2.89065409 * x*y*z,
				-0.457052559 * y*(-1 + 5 * z*z)
			);
			
		o_sh[i*4+3] +=
			vec4(		
				0.373181850	*z*(5 * z*z - 3),
				-0.457052559*x*(-1 + 5 * z*z),
				1.44532704	*(x*x - y * y)*z,
				-0.590052307*x*(x*x - 3 * y*y)
			);
		o_sh[i*4+0] *= weight;
		o_sh[i*4+1] *= weight;
		o_sh[i*4+2] *= weight;
		o_sh[i*4+3] *= weight;
	}
}
