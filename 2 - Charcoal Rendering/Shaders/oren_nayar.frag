#version 410

uniform samplerCube cube_tex;
uniform mat4 M;
uniform mat4 V;

in vec3 n_eye;
in vec3 p_eye;
in vec3 normal;

vec3 light_position_world = vec3 (0.0, 4.0, 2.0);

vec3 l_d = vec3 (0.8, 0.8, 0.8);	// mid-high intensity diffuse light

out vec4 frag_colour;

float roughness = 0.3f;

void main() {
	//Interpolation can mess up normals
	vec3 norm = normalize(n_eye);
	vec3 view_dir = normalize(p_eye);
	vec3 light_dir = normalize(vec3 (V * vec4 (light_position_world, 1.0)) - p_eye);
	
	//Oren Nayar Lighting
	float v_dot_n = dot(view_dir, norm);
	float l_dot_n = dot(light_dir, norm);
	
	float sigma_2 = roughness*roughness;
	
	float theta_r = acos(v_dot_n);
	float theta_i = acos(l_dot_n);
	
	float cos_phi_diff = dot(normalize(view_dir-norm*v_dot_n), normalize(light_dir-norm*l_dot_n));
	
	float alpha = max(theta_i, theta_r);
	float beta = min(theta_i, theta_r);
	
	float A = 1.0f - 0.5f*sigma_2/(sigma_2+0.33);
	float B = 0.45*sigma_2/(sigma_2+0.09);
	
	float o_n = clamp(l_dot_n, 0.0f, 1.0f)*(A+(B * clamp(cos_phi_diff, 0.0f, 1.0f) * sin(alpha) * tan(beta)));

	frag_colour = vec4(o_n*l_d, 1.0);
}