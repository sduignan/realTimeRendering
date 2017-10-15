#version 410

in vec3 vp;		//position
in vec3 vn;		//normals
in vec2 vt;		//tex coords
in vec4 vtan;	//tangents

uniform mat4 P; // projection matrix
uniform mat4 V; // view matrix
uniform mat4 M; // model matrix


out vec2 tex_coords;
out vec3 p_eye;
out vec3 T;
out vec3 N;
out vec3 B;

void main() {
	tex_coords = vt;
	
	//For reflection
	p_eye = (V * M * vec4 (vp, 1.0)).xyz;

	//Probably over-cautious here to normalize everything,
	//especially since everything is re-normalised in frag shader
	T = normalize(vtan).xyz;
	//Calculate bi-tangent, multiply by determinant to correct handedness
	B = normalize(cross(vn, vtan.xyz)*vtan.w);
	N = normalize(vn);


	gl_Position = P * V * M * vec4 (vp, 1.0);
}