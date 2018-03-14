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
	vec3 light_dir = light_pos-world_pos; 
	float sampled_light_dist = texture(shadow_map, -light_dir).r; // why the minus here?? 
	float real_distance = length(light_dir);
	float shadow = 0;
   if (real_distance < sampled_light_dist + 0.03)
        shadow = 1.0; 
    else
        shadow = 0.5; 
	o_color = vec4(vec3(packed_normal),1)*shadow;
}
