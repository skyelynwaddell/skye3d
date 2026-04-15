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

	// Baked lightmap is the primary static light source
	// fragColor carries the greyscale brightness sampled at each vertex
	vec3 lighting = fragColor.rgb;

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

	finalColor = texelColor * vec4(lighting, 1.0);

	// Gamma correction
	finalColor = pow(finalColor, vec4(1.0/2.2));
}
