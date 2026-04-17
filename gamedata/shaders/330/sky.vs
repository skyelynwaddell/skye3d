#version 330 core

in vec3 vertexPosition;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position  = mvp * vec4(vertexPosition, 1.0);
}
