#version 300 es
precision highp float;

in vec2 v_tex_coord;

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

    vec4 summed_light = vec4(0.0,0.0,0.0,1.0);
    for (int ray_index = 0; ray_index < num_rays; ray_index++) {
        float sh_coeff_for_ray = texelFetch(u_relight_shs_texture, ivec2(sh_index, ray_index), 0).r;

        vec2 relight_ray_uv = texelFetch(u_relight_uvs_texture, ivec2(ray_index, probe_index), 0).rg;
        vec4 lookedup_light = texture(u_lightmap, relight_ray_uv);

        if (relight_ray_uv.x == -1.0) {
            //lookedup_light = vec4(1.0, 0.0, 1.0, 1.0); // Make missed rays magenta-colored
            lookedup_light = vec4(0.0, 0.0, 0.0, 1.0); // Make missed rays black 
        }

        summed_light += sh_coeff_for_ray * lookedup_light;
    }
    vec4 resulting_sh_coeff = summed_light * 4.0*PI / float(num_rays);
    o_color = vec4(resulting_sh_coeff.rgb, 1.0);
}

