#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float u_time;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main() {
	// Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

	// Circle transition mask
	if (u_time > 2.0 && pow(fragTexCoord.x, 2) + pow(fragTexCoord.y, 2) > 5 * max(0, 1 - (u_time - 2.0))) {
		gl_FragColor = source;	
	} else {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
	}
}
