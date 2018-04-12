#version 420
precision highp float;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_lm_uv;


out vec3 position;
out vec3 normal;

void main()
{
	position = in_position;
	normal = in_normal;
	gl_Position = vec4(in_lm_uv/1024.0*2.0-1.0,0,1);
}
