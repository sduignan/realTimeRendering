#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>

//3rd party code
#include "Antons_maths_funcs.h"
#include "obj_parser.h"
#include "text.h"
#include "gl_utils.h" // common opengl functions and small utilities like logs

#define STBI_ASSERT(x)

//My own classes
#include "scene_object.h"

GLuint light_depth_fb;
GLuint light_depth_fb_tex;
int light_shadow_size = 1024;

mat4 light_V;
mat4 light_P;

GLuint msaa_fb;
GLuint msaa_fb_tex;

int g_gl_width = 1600;
int g_gl_height = 900;
bool close_window = false;
GLFWwindow* g_window = NULL;

float angle = 0.0f;
float model_rotation = 0.0f;
float base_rotation = -45.0f;

// camera matrices. it's easier if they are global
mat4 view_mat;
mat4 proj_mat;
vec3 cam_pos(0.0f, 0.0f, 5.0f);

bool cam_moved = false;
vec3 move(0.0, 0.0, 0.0);
float cam_yaw = 0.0f; // y-rotation in degrees
float cam_pitch = 0.0f;
float cam_roll = 0.0;

float increment = 0.5f;
float trans_increment = 0.05f;
float cam_heading = 0.0f; // y-rotation in degrees

// keep track of some useful vectors that can be used for keyboard movement
vec4 fwd(0.0f, 0.0f, -1.0f, 0.0f);
vec4 rgt(1.0f, 0.0f, 0.0f, 0.0f);
vec4 up(0.0f, 1.0f, 0.0f, 0.0f);

versor q;

#define NUM_SHADERS 4
std::string shader_list[] = { "depth", "oren_nayar", "charcoal_not_interpolated", "charcoal_interpolated"};
char* texture_array_files[] = { "Textures/charcoal_0.png", "Textures/charcoal_1.png", "Textures/charcoal_2.png", "Textures/charcoal_3.png", "Textures/charcoal_4.png", "Textures/charcoal_5.png", "Textures/charcoal_6.png", "Textures/charcoal_7.png", "Textures/charcoal_8.png"};

struct shader_info {
	GLuint shader_id;
	int M_loc;
	int V_loc;
	int P_loc;
} shader_infos[NUM_SHADERS];

struct shadowed_shader {
	shader_info base;
	int shadow_P_loc;
	int shadow_V_loc;
};
#define NUM_SHADOWED_SHADERS 2
std::string shadowed_shaders_list[] = {"shade_unsmoothed", "shade"};
shadowed_shader shadowed_shaders[2];

int turn = 1;
int choice = 0;

int text_ids[8];

bool toggle_text = false;

//keyboard control
void My_Key_Callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
					close_window = true;
					break;
			case GLFW_KEY_LEFT: {
				cam_yaw += increment;
				cam_moved = true;
				versor q_yaw = quat_from_axis_deg(cam_yaw, up.v[0], up.v[1], up.v[2]);
				q = q_yaw * q;
				break;
			}
			case GLFW_KEY_RIGHT: {
				cam_yaw -= increment;
				cam_moved = true;
				versor q_yaw = quat_from_axis_deg(
					cam_yaw, up.v[0], up.v[1], up.v[2]
				);
				q = q_yaw * q;
				break;
			}
			case GLFW_KEY_UP: {
				cam_pitch += increment;
				cam_moved = true;
				versor q_pitch = quat_from_axis_deg(
					cam_pitch, rgt.v[0], rgt.v[1], rgt.v[2]
				);
				q = q_pitch * q;
				break;
			}
			case GLFW_KEY_DOWN: {
				cam_pitch -= increment;
				cam_moved = true;
				versor q_pitch = quat_from_axis_deg(
					cam_pitch, rgt.v[0], rgt.v[1], rgt.v[2]
				);
				q = q_pitch * q;
				break;
			}
			case GLFW_KEY_Z: {
				cam_roll += increment;
				cam_moved = true;
				versor q_roll = quat_from_axis_deg(
					cam_roll, fwd.v[0], fwd.v[1], fwd.v[2]
				);
				q = q_roll * q;
				break;
			}
			case GLFW_KEY_C: {
				cam_roll -= increment;
				cam_moved = true;
				versor q_roll = quat_from_axis_deg(
					cam_roll, fwd.v[0], fwd.v[1], fwd.v[2]
				);
				q = q_roll * q;
				break;
			}
			case GLFW_KEY_A: {
				move.v[0] -= trans_increment;
				cam_moved = true;
				break;
			}
			case GLFW_KEY_D: {
				move.v[0] += trans_increment;
				cam_moved = true;
				break;
			}
			case GLFW_KEY_W: {
				move.v[2] -= trans_increment;
				cam_moved = true;
				break;
			}
			case GLFW_KEY_S: {
				move.v[2] += trans_increment;
				cam_moved = true;
				break;
			}
			case GLFW_KEY_Q: {
				move.v[1] -= trans_increment;
				cam_moved = true;
				break;
			}
			case GLFW_KEY_E: {
				move.v[1] += trans_increment;
				cam_moved = true;
				break;
			}
			case GLFW_KEY_T: {
				toggle_text = !toggle_text;
				break;
			}
			case GLFW_KEY_SPACE: {
				choice = (choice + 1) % 8;
				unhide_text(text_ids[choice]);
				hide_text(text_ids[(choice + 7)%8]);
				break;
			}
			case GLFW_KEY_ENTER: {
				//Stop the scene spinning
				turn = (turn+1)%2;
				break;
			}
		}
	}
}

