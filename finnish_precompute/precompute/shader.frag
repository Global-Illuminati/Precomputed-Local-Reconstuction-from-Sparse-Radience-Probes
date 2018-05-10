#version 420
precision highp float;
#if 1
in vec3 world_pos;
in vec3 normal;
in vec2 lightmap_uv;
layout(location = 0) out vec4 o_color;
layout(binding = 0) uniform samplerCube shadow_map;
layout(binding = 1) uniform sampler2D positions;
layout(binding = 2) uniform sampler2D normals;


uniform vec3 light_pos;

void main()
{
	vec3 packed_normal = normal * vec3(0.5) + vec3(0.5);
	vec3 light_dir = world_pos-light_pos; 

	float sampled_light_dist = texture(shadow_map, light_dir).r; 
	float real_distance = length(light_dir);
	float vis = 0;
   if (real_distance < sampled_light_dist + 0.01)
        vis= 1.0; 
    else
        vis= 0.0; 
		

	o_color = vec4(vec3(packed_normal),1)*(vis);
	//o_color = vec4(0.0,0.0,0.0,1.0);
	//o_color = vec4(length(light_dir/200));
	//o_color = vec4(sampled_light_dist);
	//o_color = vec4(vec3(texture(shadow_map, light_dir).r),1.0);
	//o_color = texture(shadow_map, light_dir)/30.0;
}
#else

layout(binding = 0) uniform sampler2D p_tex;
layout(binding = 1) uniform sampler2D n_tex;
layout(location = 0) out vec4 o_color;

void main()
{
	vec3 v = texture(p_tex,gl_FragCoord.xy/512.0).xyz;
	vec3 pv = v * 0.5 + 0.5;
	o_color = vec4(pv,1);
}

#endif
