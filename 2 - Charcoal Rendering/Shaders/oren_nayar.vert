#version 410

in vec3 vp;		//position
in vec3 vn;		//normals
in vec2 vt;		//tex coords
in vec4 vtan;	//tangents

uniform mat4 P; // projection matrix
uniform mat4 V; // view matrix
uniform mat4 M; // model matrix


out vec3 n_eye;
out vec3 p_eye;
out vec3 normal;

void main() {
	
	//For Oren Nayar lighting
	n_eye = (V * M * vec4 (vn, 0.0)).xyz;
	p_eye = (V * M * vec4 (vp, 1.0)).xyz;

	normal = vn;
	
	gl_Position = P * V * M * vec4 (vp, 1.0);
}