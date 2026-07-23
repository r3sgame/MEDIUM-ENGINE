#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform float u_time;

// Output fragment color
out vec4 finalColor;

//const vec4 color = vec4(0.09, 0.0, 0.0, 0.30);   // Chosen color to overlay

void main()
{
    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

    vec2 uv = fragTexCoord * 10.0;

    float time = u_time;

    float color = 0.4 * (0.8 + 0.25*cos(time+uv.y) + 0.25*cos(time+uv.x));
    //+ 0.4 * (0.5 + 0.25*sin(-u_time+uv.y) + 0.25*sin(-u_time+uv.x));

    // Mix colors
    vec3 mixColor = mix(source.rgb, vec3(color, 0, 0), 0.4);

    // Calculate final fragment color
    finalColor = vec4(mixColor, source.a);
}