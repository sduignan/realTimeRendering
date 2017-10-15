#version 410

in vec3 vp;		//position
in vec3 vn;		//normals
in vec2 vt;		//tex coords

uniform mat4 P; // projection matrix
uniform mat4 V; // view matrix
uniform mat4 M; // model matrix

out vec3 local_pos;
out vec3 normal;

//Baseline silhouette thickness
float sil_offset = 0.002f;

const vec3 light_position_world = vec3 (0.0, 8.0, 2.0);

void main() {	
	local_pos = vp;
	normal = vn;
	
	//Calculate lighting (Phong diffuse rather than Oren Nayar for speed
	vec3 n_eye = (V * M * vec4 (vn, 0.0)).xyz;
	vec3 p_eye = (V * M * vec4 (vp, 1.0)).xyz;
	
	vec3 light_position_eye = vec3 (V * vec4 (light_position_world, 1.0));
	vec3 distance_to_light_eye = light_position_eye - p_eye;
	vec3 direction_to_light_eye = normalize (distance_to_light_eye);
	
	// diffuse intensity
	float diffuse_intensity = dot (direction_to_light_eye, n_eye);
	diffuse_intensity = max(2*diffuse_intensity, 1.0);
	
	float depth = (P*V*M*vec4(vp, 1.0)).z;
	
	//Scale the thickness of the silhouette based on the depth into the image, and the lighting at that point
	sil_offset = abs(depth*sil_offset*(1/diffuse_intensity));
	
	vec4 sil_pos = vec4(vp + vn*sil_offset, 1.0);
	vec4 pos = P * V * M * sil_pos;
	//Push the silhouette deeper into the scene, so it's hidden behind the object it's outlining
	pos.z += 5*sil_offset;
	gl_Position = pos;
}