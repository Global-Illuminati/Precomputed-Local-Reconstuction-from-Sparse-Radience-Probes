#version 300 es

#include <mesh_attributes.glsl>
layout(location = 10) in vec3 a_translation;

uniform mat4 u_projection_from_world;

out vec3 v_dir;
flat out int v_probe_index;

void main()
{
    v_probe_index = gl_InstanceID;
    v_dir = a_position;
	vec3 translated_position = a_position + a_translation;
//	vec3 translated_position = a_position + vec3(0.0,1.0,0.0)*float(v_probe_index); // Use this to render them in a single line for easier comparison
	gl_Position = u_projection_from_world * vec4(translated_position, 1.0);
}
