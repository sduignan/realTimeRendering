#version 410

in vec3 local_pos;
in vec3 normal;
uniform sampler2DArray charcoal_textures;

out vec4 frag_colour;

void main() {
	//Tri-planar mapping time
	vec3 blending_ratio = normalize(abs(normal));
	//Texel fetching, and blending
	vec3 charcoal_black = texture(charcoal_textures, vec3(local_pos.yz, 0)).xyz*blending_ratio.x +
							texture(charcoal_textures, vec3(local_pos.xz, 0)).xyz*blending_ratio.y +
							texture(charcoal_textures, vec3(local_pos.xy, 0)).xyz*blending_ratio.z;

	frag_colour = vec4(charcoal_black, 1.0);
}