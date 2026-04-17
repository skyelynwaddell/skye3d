#version 330 core

in vec3 fragPosition;

out vec4 fragColor;

uniform sampler2D tex;
uniform float     time;
uniform vec3      cameraPos;

vec3 SampleLayer(vec2 uv)
{
    vec2 l1 = uv + vec2(time * 0.012, time * 0.007);
    vec2 l2 = uv * 1.4 + vec2(0.5, 0.5) - vec2(time * 0.009, time * 0.013);
    return mix(texture(tex, l1).rgb, texture(tex, l2).rgb, 0.5);
}

void main()
{
    // Normalized direction from camera — removes distance, sky stays fixed as you walk
    vec3 dir = normalize(fragPosition - cameraPos);

    // Blend weights from VIEW DIRECTION, not face normal.
    // This is the key: adjacent faces with different normals but seen from the
    // same direction will share the same blend weights, so their UV projections
    // match and the seam line at the brush edge disappears.
    vec3 blend = pow(abs(dir), vec3(4.0));
    blend /= dot(blend, vec3(1.0));

    // Scale — larger value = more tiles visible = repeats are less obvious
    float scale = 0.3;
    vec2 uvX = dir.zy * scale;
    vec2 uvY = dir.xz * scale;
    vec2 uvZ = dir.xy * scale;

    vec3 col = SampleLayer(uvX) * blend.x
             + SampleLayer(uvY) * blend.y
             + SampleLayer(uvZ) * blend.z;

    fragColor = vec4(col * 1.2, 1.0);
}
