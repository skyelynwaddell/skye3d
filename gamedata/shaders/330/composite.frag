#version 430

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;        // original scene
uniform sampler2D texture1;        // blurred bloom

uniform float bloomIntensity;      // how much bloom to add (default 0.8)
uniform float exposure;            // exposure multiplier  (default 1.2)
uniform float saturation;          // color saturation     (default 1.2)
uniform float warmth;              // warm tint strength   (default 0.15)
uniform float vignetteStrength;    // edge darkening       (default 0.3)

// ACES filmic tone mapping (industry standard, nice S-curve)
vec3 ACESFilm(vec3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	vec3 scene = texture(texture0, fragTexCoord).rgb;
	vec3 bloom = texture(texture1, fragTexCoord).rgb;

	// Add bloom
	vec3 color = scene + bloom * bloomIntensity;

	// Exposure
	color *= exposure;

	// Warm color tint — push toward amber in bright areas
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	vec3 warm = vec3(1.0, 0.9, 0.75);
	color = mix(color, color * warm, warmth * smoothstep(0.1, 0.8, luma));

	// ACES tone mapping
	color = ACESFilm(color);

	// Saturation boost
	vec3 grey = vec3(dot(color, vec3(0.2126, 0.7152, 0.0722)));
	color = mix(grey, color, saturation);

	// Vignette — darken edges
	vec2 uv = fragTexCoord * 2.0 - 1.0;
	float vignette = 1.0 - dot(uv, uv) * vignetteStrength;
	color *= clamp(vignette, 0.0, 1.0);

	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	finalColor = vec4(color, 1.0);
}
