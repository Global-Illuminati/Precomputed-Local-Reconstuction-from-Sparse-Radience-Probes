#version 300 es
precision highp float;

in vec2 v_tex_coord;

uniform int u_num_sh_coeffs_to_render;

uniform sampler2D u_relight_uvs_texture;
uniform sampler2D u_relight_shs_texture;
uniform sampler2D u_lightmap;

layout(location = 0) out vec4 o_color;

#define PI 3.1415926535897932384626433832795

void main()
{
    int num_rays = textureSize(u_relight_uvs_texture, 0).x;
    int num_probes = textureSize(u_relight_uvs_texture, 0).y;
    int num_sh_coeffs = textureSize(u_relight_shs_texture, 0).x;


    int sh_index = int(gl_FragCoord.x);
    int probe_index = int(gl_FragCoord.y);

    if (sh_index >= u_num_sh_coeffs_to_render) {
        o_color = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        vec4 summed_light = vec4(0.0,0.0,0.0,1.0);
        float num_hit_rays = 1.0;
        for (int ray_index = 0; ray_index < num_rays; ray_index++) {
            float sh_coeff_for_ray = texelFetch(u_relight_shs_texture, ivec2(sh_index, ray_index), 0).r;

            vec2 relight_ray_uv = texelFetch(u_relight_uvs_texture, ivec2(ray_index, probe_index), 0).rg;
            //@perf avoid branch, texture u_lightmap should be black if we set the texture to not repeat.
            if (relight_ray_uv.x != -1.0) {
                num_hit_rays += 1.0;
                summed_light += sh_coeff_for_ray * texture(u_lightmap, relight_ray_uv);
            }
        }
        vec4 resulting_sh_coeff = summed_light * 4.0*PI / float(num_rays);
        o_color = vec4(resulting_sh_coeff.rgb, 1.0);
    }


}

