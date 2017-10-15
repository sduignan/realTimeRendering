#version 410

uniform mat4 V;

in vec2 tex_coords;
in vec3 n_eye;
in vec3 p_eye;
in vec3 normal;
in vec3 local_pos;

uniform sampler2D _texture;
uniform sampler2DArray charcoal_textures;

vec3 light_position_world = vec3 (0.0, 8.0, 2.0);

float roughness = 0.3f;

out vec4 frag_colour;

void main() {
	//Interpolation can mess up normals
	vec3 norm = normalize(n_eye);
	vec3 view_dir = normalize(-p_eye);
	vec3 light_dir = normalize(vec3 (V * vec4 (light_position_world, 1.0)) - p_eye);
	
	//Oren Nayar lighting
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
	vec3 colour = o_n*texture(_texture, tex_coords).xyz;
	float charcoal_I = (colour.x+colour.y+colour.z)*2.3333;
	
	//Tri-planar mapping time
	vec3 blending_ratio = normalize(abs(normal));
		
	//Texel fetching, blending, and interpolating
	vec3 charcoal_lower = texture(charcoal_textures, vec3(local_pos.yz, floor(charcoal_I))).xyz*blending_ratio.x +
							texture(charcoal_textures, vec3(local_pos.xz, floor(charcoal_I))).xyz*blending_ratio.y +
							texture(charcoal_textures, vec3(local_pos.xy, floor(charcoal_I))).xyz*blending_ratio.z;
	vec3 charcoal_higher = texture(charcoal_textures, vec3(local_pos.yz, ceil(charcoal_I))).xyz*blending_ratio.x +
							texture(charcoal_textures, vec3(local_pos.xz, ceil(charcoal_I))).xyz*blending_ratio.y +
							texture(charcoal_textures, vec3(local_pos.xy, ceil(charcoal_I))).xyz*blending_ratio.z;
	vec3 charcoal_final = mix(charcoal_lower, charcoal_higher, (charcoal_I-floor(charcoal_I)));
	
	
	frag_colour = vec4 (charcoal_final, 1.0);
}