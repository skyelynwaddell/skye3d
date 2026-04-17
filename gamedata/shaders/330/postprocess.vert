#version 430

in vec3 vertexPosition;
in vec2 vertexTexCoord;

out vec2 fragTexCoord;

uniform mat4 mvp;

void main()
{
	fragTexCoord = vertexTexCoord;
	gl_Position = mvp * vec4(vertexPosition, 1.0);
}
