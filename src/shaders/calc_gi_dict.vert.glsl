#version 300 es
precision highp float;
precision highp int;

layout(location = 0) in ivec2 a_px_map;
layout(location = 1) in ivec4 a_dict_indices; //8 of int16s stored in a ivec4...
layout(location = 2) in vec4 a_dict_coeffs_03; 
layout(location = 3) in vec4 a_dict_coeffs_47; 

flat out ivec4 dict_indices;
flat out vec4 dict_coeffs_03;
flat out vec4 dict_coeffs_47;

void main()
{
	dict_indices = a_dict_indices;
	dict_coeffs_03 = a_dict_coeffs_03;
	dict_coeffs_47 = a_dict_coeffs_47;
	
	gl_Position = vec4((vec2(a_px_map)+0.5)*(2.0/1024.0)-1.0,0,1);
	gl_PointSize = 1.0;
}
