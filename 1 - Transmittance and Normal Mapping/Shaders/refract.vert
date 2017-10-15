#version 410

in vec3 vp;
in vec3 vn;

uniform mat4 P; // projection matrix
uniform mat4 V; // view matrix
uniform mat4 M; // model matrix
out vec3 n_eye;
out vec3 p_eye;

void main() {
	p_eye = (V * M * vec4 (vp, 1.0)).xyz;
	n_eye = (V * M * vec4 (vn, 0.0)).xyz;
	gl_Position = P * V * M * vec4 (vp, 1.0);
}