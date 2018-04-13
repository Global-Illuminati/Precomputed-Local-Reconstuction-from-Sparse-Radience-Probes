#version 300 es
precision highp float;
precision highp int;

layout(location = 0) in ivec2 a_px_map;
layout(location = 1) in ivec4 a_probe_indices; //8 of int16s stored in a ivec4...

flat out int receiver_index;
flat out ivec4 probe_indices;
void main()
{
	probe_indices = a_probe_indices;
	receiver_index = gl_VertexID;
	gl_Position = vec4((vec2(a_px_map)+0.5)*(2.0/1024.0)-1.0,0,1);
	gl_PointSize = 1.0;
}
