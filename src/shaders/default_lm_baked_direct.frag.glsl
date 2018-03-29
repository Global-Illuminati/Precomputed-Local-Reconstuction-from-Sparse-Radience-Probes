#version 300 es
precision highp float;



//
// NOTE: All fragment calculations are in *view space*
//

in vec3 v_position;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_tex_coord;
in vec2 v_lightmap_coord;
in vec4 v_light_space_position;

#include <scene_uniforms.glsl>

uniform sampler2D u_diffuse_map;
uniform sampler2D u_light_map;
uniform sampler2D u_baked_direct;

layout(location = 0) out vec4 o_color;

void main()
{
	vec3 diffuse = texture(u_diffuse_map, v_tex_coord).rgb;
    vec3 color  = texture(u_light_map,v_lightmap_coord).xyz*diffuse + texture(u_baked_direct,v_lightmap_coord).xyz;
	o_color = vec4(color, 1.0);

}
