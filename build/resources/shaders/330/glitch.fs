#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform float u_time;

// Output fragment color
out vec4 finalColor;

const vec4 color = vec4(0.09, 0.0, 0.0, 0.30);   // Chosen color to overlay

float rand(float n){
  return fract(sin(n + u_time) * 43758.5453) * 2.0 - 1.0;
}

void main()
{
    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);
    vec2 uv = fragTexCoord * 1000.0;

    if (source != vec4(0.0, 0.0, 0.0, 0.0)) {  
      vec3 glitch = vec3(rand(source.r + uv.x + uv.y * source.g), rand(source.b + uv.x + uv.y * source.r), rand(source.g + uv.x * source.b + uv.y));
      finalColor = vec4(mix(source.rgb, glitch, 0.6), source.a);
    } else {
      finalColor = source;
    }
    //vec4(rand(fragTexCoord.x), rand(fragTexCoord.y), rand(fragTexCoord.x + fragTexCoord.y), 1.0);
}
