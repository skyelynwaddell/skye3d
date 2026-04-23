#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec4 lightColor;
uniform vec3 colDiffuse;
uniform vec3 colAmbient;
uniform sampler2D texture0;
uniform float ambientStrength;

// Flashlight Uniforms
uniform vec3 flashPos;
uniform vec3 flashDir;
uniform vec4 flashColor;
uniform float flashIntensity;
uniform float flashRange;
uniform float flashConeAngle;
uniform int flashEnabled;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 worldLightDir = normalize(lightPos - fragPosition);
    
    vec4 texColor = texture(texture0, fragTexCoord);
    if (texColor.a < 0.1) texColor = fragColor;
    
    // --- 1. DARK AMBIENT ---
    float ao = 0.5 + 0.5 * normal.y;
    vec3 ambient = colAmbient * (ambientStrength * 0.15) * (0.6 + 0.4 * ao);
    
    // --- 2. STANDARD WORLD LIGHT ---
    float diff = max(dot(normal, worldLightDir), 0.0);
    vec3 diffuse = colDiffuse * diff * vec3(lightColor);
    
    vec3 totalLighting = ambient + diffuse;

    // --- 3. FLASHLIGHT IMPACT ---
    if (flashEnabled == 1) {
        vec3 toFrag = fragPosition - flashPos;
        float dist = length(toFrag);
        
        if (dist < flashRange) {
            vec3 fLightDir = normalize(toFrag);
            vec3 fLookDir  = normalize(flashDir);
            float cosAngle = dot(fLightDir, fLookDir);
            
            // Soft edge calculation
            float innerThreshold = cos(radians(flashConeAngle));
            float outerThreshold = cos(radians(flashConeAngle + 12.0));
            float edgeFade = clamp((cosAngle - outerThreshold) / (innerThreshold - outerThreshold), 0.0, 1.0);

            if (edgeFade > 0.0) {
                float distFalloff = clamp(1.0 - (dist / flashRange), 0.0, 1.0);
                distFalloff *= distFalloff; 
                
                // Diffuse interaction only
                float fNdotL = clamp(dot(normal, -fLightDir), 0.0, 1.0);
                
                // Color scaling (0.033 for your 30.0 color value)
                vec3 flashContribution = (flashColor.rgb * 0.5) * flashIntensity * edgeFade * distFalloff * (fNdotL * 0.9 + 0.1);
                totalLighting += flashContribution;
            }
        }
    }

    // --- 4. RIM LIGHT (Keeping this as a subtle "shape definer") ---
    // If you want it even flatter, you can set rimColor to vec3(0.0)
    float rim = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
    vec3 rimColor = vec3(0.1, 0.15, 0.2) * rim * 0.1;

    // --- 5. FINAL COMBINE ---
    // Combined without the specular component
    vec3 result = (totalLighting * texColor.rgb) + rimColor;
    
    // Contrast boost
    result = pow(result, vec3(1.15));

    finalColor = vec4(result, texColor.a);
}