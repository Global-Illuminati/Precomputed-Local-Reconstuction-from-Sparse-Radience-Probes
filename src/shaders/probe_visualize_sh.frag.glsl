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
#if 0 
// hardcoded shs
    float Y[16];
    Y[0] = 0.282098949
    Y[1] = -0.488609731* y
    Y[2] = 0.488609731 * z
    Y[3] = -0.488609731* x
    Y[4] = 1.09256458	 * y*x
    Y[5] = -1.09256458 * y*z
    Y[6] = 0.315396219 * (3.0 * z*z - 1.0)
    Y[7] = -1.09256458 * x*z
    Y[8] = 0.546282291 * (x*x - y * y)
    Y[9] = -0.590052307* y*(3.0 * x*x - y * y)
    Y[10] = 2.89065409  * x*y*z
    Y[11] = -0.457052559* y*(-1.0 + 5.0 * z*z)
    Y[12] = 0.373181850 * z*(5.0 * z*z - 3.0)
    Y[13] = -0.457052559* x*(-1.0 + 5.0 * z*z)
    Y[14] = 1.44532704	 * (x*x - y * y)*z
    Y[15] = -0.590052307*x*(x*x - 3.0 * y*y);
  #else
  #define PVT(l,m) ((m)+((l)*((l)+1))/2)
  #define YVR(l,m) ((m)+(l)+((l)*(l)))

    const int L = 7;
  //generate shs
    const int sizeP = (L + 1) * (L + 2) / 2;
    float P[sizeP];
    float A[sizeP];
    float B[sizeP];
    float Y[(L + 1)*(L + 1)];

    for (int l = 2; l <= L; l++) {
      float ls = float(l * l);
      float lm1s = float((l - 1) * (l - 1));
      for (int m = 0; m < l - 1; m++) {
        float ms = float(m * m);
        A[PVT(l, m)] = sqrt((4. * ls - 1.) / (ls - ms));
        B[PVT(l, m)] = -sqrt((lm1s - ms) / (4. * lm1s - 1.));
      }
    }

    float sintheta = sqrt(1. - z * z);
    if (z == 1.) sintheta = 0.0;
    float temp = 0.39894228040143267794;  // = sqrt(0.5/M_PI)
    P[PVT(0, 0)] = temp;

    const float SQRT3 = 1.7320508075688772935;
    P[PVT(1, 0)] = z * SQRT3 * temp;
    const float SQRT3DIV2 = -1.2247448713915890491;
    temp = SQRT3DIV2 * sintheta * temp;
    P[PVT(1, 1)] = temp;

    for (int l = 2; l <= L; l++) {
      for (int m = 0; m < l - 1; m++) {
        P[PVT(l, m)] = A[PVT(l, m)]
          * (z * P[PVT(l - 1, m)]
            + B[PVT(l, m)] * P[PVT(l - 2, m)]);
      }
      P[PVT(l, l - 1)] = z * sqrt(2. * (float(l) - 1.) + 3.) * temp;
      temp = -sqrt(1.0 + 0.5 / float(l)) * sintheta * temp;
      P[PVT(l, l)] = temp;
    }

    for (int l = 0; l <= L; l++)
      Y[YVR(l, 0)] = P[PVT(l, 0)] * 0.5 * sqrt(2.);
    if (sintheta == 0.) {
      for (int m = 1; m <= L; m++) {
        for (int l = m; l <= L; l++) {
          Y[YVR(l, -m)] = 0.;
          Y[YVR(l, m)] = 0.;
        }
      }
    } else {
      float c1 = 1.0, c2 = x / sintheta;
      float s1 = 0.0, s2 = -y / sintheta;
      float tc = 2.0 * c2;
      for (int m = 1; m <= L; m++) {
        float s = tc * s1 - s2;
        float c = tc * c1 - c2;
        s2 = s1;
        s1 = s;
        c2 = c1;
        c1 = c;
        for (int l = m; l <= L; l++) {
          Y[YVR(l, -m)] = P[PVT(l, m)] * s;
          Y[YVR(l, m)] = P[PVT(l, m)] * c;
        }
      }
    }
#endif




    o_color = vec4(0);
    for(int i  = 0; i<u_num_sh_coeffs_to_render;i++){
      o_color += texelFetch(u_probe_sh_texture, ivec2(i,  v_probe_index), 0) * Y[i];
    }
    o_color = vec4(o_color.rgb, 1.0);

}
