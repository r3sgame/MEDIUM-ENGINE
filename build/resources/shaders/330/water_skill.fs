#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

const vec4 color = vec4(0.0, 0.0, 1.0, 0.55);   // Chosen color to overlay

void main()
{
    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

    // Mix colors
    vec3 mixColor = mix(source.rgb, color.rgb, color.a);

    // Calculate final fragment color
    finalColor = vec4(mixColor, source.a);
}