#version 330

in vec3 vBary;

out vec4 fragColor;

uniform vec4 wireColor;
uniform float lineWidth;

void main()
{
    float edge = min(min(vBary.x, vBary.y), vBary.z);

    float line = smoothstep(0.0, lineWidth, edge);

    fragColor = vec4(wireColor.rgb, 1.0 - line);
}