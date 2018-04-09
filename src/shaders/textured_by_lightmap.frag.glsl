#version 300 es
precision highp float;

#include <common.glsl>

in vec2 v_lightmap_coord;

#include <scene_uniforms.glsl>

uniform sampler2D u_light_map;

uniform vec3 u_dir_light_color;
uniform vec3 u_dir_light_view_direction;

layout(location = 0) out vec4 o_color;

void main()
{
    vec2 coords = v_lightmap_coord;// /100.0 * 1024.0;
	o_color = vec4(texture(u_light_map, coords).rgb*2.0,1.0);

}
