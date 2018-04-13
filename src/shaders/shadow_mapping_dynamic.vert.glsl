#version 300 es

#include <mesh_attributes_dynamic.glsl>

uniform mat4 u_world_from_local;
uniform mat4 u_light_projection_from_world;

void main()
{
	gl_Position = u_light_projection_from_world * u_world_from_local * vec4(a_position, 1.0);
}
