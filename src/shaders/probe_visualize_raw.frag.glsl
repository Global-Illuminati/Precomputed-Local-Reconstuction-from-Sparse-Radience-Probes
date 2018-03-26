#version 300 es
precision lowp float;

in vec3 v_dir;
flat in int v_probe_index;

uniform sampler2D u_relight_uvs_texture;
uniform sampler2D u_relight_dirs_texture;
uniform sampler2D u_lightmap;

layout(location = 0) out vec4 o_color;

int closest_relight_ray(vec3 dir) {
    int num_rays = textureSize(u_relight_uvs_texture, 0).x;
    int closest_index = 0;
    float closest_dist = 2.0;
    for (int i = 0; i < num_rays; i++) {
        vec3 ray = texelFetch(u_relight_dirs_texture, ivec2(i, 0), 0).rgb;
        float dist = distance(dir, ray);
        if (dist < closest_dist) {
            closest_dist = dist;
            closest_index = i;
        }
    }
    return closest_index;
}

void main()
{
    vec3 dir = normalize(v_dir);
    int ray_index = closest_relight_ray(dir);
    vec2 relight_ray_uv = texelFetch(u_relight_uvs_texture, ivec2(ray_index, v_probe_index), 0).rg;
    vec4 lookedup_light = vec4(texture(u_lightmap, relight_ray_uv).rgb,1);
    o_color = lookedup_light;

    if (relight_ray_uv.x == -1.0) {
        o_color = vec4(1.0, 0.0, 1.0, 1.0); // Make missed rays magenta-colored
    }
    
//    o_color = vec4(float(ray_index) / 100.0, 0.0, 0.0, 1.0);
}
