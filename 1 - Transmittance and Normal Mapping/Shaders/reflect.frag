#version 410

in vec3 n_eye; // normals from vertex shader
in vec3 p_eye;

uniform samplerCube cube_tex;
uniform mat4 V;
out vec4 frag_colour;

void main() {
	//determine incident ray and surface normal
	//normalize everything always to prevent errors
	vec3 incident = normalize(p_eye);
	vec3 normal = normalize(n_eye);

	//reflect incident ray around normal
	vec3 reflected = reflect(incident, normal);
	
	//convert from view to world space
	reflected = (inverse(V) * vec4(reflected, 0.0)).xyz;
	
	frag_colour = texture(cube_tex, reflected);
}