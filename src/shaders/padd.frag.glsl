#version 300 es
precision highp float;

uniform sampler2D u_texture;
layout(location = 0) out vec4 o_color;

void main()
{
    ivec2 px = ivec2(gl_FragCoord.xy);
    vec4 col = texelFetch(u_texture,px,0);
    if(col.a == 1.0) {o_color = col; return;}

    float num_non_zero = 0.0;
    for(int x = -1; x <= 1;x++)
    for(int y = -1; y <= 1;y++){
        ivec2 off = ivec2(x,y);
        vec4 col = texelFetch(u_texture,px+off,0);
        if(col.a == 1.0)
        {
            o_color += col;
            num_non_zero += 1.0;
        }
    }
    o_color /= num_non_zero;
}
