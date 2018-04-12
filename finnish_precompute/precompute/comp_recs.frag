#version 420
precision highp float;

in vec3 position;
in vec3 normal;

layout(location = 0) out vec3 out_position; 
layout(location = 1) out vec3 out_normal; 

void main()
{
	out_position = position;
	out_normal = normal;
}

