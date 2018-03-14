#version 420
precision highp float;

in vec3 world_pos;
uniform vec3 light_pos;
layout(location = 0) out float o_color;

void main() {
	o_color = length(light_pos-world_pos);
}


