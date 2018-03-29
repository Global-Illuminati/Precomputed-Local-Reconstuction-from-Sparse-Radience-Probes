#version 420
precision highp float;

in vec3 world_pos;
uniform vec3 light_pos;
out vec4 o_color;

void main() {
	o_color = vec4(length(world_pos-light_pos));
}


