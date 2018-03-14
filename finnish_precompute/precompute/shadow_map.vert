#version 330

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_matrix;

out vec3 world_pos;

void main() {
	gl_Position = u_matrix * vec4(a_position, 1.0);
	world_pos = a_position;
}
	