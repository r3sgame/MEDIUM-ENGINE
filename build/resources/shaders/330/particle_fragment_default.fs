#version 330
// Input from Vertex Shader
in vec4 fragColor; // Contains (r, g, b, 1.0) from the VBO

// Uniforms (If you want to fade all particles at once, e.g. at the end of the effect)
uniform float globalFade; // Pass this from raylib with SetShaderValue(..., GetShaderLocation(..., "globalFade"), &fadingValue, SHADER_UNIFORM_FLOAT)

out vec4 outColor;

void main()
{
    // Create a smooth circular particle shape using the point's local coordinate (0.0 to 1.0)
    vec2 pointCenter = gl_PointCoord - vec2(0.5);
    float distance = length(pointCenter);
    float alpha = 1.0 - smoothstep(0.4, 0.5, distance); // Makes a smooth, soft edge
    
    // The final color is the color received from the VBO
    vec4 finalColor = fragColor;

    // Apply the circular alpha mask
    finalColor.a = alpha;
    
    // Apply an optional global fade or other visual effect
    finalColor.a *= globalFade; 
    
    outColor = finalColor;
}