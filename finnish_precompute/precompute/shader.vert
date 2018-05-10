#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_lightmap_uv;

uniform mat4 matrix;

out vec3 world_pos;
out vec3 normal;
out vec2 lightmap_uv;
void main()
{
	vec4 transformed_vert = matrix*vec4(in_position, 1.0) ;
	world_pos = in_position;
	normal    = in_normal;
	lightmap_uv = in_lightmap_uv/1024.0;
	gl_Position = transformed_vert;
}
