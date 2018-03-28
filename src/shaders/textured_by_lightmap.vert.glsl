#version 300 es

#include <mesh_attributes.glsl>
#include <scene_uniforms.glsl>

uniform mat4 u_world_from_local;
uniform mat4 u_view_from_world;
uniform mat4 u_projection_from_view;

out vec2 v_lightmap_coord;

void main()
{
	mat4 view_from_local = u_view_from_world * u_world_from_local;

	vec4 view_space_position = view_from_local * vec4(a_position, 1.0);
	vec4 view_space_normal   = view_from_local * vec4(a_normal, 0.0);
	vec4 view_space_tangent  = view_from_local * vec4(a_tangent.xyz, 0.0);

	v_lightmap_coord = a_lightmap_coord;//a_lightmap_coord;

	gl_Position = u_projection_from_view * view_space_position;

}
