#version 410

in vec3 vp;		//position
in vec3 vn;		//normals
in vec2 vt;		//tex coords
in vec4 vtan;	//tangents

uniform mat4 P; // projection matrix
uniform mat4 V; // view matrix
uniform mat4 M; // model matrix


out vec3 view_dir_tan;
out vec3 light_dir_tan;
out vec2 tex_coords;

vec3 light_pos_world = vec3(5.0, 20.0, 5.0);

void main() {
	tex_coords = vt;
	
	//Extract camera position from view matrix (avoid passing in another variable)
	vec3 cam_position = (inverse(V)*vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	vec3 light_direction = vp - light_pos_world;
	
	//Calculate bi-tangent, multiply by determinant to correct handedness
	vec3 bitan = cross(vn, vtan.xyz)*vtan.w;
	
	// transform world camera and light uniforms into local space
	vec3 cam_pos_local = vec3 (inverse (M) * vec4 (cam_position, 1.0));
	vec3 light_dir_local = vec3 (inverse (M) * vec4 (light_direction, 0.0));
	
	// Work out view direction in local space
	vec3 view_dir_local = normalize (cam_pos_local - vp);
	
	// work out view direction in tangent space
	view_dir_tan = vec3 (
		dot (vtan.xyz, view_dir_local),
		dot (bitan, view_dir_local),
		dot (vn, view_dir_local)
	);
	// work out light direction in tangent space
	light_dir_tan = vec3 (
		dot (vtan.xyz, light_dir_local),
		dot (bitan, light_dir_local),
		dot (vn, light_dir_local)
	);


	gl_Position = P * V * M * vec4 (vp, 1.0);
}