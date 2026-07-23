#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float u_time;

float rand(float n){
  return fract(sin(n + u_time) * 43758.5453) * 2.0 - 1.0;
}

void main() {
	vec2 uv = fragTexCoord * 1000.0;

	vec3 color = vec3(0.0, 0.0, 0.5*abs(0.25*sin(0.5*u_time + fragTexCoord.x + fragTexCoord.y)));
    
	vec3 glitch = vec3(rand(color.r + uv.x + uv.y * color.b), rand(color.r * uv.x + uv.y + color.b), rand(color.b * color.r * uv.x + uv.y + 2.0));

	vec3 mixColor = mix(color, glitch, 0.25);

	gl_FragColor = vec4(mixColor.b, mixColor.g, mixColor.r, 0.75);
}