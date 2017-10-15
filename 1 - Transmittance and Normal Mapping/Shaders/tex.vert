#version 400

layout (location = 0) in vec3 vp; //vertex position
layout (location = 1) in vec3 vn; // vertex normal
layout (location = 2) in vec2 vt; // per-vertex texture co-ords

uniform mat4 P; // projection matrix
uniform mat4 V; // view matrix
uniform mat4 M; // model matrix
out vec3 n_eye; //normal in eyespace
out vec3 p_eye; //position in eyespace
out mat4 view_mat;

out vec2 texture_coordinates;

void main() {
	// send normals to fragment shader
	n_eye = (V * M * vec4 (vn, 0.0)).xyz;
	p_eye = (V * M * vec4 (vp, 1.0)).xyz;
	view_mat = V;
	texture_coordinates = vt;
	gl_Position = P * V * M * vec4 (vp, 1.0);
}
