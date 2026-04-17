#version 430

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;        // scene color
uniform float bloomThreshold;      // brightness cutoff (default 0.7)
uniform float bloomKnee;           // soft knee width  (default 0.3)

void main()
{
	vec3 color = texture(texture0, fragTexCoord).rgb;
	float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

	// Soft knee — smoothly ramp into bloom instead of hard cutoff
	float soft = brightness - bloomThreshold + bloomKnee;
	soft = clamp(soft / (2.0 * bloomKnee + 0.0001), 0.0, 1.0);
	soft = soft * soft;

	float contribution = max(soft, step(bloomThreshold, brightness));
	finalColor = vec4(color * contribution, 1.0);
}
