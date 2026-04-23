#version 330 core

in vec3 fragPosition; 
in vec2 fragTexCoord;
in vec3 fragNormal;   
out vec4 fragColor;

uniform sampler2D texture0;
uniform float time;
uniform float wave_speed = 1.0;

// Flashlight Uniforms
uniform vec3 flashPos;
uniform vec3 flashDir;
uniform vec4 flashColor;
uniform float flashIntensity;
uniform float flashRange;
uniform float flashConeAngle;
uniform int flashEnabled;

const float PI = 3.14159265359;

void main() {
    vec2 uv = fragTexCoord;
    
    // 1. WAVE DISTORTION (Keep your original movement)
    float waveOffset = sin(uv.y * wave_speed * PI + time) * cos(uv.x * wave_speed * PI + time) * 0.05;
    uv += waveOffset;

    // Get the actual texture color
    vec4 texelColor = texture(texture0, uv);

    // 2. AMBIENT LIGHT (The "Darkness" level)
    // We use a small multiplier of the texture color so it's visible but dark
    vec3 lighting = vec3(0.35); 

    // 3. FLASHLIGHT LOGIC
    if (flashEnabled == 1) {
        vec3 toFrag = fragPosition - flashPos;
        float dist = length(toFrag);
        
        if (dist < flashRange) {
            vec3 lightDir = normalize(toFrag);
            vec3 lookDir  = normalize(flashDir);

            float cosAngle = dot(lightDir, lookDir);
            
            // Soft edges for the flashlight circle
            float innerThreshold = cos(radians(flashConeAngle));
            float outerThreshold = cos(radians(flashConeAngle + 15.0));
            float edgeFade = clamp((cosAngle - outerThreshold) / (innerThreshold - outerThreshold), 0.0, 1.0);

            if (edgeFade > 0.0) {
                float distFalloff = clamp(1.0 - (dist / flashRange), 0.0, 1.0);
                distFalloff *= distFalloff; 

                // Use the surface normal to catch "glints" from the waves
                vec3 waveNormal = normalize(vec3(waveOffset * 5.0, 1.0, waveOffset * 5.0));
                float NdotL = clamp(dot(waveNormal, -lightDir), 0.0, 1.0);
                
                // Calculate how much extra brightness to add
                // (Using 0.033 to balance your high 30.0 color value)
                float flashBrightness = flashIntensity * edgeFade * distFalloff * (NdotL * 0.8 + 0.2);
                vec3 flashContribution = (flashColor.rgb * 0.5) * flashBrightness;
                
                lighting += flashContribution;
            }
        }
    }

    // 4. FINAL OUTPUT
    // Multiply the original texture color by our lighting factor
    // This ensures it never turns "blue" unless the texture itself is blue
    vec3 finalRGB = texelColor.rgb * lighting;

    fragColor = vec4(finalRGB, texelColor.a);
}