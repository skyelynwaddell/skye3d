#version 430

// 1. INPUT ATTRIBUTES
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// 2. UNIFORMS
uniform sampler2D texture0;
uniform vec3 viewPos;
uniform int lightPower;

// Flashlight uniforms
uniform vec3 flashPos;
uniform vec3 flashDir;
uniform vec4 flashColor;
uniform float flashIntensity;
uniform float flashRange;
uniform float flashConeAngle;
uniform int flashEnabled;

// 3. LIGHTING CONSTANTS/STRUCTS
#define MAX_LIGHTS 4
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};
uniform Light lights[MAX_LIGHTS];

// 4. HELPER FUNCTIONS
float Attenuate(float dist) {
    float HD = float(lightPower);
    float K = 1.0 / (HD * HD + 0.0001);
    return 1.0 / (1.0 + K * dist * dist);
}

// 5. OUTPUT
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 normal = normalize(fragNormal);

    // Ambient baseline (stops pure black areas)
    vec3 lighting = max(fragColor.rgb * 2.0, vec3(0.01));

    // --- Standard Lights Loop ---
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i].enabled == 1) {
            vec3 light_dir;
            float light_dist;

            if (lights[i].type == LIGHT_DIRECTIONAL) {
                light_dir = normalize(lights[i].target - lights[i].position);
                light_dist = distance(lights[i].target, lights[i].position);
            } else {
                light_dir = normalize(fragPosition - lights[i].position);
                light_dist = distance(fragPosition, lights[i].position);
            }

            float NdotL = clamp(abs(dot(normal, -light_dir)), 0.0, 1.0);
            lighting += lights[i].color.rgb * NdotL * Attenuate(light_dist);
        }
    }

    if (flashEnabled == 1) {
        vec3 toFrag = fragPosition - flashPos;
        float dist = length(toFrag);
        
        if (dist < flashRange) {
            vec3 lightDir = normalize(toFrag);
            vec3 lookDir  = normalize(flashDir);

            // 1. Calculate how aligned the pixel is with our look direction
            float cosAngle = dot(lightDir, lookDir);
            
            // 2. Define the "Ease" zone
            // inner = full brightness starts here
            // outer = light hits zero here (we add 10 degrees for a soft edge)
            float innerThreshold = cos(radians(flashConeAngle));
            float outerThreshold = cos(radians(flashConeAngle + 15.0));

            // 3. Calculate intensity based on the angle
            // This replaces the "if (cosAngle > threshold)" with a smooth 0.0 to 1.0 gradient
            float edgeFade = clamp((cosAngle - outerThreshold) / (innerThreshold - outerThreshold), 0.0, 1.0);

            if (edgeFade > 0.0) {
                // 4. Distance Falloff (Squared makes the fade into the distance feel more natural)
                float distFalloff = clamp(1.0 - (dist / flashRange), 0.0, 2.0);
                distFalloff *= distFalloff; 
                
                // 5. Surface Normal (NdotL)
                float NdotL = clamp(dot(normal, -lightDir), 0.0, 1.0);
                
                // 6. Combine everything
                // We use edgeFade to smooth the circle and distFalloff to smooth the range
                float totalIntensity = edgeFade * distFalloff * (NdotL * 0.9 + 0.1);
                
                lighting += (flashColor.rgb * flashIntensity * totalIntensity);
            }
        }
    }

    finalColor = vec4(texelColor.rgb * lighting, texelColor.a);
}