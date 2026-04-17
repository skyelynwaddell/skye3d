#version 430

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;        // baked lightmap brightness interpolated across the triangle

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light
{
	int enabled;
	int type;
	vec3 position;
	vec3 target;
	vec4 color;
};

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;
uniform int lightPower;

float
Attenuate(float dist)
{
	float HD = lightPower;
	float K = 1.0 / (HD * HD);
	float dsq = dist * dist;
	return 1.0 / (1.0 + K*dsq);
}

void main()
{
	// Texel color fetching from texture sampler
	vec4 texelColor = texture(texture0, fragTexCoord);
	vec3 normal = normalize(fragNormal);

	// Baked lightmap
	float lm = fragColor.r;
	vec3 lighting = vec3(lm) * 2.5;

	// Tint the lightmap warm — brighter areas get more amber
	vec3 warmLight = vec3(1.0, 0.75, 0.5);        // amber tint
	vec3 coolLight = vec3(0.7, 0.8, 1.0);          // blueish shadow tint
	vec3 lightTint = mix(coolLight, warmLight, smoothstep(0.0, 0.6, lm));
	lighting *= lightTint;

	// Dynamic lights are additive on top of the baked lightmap
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		if (lights[i].enabled == 1)
		{
			vec3 light_direction = vec3(0);
			float light_distance = 0;

			if (lights[i].type == LIGHT_DIRECTIONAL)
			{
				light_direction = normalize(lights[i].target - lights[i].position);
				light_distance = distance(lights[i].target, lights[i].position);
			}
			else if (lights[i].type == LIGHT_POINT)
			{
				light_direction = normalize(fragPosition - lights[i].position);
				light_distance = distance(fragPosition, lights[i].position);
			}

			float NdotL = abs(dot(normal, light_direction)) * Attenuate(light_distance);
			lighting += lights[i].color.rgb * NdotL;
		}
	}

	// Base lit color
	vec3 litColor = texelColor.rgb * lighting;

	// Fake bloom — bright areas bleed warm light
	float brightness = dot(litColor, vec3(0.2126, 0.7152, 0.0722));
	float bloom = max(brightness - 0.3, 0.0) * 0.7;
	litColor += bloom * vec3(1.0, 0.7, 0.4);      // strong amber bloom

	// Glow boost — bright spots get extra intensity
	float glow = smoothstep(0.3, 1.0, brightness);
	litColor = mix(litColor, litColor * 1.5, glow);

	// Ambient — warm-tinted so shadows feel atmospheric, not dead grey
	litColor += texelColor.rgb * vec3(0.06, 0.04, 0.03);

	// Color grade — push midtones warm, increase saturation
	vec3 grey = vec3(dot(litColor, vec3(0.2126, 0.7152, 0.0722)));
	litColor = mix(grey, litColor, 1.25);          // boost saturation 25%

	finalColor = vec4(litColor, texelColor.a);

	// Tone mapping (Reinhard) — keeps brights from clipping harshly
	finalColor.rgb = finalColor.rgb / (finalColor.rgb + vec3(1.0));

	// Gamma correction
	finalColor = pow(finalColor, vec4(1.0/2.2));
}