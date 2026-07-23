#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float u_time;
	const int AMOUNT = 6;

float rand(float n){
    return fract(sin(n + u_time) * 43758.5453) * 2.0 - 1.0;
}

float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

void main() {
	vec2 coord = 20.0 * fragTexCoord;
	float len;
	for (int i = 0; i < AMOUNT; i++) {
		len = length(vec2(coord.x, coord.y));
		coord.x = coord.x - cos(coord.y + sin(len)) + cos(u_time / 9.0);
		coord.y = coord.y + sin(coord.x + cos(len)) + sin(u_time / 12.0);
	}

	vec4 col = vec4(cos(len * 2.0), 0, 0, 0.2);
	vec3 glitch = vec3(random(coord), random(coord * 2.0), random(coord * 3.0));

	gl_FragColor = vec4(mix(col.rgb, glitch, 0.6), 0.4);
}