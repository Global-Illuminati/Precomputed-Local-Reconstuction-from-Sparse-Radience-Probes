#version 420
precision highp float;

in vec3 in_position;
in vec3 in_normal;

layout(location = 0) out vec3 position; 
layout(location = 1) out vec3 normal; 

void main()
{
	position = in_position;
	normal = in_normal;
}

