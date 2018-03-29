#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

uniform mat4 matrix;

out vec3 world_pos;
out vec3 normal;
void main()
{
	vec4 transformed_vert = matrix*vec4(in_position, 1.0) ;
	world_pos = in_position;
	normal    = in_normal;
	gl_Position = transformed_vert;
}
