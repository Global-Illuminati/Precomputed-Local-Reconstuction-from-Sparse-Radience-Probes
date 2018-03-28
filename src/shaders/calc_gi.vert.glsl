#version 300 es
precision highp float;
precision highp int;


layout(location = 0) in ivec2 a_px_map;
flat out int receiver_index;
void main()
{
	receiver_index = gl_VertexID;
	gl_Position = vec4((vec2(a_px_map)+vec2(0.5))*(2.0/1024.0)-1.0,0,1);
	gl_PointSize = 1.0/1024.0;
}
