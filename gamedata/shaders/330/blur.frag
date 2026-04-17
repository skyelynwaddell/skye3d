#version 430

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 direction;            // (1/w, 0) for horizontal, (0, 1/h) for vertical

// 9-tap gaussian weights (sigma ~= 4)
const float weights[5] = float[](0.2270270, 0.1945945, 0.1216216, 0.0540540, 0.0162162);

void main()
{
	vec3 result = texture(texture0, fragTexCoord).rgb * weights[0];

	for (int i = 1; i < 5; i++)
	{
		vec2 offset = direction * float(i);
		result += texture(texture0, fragTexCoord + offset).rgb * weights[i];
		result += texture(texture0, fragTexCoord - offset).rgb * weights[i];
	}

	finalColor = vec4(result, 1.0);
}