void cast_shadows(scene_object box, scene_object pear, scene_object pear2, scene_object skull, scene_object vase) {
	//bind framebuffer that renders to texture
	glBindFramebuffer(GL_FRAMEBUFFER, light_depth_fb);
	//set viewpor to correct size
	glViewport(0, 0, light_shadow_size, light_shadow_size);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// no need to clear the colour buffer
	glClear(GL_DEPTH_BUFFER_BIT);
	// bind out shadow-casting shader from the previous section
	glUseProgram(shader_infos[0].shader_id);
	// send in the view and projection matrices from the light
	glUniformMatrix4fv(shader_infos[0].V_loc, 1, GL_FALSE, light_V.m);
	glUniformMatrix4fv(shader_infos[0].P_loc, 1, GL_FALSE, light_P.m);

	glUniformMatrix4fv(shader_infos[0].M_loc, 1, GL_FALSE, box.model.m);

	box.draw();
	pear.draw();
	pear2.draw();
	vase.draw();
	//Stupid skull won't stay where I tell it to in blender.
	glUniformMatrix4fv(shader_infos[0].M_loc, 1, GL_FALSE, skull.model.m);
	skull.draw();
	
	if (choice < 6) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, msaa_fb);
	}
}

int main() {
	//START OPENGL
	restart_gl_log();
	// start GL context and O/S window using the GLFW helper library
	start_gl();

	// Tell the window where to find its key callback function
	glfwSetKeyCallback(g_window, My_Key_Callback);
	
	//TEXT setup
	if (!init_text_rendering("freemono.png", "freemono.meta", g_gl_width, g_gl_height))
	{
		fprintf(stderr, "ERROR init text rendering\n");
		exit(1);
	}

	
	text_ids[0] = add_text("Start with Oren Nayar Shading", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[1] = add_text("Use tri-planar mapping to project charcoal textures onto the objects", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[2] = add_text("Interpolate between the two closest charcoal tones", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[3] = add_text("Add shadows to the scene", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[4] = add_text("Soften the shadow edges using Gaussian blurring", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[5] = add_text("Add variable thickness silhouettes to the scene", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[6] = add_text("Use 16x multisample anti-aliasing to improve the look of the result", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);
	text_ids[7] = add_text("Charcoal Shader", -0.8, 0.85, 30, 0.0, 0.0, 0.0, 1.0);

	//Hide all but first set
	for (int i = 1; i < 8; i++) {
		hide_text(text_ids[i]);
	}

	// Set up vertex buffers and vertex array objects
	scene_object skull("Meshes/skull.obj", "Textures/skull/skull_diffuse.png", "Textures/skull/skull_normal.png", "Textures/noise.png", texture_array_files);
	scene_object box("Meshes/box.obj", "Textures/box/box_diffuse.png", "Textures/box/box_normal.png", "Textures/noise.png", texture_array_files);
	scene_object pear("Meshes/pear.obj", "Textures/pear/pear_diffuse.jpg", "Textures/pear/pear_normal_map.jpg", "Textures/noise.png", texture_array_files);
	scene_object pear2("Meshes/pear2.obj", "Textures/pear/pear_diffuse.jpg", "Textures/pear/pear_normal_map.jpg", "Textures/noise.png", texture_array_files);
	scene_object vase("Meshes/vase.obj", "Textures/box/box_diffuse.png", "Textures/pear/pear_normal_map.jpg", "Textures/noise.png", texture_array_files);

	// Shaders
	for (int i = 0; i < NUM_SHADERS; i++) {
		std::string frag = "Shaders/" + shader_list[i] + ".frag";
		std::string vert = "Shaders/" + shader_list[i] + ".vert";
		shader_infos[i].shader_id = create_programme_from_files(vert.c_str(), frag.c_str());
		glUseProgram(shader_infos[i].shader_id);
		//get locations of M, V, P matrices
		shader_infos[i].M_loc = glGetUniformLocation(shader_infos[i].shader_id, "M");
		assert(shader_infos[i].M_loc > -1);
		shader_infos[i].V_loc = glGetUniformLocation(shader_infos[i].shader_id, "V");
		assert(shader_infos[i].V_loc > -1);
		shader_infos[i].P_loc = glGetUniformLocation(shader_infos[i].shader_id, "P");
		assert(shader_infos[i].P_loc > -1);

		//No asserts here, these uniforms don't exist in all shaders, it's okay for them to fail silently
		GLuint texLoc = glGetUniformLocation(shader_infos[i].shader_id, "_texture");
		glUniform1i(texLoc, 0);
		texLoc = glGetUniformLocation(shader_infos[i].shader_id, "normal_map");
		glUniform1i(texLoc, 1);
		texLoc = glGetUniformLocation(shader_infos[i].shader_id, "noise");
		glUniform1i(texLoc, 2);
		texLoc = glGetUniformLocation(shader_infos[i].shader_id, "charcoal_textures");
		glUniform1i(texLoc, 3);		
	}

	//Setup for silhouette shader
	shader_info silhouette;
	silhouette.shader_id = create_programme_from_files("Shaders/silhouette.vert", "Shaders/silhouette.frag");
	glUseProgram(silhouette.shader_id);
	//get locations of M, V, P matrices
	silhouette.M_loc = glGetUniformLocation(silhouette.shader_id, "M");
	assert(silhouette.M_loc > -1);
	silhouette.V_loc = glGetUniformLocation(silhouette.shader_id, "V");
	assert(silhouette.V_loc > -1);
	silhouette.P_loc = glGetUniformLocation(silhouette.shader_id, "P");
	assert(silhouette.P_loc > -1);

	GLuint texLoc = glGetUniformLocation(silhouette.shader_id, "charcoal_textures");
	glUniform1i(texLoc, 3);

	//Separate setup for shadowed shader

	for (int i = 0; i < NUM_SHADOWED_SHADERS; i++) {
		std::string frag = "Shaders/" + shadowed_shaders_list[i] + ".frag";
		std::string vert = "Shaders/" + shadowed_shaders_list[i] + ".vert";
		shadowed_shaders[i].base.shader_id = create_programme_from_files(vert.c_str(), frag.c_str());
		glUseProgram(shadowed_shaders[i].base.shader_id);
		shadowed_shaders[i].base.M_loc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "M");
		assert(shadowed_shaders[i].base.M_loc > -1);
		shadowed_shaders[i].base.V_loc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "V");
		assert(shadowed_shaders[i].base.V_loc > -1);
		shadowed_shaders[i].base.P_loc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "P");
		assert(shadowed_shaders[i].base.P_loc > -1);
		shadowed_shaders[i].shadow_V_loc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "caster_V");
		assert(shadowed_shaders[i].shadow_V_loc > -1);
		shadowed_shaders[i].shadow_P_loc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "caster_P");
		assert(shadowed_shaders[i].shadow_P_loc > -1);

		GLuint texLoc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "_texture");
		glUniform1i(texLoc, 0);
		texLoc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "normal_map");
		glUniform1i(texLoc, 1);
		texLoc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "noise");
		glUniform1i(texLoc, 2);
		texLoc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "charcoal_textures");
		glUniform1i(texLoc, 3);
		texLoc = glGetUniformLocation(shadowed_shaders[i].base.shader_id, "depth_map");
		glUniform1i(texLoc, 4);
	}


	//Multi-sampling framebuffer
	glGenTextures(1, &msaa_fb_tex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_fb_tex);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 16, GL_RGBA8, g_gl_width, g_gl_height, false);

	glGenFramebuffers(1, &msaa_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, msaa_fb);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaa_fb_tex, 0);

	//Fussy framebuffer wants its own depth texture to use as a depth buffer
	GLuint depth_texture;
	glGenTextures(1, &depth_texture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depth_texture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 16, GL_DEPTH_COMPONENT, g_gl_width, g_gl_height, false);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depth_texture, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	//Set up framebuffer that renders to texture for light-depth values
	{
		// create framebuffer
		glGenFramebuffers(1, &light_depth_fb);
		glBindFramebuffer(GL_FRAMEBUFFER, light_depth_fb);

		// create texture for framebuffer
		glGenTextures(1, &light_depth_fb_tex);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, light_depth_fb_tex);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_DEPTH_COMPONENT,
			light_shadow_size,
			light_shadow_size,
			0,
			GL_DEPTH_COMPONENT,
			GL_UNSIGNED_BYTE,
			NULL
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// clamp to edge. Scene positioned so that all shadow casting objects fall within camera view
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach depth texture to framebuffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, light_depth_fb_tex, 0);

		// tell framebuffer not to use any colour drawing outputs
		GLenum draw_bufs[] = { GL_NONE };
		glDrawBuffers(1, draw_bufs);

		//No buffer to read from for drawing at this time
		glReadBuffer(GL_NONE);

		// bind default framebuffer again
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	

	//CREATE CAMERA
	// input variables
	float near = 0.1f; // clipping plane
	float far = 100.0f; // clipping plane
	float fovy = 67.0f; // 67 degrees
	float aspect = (float)g_gl_width / (float)g_gl_height; // aspect ratio
	proj_mat = perspective(fovy, aspect, near, far);

	mat4 T = translate(
		identity_mat4(), vec3(-cam_pos.v[0], -cam_pos.v[1], -cam_pos.v[2])
	);
	mat4 R = rotate_y_deg(identity_mat4(), -cam_heading);
	q = quat_from_axis_deg(-cam_heading, 0.0f, 1.0f, 0.0f);
	view_mat = R * T;


	//SET RENDERING DEFAULTS
	for (int i = 0; i < NUM_SHADERS; i++) {
		glUseProgram(shader_infos[i].shader_id);
		glUniformMatrix4fv(shader_infos[i].P_loc, 1, GL_FALSE, proj_mat.m);
		glUniformMatrix4fv(shader_infos[i].V_loc, 1, GL_FALSE, view_mat.m);
	}
	
	mat4 scene_model_mat = scale(identity_mat4(), vec3(3, 3, 3));

	//SHADOW CASTER VARIABLES
	vec3 light_pos(0.0, 8.0, 2.0);
	vec3 light_target(0.0, 0.0, 2.0);
	vec3 up_dir(0.0, 0.0, -1.0);
	light_V = look_at(light_pos, light_target, up_dir);

	float light_near = 0.5f;
	float light_far = 10.0f;
	float light_fov = 60.0f;
	float light_aspect = 1.0f;
	light_P = perspective(light_fov, light_aspect, light_near, light_far);


	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // set counter-clock-wise vertex order to mean the front

	glEnable(GL_MULTISAMPLE);
	glClearColor(0.2, 0.2, 0.2, 1.0); // grey background to help spot mistakes
	glViewport(0, 0, g_gl_width, g_gl_height);

	//MAIN LOOP
	while (!glfwWindowShouldClose(g_window)) {
		float x_displacement = 0;
		mat4 model = translate(rotate_y_deg(scene_model_mat, model_rotation), vec3(x_displacement, -1.75, 0.0));
		box.model = model;
		//pear.model = model;
		//pear2.model = model;
		skull.model = translate(model, vec3(0.0, -0.85*3.0, 0.0));

		cast_shadows(box, pear, pear2, skull, vase);

		// wipe the drawing surface clear
		glViewport(0, 0, g_gl_width, g_gl_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (choice <= 2) {
			glUseProgram(shader_infos[choice + 1].shader_id);
			glUniformMatrix4fv(shader_infos[choice + 1].V_loc, 1, GL_FALSE, view_mat.m);
			glUniformMatrix4fv(shader_infos[choice + 1].P_loc, 1, GL_FALSE, proj_mat.m);
			glUniformMatrix4fv(shader_infos[choice + 1].M_loc, 1, GL_FALSE, model.m);

			box.draw();
			pear.draw();
			pear2.draw();
			vase.draw();
			glUniformMatrix4fv(shader_infos[choice + 1].M_loc, 1, GL_FALSE, skull.model.m);
			skull.draw();
		}
		else {
			int shader_choice = !(1 < (choice - 3)) ? (choice - 3) : 1;
			glUseProgram(shadowed_shaders[shader_choice].base.shader_id);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, light_depth_fb_tex);


			glUniformMatrix4fv(shadowed_shaders[shader_choice].base.V_loc, 1, GL_FALSE, view_mat.m);
			glUniformMatrix4fv(shadowed_shaders[shader_choice].base.P_loc, 1, GL_FALSE, proj_mat.m);
			glUniformMatrix4fv(shadowed_shaders[shader_choice].shadow_V_loc, 1, GL_FALSE, light_V.m);
			glUniformMatrix4fv(shadowed_shaders[shader_choice].shadow_P_loc, 1, GL_FALSE, light_P.m);

			glUniformMatrix4fv(shadowed_shaders[shader_choice].base.M_loc, 1, GL_FALSE, model.m);

			box.draw();
			pear.draw();
			pear2.draw();
			vase.draw();
			glUniformMatrix4fv(shadowed_shaders[shader_choice].base.M_loc, 1, GL_FALSE, skull.model.m);
			skull.draw();
		}

		if (choice >= 5) {
			//Silhouetting		
			glUseProgram(silhouette.shader_id);
			glUniformMatrix4fv(silhouette.V_loc, 1, GL_FALSE, view_mat.m);
			glUniformMatrix4fv(silhouette.P_loc, 1, GL_FALSE, proj_mat.m);
			glUniformMatrix4fv(silhouette.M_loc, 1, GL_FALSE, model.m);

			box.draw();
			pear.draw();
			pear2.draw();
			vase.draw();
			glUniformMatrix4fv(silhouette.M_loc, 1, GL_FALSE, skull.model.m);
			skull.draw();
		}

		//Rotate the scene
		model_rotation = base_rotation + 30*sin(angle);
		angle = fmod(angle + turn*ONE_DEG_IN_RAD*0.5, TAU);

		//Write scene description on frame
		if (toggle_text) {
			draw_texts();
		}

		if (choice >= 6) {
			//Anti-aliasing
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);   // Make sure no FBO is set as the draw framebuffer
			glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fb); // Make sure your multisampled FBO is the read framebuffer
			glDrawBuffer(GL_BACK);                       // Set the back buffer as the draw buffer
			glBlitFramebuffer(0, 0, g_gl_width, g_gl_height, 0, 0, g_gl_width, g_gl_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}


		if (GLFW_PRESS == glfwGetKey(g_window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(g_window, 1);
		}

		// update view matrix
		if (cam_moved) {
			cam_heading += cam_yaw;

			// re-calculate local axes so can move fwd in dir cam is pointing
			R = quat_to_mat4(q);
			fwd = R * vec4(0.0, 0.0, -1.0, 0.0);
			rgt = R * vec4(1.0, 0.0, 0.0, 0.0);
			up = R * vec4(0.0, 1.0, 0.0, 0.0);

			cam_pos = cam_pos + vec3(fwd) * -move.v[2];
			cam_pos = cam_pos + vec3(up) * move.v[1];
			cam_pos = cam_pos + vec3(rgt) * move.v[0];
			mat4 T = translate(identity_mat4(), vec3(cam_pos));

			view_mat = inverse(R) * inverse(T);
		}

		cam_moved = false;
		cam_yaw = 0.0f;
		cam_pitch = 0.0f;
		cam_roll = 0.0;
		move = vec3(0.0, 0.0, 0.0);

		glfwPollEvents();
		glfwSwapBuffers(g_window);
		if (close_window) {
			glfwDestroyWindow(g_window);
		}
	}
}