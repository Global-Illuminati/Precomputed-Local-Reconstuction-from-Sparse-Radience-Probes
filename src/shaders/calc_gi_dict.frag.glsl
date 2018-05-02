#version 300 es
precision highp float;
precision highp int;

uniform sampler2D dictionary; 
flat in ivec4 dict_indices; //@cleanup, ugly, hacky, 8 ints in ivec4
flat in vec4 dict_coeffs_03;
flat in vec4 dict_coeffs_47;

out vec4 o_light;
void main()
{
	o_light = vec4(0.0);
	o_light.xyz += dict_coeffs_03.x * texelFetch(dictionary, ivec2(dict_indices.x & 0xffff, 0),0).xyz;
	o_light.xyz += dict_coeffs_03.y * texelFetch(dictionary, ivec2(dict_indices.x >> 16   , 0),0).xyz;
	o_light.xyz += dict_coeffs_03.z * texelFetch(dictionary, ivec2(dict_indices.y & 0xffff, 0),0).xyz;
	o_light.xyz += dict_coeffs_03.w * texelFetch(dictionary, ivec2(dict_indices.y >> 16   , 0),0).xyz;

	o_light.xyz += dict_coeffs_47.x * texelFetch(dictionary, ivec2(dict_indices.z & 0xffff, 0),0).xyz;
	o_light.xyz += dict_coeffs_47.y * texelFetch(dictionary, ivec2(dict_indices.z >> 16   , 0),0).xyz;
	o_light.xyz += dict_coeffs_47.z * texelFetch(dictionary, ivec2(dict_indices.w & 0xffff, 0),0).xyz;
	o_light.xyz += dict_coeffs_47.w * texelFetch(dictionary, ivec2(dict_indices.w >> 16   , 0),0).xyz;
	o_light.a = 1.0;
}
