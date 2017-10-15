#version 410

in vec3 view_dir_tan;
in vec3 light_dir_tan;
in vec2 tex_coords;

uniform sampler2D _texture;
uniform sampler2D normal_map;

out vec4 frag_colour;

vec3 l_a = vec3 (0.2, 0.2, 0.2);	// low intensity ambient light
vec3 l_d = vec3 (0.8, 0.8, 0.8);	// mid-high intensity diffuse light
vec3 l_s = vec3 (1.0, 1.0, 1.0);	// White specular light

float spec_exp = 100.0f;	// Specular "power"

vec3 k_s = vec3 (0.2, 0.2, 0.2);	// dim specular reflection

void main() {
	//sample the normal map, convert from 0...1 range to -1...1 range
	vec3 normal_tan = texture(normal_map, tex_coords).rgb;
	normal_tan = normalize(normal_tan*2.0 - 1.0);
	
	//get colour of fragment from texture
	vec3 texel = texture(_texture, tex_coords).xyz;
	
	//Ambient component, doesn't need to be done in tan space
	vec3 Ia = l_a*texel;
	
	//diffuse light, calculated in tangent space
	vec3 dir_to_light_tan = normalize(-light_dir_tan);
	float dot_prod = max(dot (dir_to_light_tan, normal_tan), 0.0);
	vec3 Id = l_d * texel * dot_prod;
	
	//Specular in tangent space	
	vec3 reflection_tan = reflect (normalize (light_dir_tan), normal_tan);
	float dot_prod_s = max (dot (reflection_tan, normalize (view_dir_tan)), 0.0);
	float specular_factor = pow (dot_prod_s, spec_exp);
	vec3 Is = l_s * k_s * specular_factor;
	
	frag_colour = vec4((Ia+Id+Is), 1.0);
}