#ifndef COMMON_GLSL
#define COMMON_GLSL

#define PI     (3.14159265358979323846)
#define TWO_PI (2.0 * PI)

float saturate(in float value) {
	return clamp(value, 0.0, 1.0);
}

void reortogonalize(in vec3 v0, inout vec3 v1)
{
	// Perform Gram-Schmidt's re-ortogonalization process to make v1 orthagonal to v1
	v1 = normalize(v1 - dot(v1, v0) * v0);
}

vec2 spherical_from_direction(vec3 direction)
{
	highp float theta = acos(clamp(direction.y, -1.0, 1.0));
	highp float phi = atan(direction.z, direction.x);
	if (phi < 0.0) phi += TWO_PI;

	return vec2(phi / TWO_PI, theta / PI);
}

float sample_shadow_map_pcf(in sampler2DShadow shadow_map, in vec3 shadow_coord, vec2 texel_size, in float bias)
{
	float shadow =0.0;
	float kernel_size = 3.0;

	float side = (kernel_size-1.0)/2.0;
	{
		for (float y = -side; y <= side; y+=1.0)
			for (float x = -side; x <= side; x+=1.0)
				shadow += texture(shadow_map, shadow_coord + vec3(x*texel_size.x,y*texel_size.y,-0.001));
		shadow /= (kernel_size*kernel_size);
	}
	return shadow;
}

//from: https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
vec3 toLinear(vec3 sRGB)
{
	//sRGB = mat3(3.2406,-1.5372,-0.4986,0.9689,1.8759,0.0415,0.0557,-0.2040,1.0570)*sRGB;
    bvec3 cutoff = lessThan(sRGB, vec3(0.04045));
    vec3 higher = pow((sRGB + vec3(0.055))*vec3(1./1.055), vec3(2.4));
    vec3 lower = sRGB*vec3(1./12.92);

    return mix(higher, lower, cutoff);
}

//from: https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
vec3 fromLinear(vec3 linearRGB)
{
    bvec3 cutoff = lessThan(linearRGB, vec3(0.0031308));
    vec3 higher = vec3(1.055)*pow(linearRGB, vec3(1.0/2.4)) - vec3(0.055);
    vec3 lower = linearRGB * vec3(12.92);
	//mat3(0.4124,0.3576,0.1805,0.2126,0.7152,0.0722,0.0193,0.1192,0.9505)*
    return mix(higher, lower, cutoff);
}
#endif // COMMON_GLSL

