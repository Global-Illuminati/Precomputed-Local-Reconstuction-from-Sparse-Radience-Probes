#version 420
precision highp float;

in vec3 world_pos;
in vec3 normal;

layout(location = 0) out vec4 o_color;
layout(binding = 0) uniform samplerCube shadow_map;

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
	//o_color = vec4(sampled_light_dist*200);
	//o_color = vec4(vec3(texture(shadow_map, light_dir).r),1.0);
	//o_color = texture(shadow_map, light_dir)/30.0;
}
