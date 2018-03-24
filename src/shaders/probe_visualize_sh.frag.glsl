#version 300 es
precision lowp float;

in vec3 v_dir;
flat in int v_probe_index;

uniform sampler2D u_probe_sh_texture;

uniform int u_num_sh_coeffs_to_render;

layout(location = 0) out vec4 o_color;

void main()
{
    vec3 dir = normalize(v_dir);
    o_color = vec4(dir, 1.0);

    float x = dir.x;
    float y = dir.y;
    float z = dir.z;

    int i = v_probe_index;

    float c = 1.0; // Added this multiplication factor to make the differences visible, they all look the same otherwise...

    // Maybe something is wrong with the SH expressions below?

    o_color =

      (0 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(0,  i), 0) * 0.282098949
    + (1 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(1,  i), 0) * -0.488609731* y
    + (2 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(2,  i), 0) * 0.488609731 * z
    + (3 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(3,  i), 0) * -0.488609731* x

    + (4 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(4,  i), 0) * 1.09256458	 * y*x
    + (5 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(5,  i), 0) * -1.09256458 * y*z
    + (6 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(6,  i), 0) * 0.315396219 * (3.0 * z*z - 1.0)
    + (7 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(7,  i), 0) * -1.09256458 * x*z

    + (8 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(8,  i), 0) * 0.546282291 * (x*x - y * y)
    + (9 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(9,  i), 0) * -0.590052307* y*(3.0 * x*x - y * y)
    + (10 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(10, i), 0) * 2.89065409  * x*y*z
    + (11 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(11, i), 0) * -0.457052559* y*(-1.0 + 5.0 * z*z)

    + (12 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(12, i), 0) * 0.373181850 * z*(5.0 * z*z - 3.0)
    + (13 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(13, i), 0) * -0.457052559* x*(-1.0 + 5.0 * z*z)
    + (14 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(14, i), 0) * 1.44532704	 * (x*x - y * y)*z
    + (15 < u_num_sh_coeffs_to_render ? 1.0 : 0.0) * c*texelFetch(u_probe_sh_texture, ivec2(15, i), 0) * -0.590052307*x*(x*x - 3.0 * y*y);

    o_color = vec4(o_color.rgb, 1.0);

}
