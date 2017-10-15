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
	
	
	//REFLECTION
	//reflect incident ray around normal
	vec3 reflected_ray = reflect(incident, normal);

	//convert from view to world space
	reflected_ray = (inverse(V) * vec4(reflected_ray, 0.0)).xyz;
	
	//reflected colour
	vec4 reflected_val = texture(cube_tex, reflected_ray);
	
	
	//REFRACTION
	//Approx. values for diamond
	//Different rgb values for chromatic dispersion
	float eta_r = 0.395;
	float eta_g = 0.41;
	float eta_b = 0.425;
		
	//refract incident ray
	vec3 refracted_ray_r = refract(incident, normal, eta_r);
	vec3 refracted_ray_g = refract(incident, normal, eta_g);
	vec3 refracted_ray_b = refract(incident, normal, eta_b);
	
	//convert from view to world space
	refracted_ray_r = (inverse(V) * vec4(refracted_ray_r, 0.0)).xyz;
	refracted_ray_g = (inverse(V) * vec4(refracted_ray_g, 0.0)).xyz;
	refracted_ray_b = (inverse(V) * vec4(refracted_ray_b, 0.0)).xyz;
	
	//Refracted colour
	vec4 refracted_val;
	refracted_val.r = texture(cube_tex, refracted_ray_r).r;
	refracted_val.g = texture(cube_tex, refracted_ray_g).g;
	refracted_val.b = texture(cube_tex, refracted_ray_b).b;
	
	
	//Fresnel
	vec3 fresnel;
	fresnel.r = ((1-eta_r)*(1-eta_r))/((1+eta_r)*(1+eta_r));
	fresnel.g = ((1-eta_g)*(1-eta_g))/((1+eta_g)*(1+eta_g));
	fresnel.b = ((1-eta_b)*(1-eta_b))/((1+eta_b)*(1+eta_b));
	
	//reflect-refract ratio (Schlick appox)
	vec3 ratio;
	ratio.r = fresnel.r + (1.0 - fresnel.r)*pow((1.0 - dot(-incident, normal)), 5);
	ratio.g = fresnel.g + (1.0 - fresnel.g)*pow((1.0 - dot(-incident, normal)), 5);
	ratio.b = fresnel.b + (1.0 - fresnel.b)*pow((1.0 - dot(-incident, normal)), 5);
		
	//final value
	vec3 colour =  mix(refracted_val.xyz, reflected_val.xyz, ratio);
	frag_colour = vec4(colour, 1.0);
}