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
	vec2 coord = 20.0 * fragTexCoord;

	vec4 col = vec4(abs(0.25*sin(u_time + fragTexCoord.x)),0.0,0.0,1.0);
	vec3 glitch = vec3(rand(coord.x + coord.y * col.r), rand(coord.x - coord.y * col.r), rand(coord.x * col.r + coord.y));

	gl_FragColor = vec4(mix(col.rgb, glitch, 0.6), 0.4);
}