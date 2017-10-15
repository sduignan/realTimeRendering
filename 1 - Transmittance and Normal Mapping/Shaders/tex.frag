#version 400

in vec2 texture_coordinates;

in vec3 n_eye; // normals from vertex shader
in vec3 p_eye;
in mat4 view_mat;

uniform sampler2D _texture;

out vec4 frag_colour;

vec3 light_position_world = vec3 (0.0, 8.0, 2.0);
float spec_exp = 500.0f;	// Specular "power"

vec3 l_a = vec3 (0.2, 0.2, 0.2);	// low intensity ambient light
vec3 l_d = vec3 (0.8, 0.8, 0.8);	// mid-high intensity diffuse light
vec3 l_s = vec3 (1.0, 1.0, 1.0);	//high intensity specular light

vec3 k_s = vec3 (0.2, 0.2, 0.2);	// dim specular reflection

void main() {

	vec3 norm_eye = normalize (n_eye);

	vec4 texel = texture (_texture, texture_coordinates);

	vec3 light_position_eye = vec3 (view_mat * vec4 (light_position_world, 1.0));
	vec3 distance_to_light_eye = light_position_eye - p_eye;
	vec3 direction_to_light_eye = normalize (distance_to_light_eye);


	// diffuse intensity
	float dot_prod = dot (direction_to_light_eye, norm_eye);
	dot_prod = max (dot_prod, 0.0);
	vec3 Id = l_d * texel.xyz * dot_prod; // final diffuse intensity

	// Ambient light
	vec3 Ia = l_a * texel.xyz;
	
	// specular intensity
	vec3 surface_to_viewer_eye = normalize (-p_eye);
	
	// blinn
	vec3 half_way_eye = normalize (surface_to_viewer_eye + direction_to_light_eye);
	float dot_prod_specular = max (dot (half_way_eye, norm_eye), 0.0);
	float specular_factor = pow (dot_prod_specular, spec_exp);
	
	vec3 Is = l_s * k_s * specular_factor; // final specular intensity

	vec3 I = Ia + Id + Is;
	
	frag_colour = vec4 (I, 1.0);
}
