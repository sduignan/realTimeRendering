#version 410

in vec3 vp;		//position
in vec3 vn;		//normals
in vec2 vt;		//tex coords
in vec4 vtan;	//tangents

uniform mat4 M, V, P;
//view and projection matrices for the depth map
uniform mat4 caster_P, caster_V;
//Texture coordinates for depth map
out vec4 st_shadow;
out vec2 tex_coords;
out vec3 n_eye;
out vec3 p_eye;
out vec3 normal;
out vec3 local_pos;

void main() {
	tex_coords = vt;
	
	//For reflection
	n_eye = (V * M * vec4 (vn, 0.0)).xyz;
	p_eye = (V * M * vec4 (vp, 1.0)).xyz;

	normal = vn;
	
	local_pos = vp;
	
	//Shadow map texture coordinates - the position of the vertex from the light's p.o.v.
	st_shadow = caster_P * caster_V * M * vec4 (vp, 1.0);
	
	gl_Position = P * V * M * vec4 (vp, 1.0);
}