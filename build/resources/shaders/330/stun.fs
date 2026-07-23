#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform float u_time;

// Output fragment color
out vec4 finalColor;

const vec4 color = vec4(1.0, 0.0, 0.0, 0.55);   // Chosen color to overlay

float rand(float n){
    return fract(sin(n + u_time) * 43758.5453) * 2.0 - 1.0;
}

void main()
{
    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

    // Mix colors
    vec3 mixColor = mix(source.rgb, color.rgb, color.a);

    vec2 uv = fragTexCoord * 1000.0;

    vec3 glitch = vec3(rand(source.r + uv.x + uv.y * source.g), rand(source.b + uv.x + uv.y * source.r), rand(source.g + uv.x * source.b + uv.y));
    finalColor = vec4(mix(mixColor.rgb, glitch, 0.4), source.a);
}