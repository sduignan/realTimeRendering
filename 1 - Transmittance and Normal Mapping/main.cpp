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
#include "cube_map.h"

#define FRONT "cube_map/negz.jpg"
#define BACK "cube_map/posz.jpg"
#define TOP "cube_map/posy.jpg"
#define BOTTOM "cube_map/negy.jpg"
#define LEFT "cube_map/negx.jpg"
#define RIGHT "cube_map/posx.jpg"

int g_gl_width = 1200;
int g_gl_height = 800;
bool close_window = false;
GLFWwindow* g_window = NULL;

float model_rotation = 0.0f;

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

#define NUM_SHADERS 7
std::string shader_list[] = { "light", "tex", "bump", "reflect", "refract", "fresnel", "both" };

struct shader_info {
	GLuint shader_id;
	int M_loc;
	int V_loc;
	int P_loc;
} shader_infos[NUM_SHADERS];

int sets[3][3] = { { 0, 1, 2 },{ 3, 4, 5 },{ 2, 5, 6 } };

int turn = 1;
int choice = 0;

int text_ids[3][3];

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
				cam_roll -= increment;
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
			case GLFW_KEY_SPACE: {
				//Move on to next set
				choice = (choice+1)%3;
				//Show/hide the text as appropriate
				for (int i = 0; i < 3; i++) {
					unhide_text(text_ids[choice][i]);
					hide_text(text_ids[(choice + 1) % 3][i]);
					hide_text(text_ids[(choice + 2) % 3][i]);
				}
				break;
			}
			case GLFW_KEY_ENTER: {
				//Stop the angels spinning
				turn = (turn+1)%2;
				break;
			}
		}
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
	
	text_ids[0][0] = add_text("Left:\nLighting only", -0.8, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[0][1] = add_text("Centre:\nTexture Mapped", -0.25, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[0][2] = add_text("Right:\nNormal Mapped", 0.35, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[1][0] = add_text("Left:\nReflection", -0.8, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[1][1] = add_text("Centre:\nRefraction", -0.25, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[1][2] = add_text("Right:\nReflection, Refraction,\nFresnel Effect,\n& Chromatic Dispersion", 0.35, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[2][0] = add_text("Left:\nNormal Mapped", -0.8, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[2][1] = add_text("Centre:\nFresnel Effect with\nChromatic Dispersion", -0.25, 0.85, 30, 1.0, 1.0, 0.0, 1.0);
	text_ids[2][2] = add_text("Right:\nNormal Mapped\nwith Fresnel Effect\n& Chromatic Dispersion", 0.35, 0.85, 30, 1.0, 1.0, 0.0, 1.0);

	//Hide all but first set
	for (int i = 1; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			hide_text(text_ids[i][j]);
		}
	}

	// Set up vertex buffers and vertex array objects
	scene_object angel("Meshes/angel.obj", "Textures/angel_diffuse.png", "Textures/angel_normals.png");


	//CUBE MAP
	cube_map sky_cube(FRONT, BACK, TOP, BOTTOM, LEFT, RIGHT);

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
	}

	// cube-map shaders
	sky_cube.shader = create_programme_from_files("Shaders/sky_cube.vert", "Shaders/sky_cube.frag");
	glUseProgram(sky_cube.shader);
	// note that this view matrix should NOT contain camera translation.
	sky_cube.proj_loc = glGetUniformLocation(sky_cube.shader, "P");
	sky_cube.view_loc = glGetUniformLocation(sky_cube.shader, "V");


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
	
	sky_cube.proj = proj_mat;
	sky_cube.view = R;
	sky_cube.update_mats(true, true);

	mat4 angel_model_mat = scale(identity_mat4(), vec3(1.5, 1.5, 1.5));

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // set counter-clock-wise vertex order to mean the front
	glClearColor(0.2, 0.2, 0.2, 1.0); // grey background to help spot mistakes
	glViewport(0, 0, g_gl_width, g_gl_height);

	//MAIN LOOP
	while (!glfwWindowShouldClose(g_window)) {
		// wipe the drawing surface clear
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// render a sky-box using the cube-map texture
		sky_cube.draw();

		//Draw the angel with the shaders in the current set
		for (int i = 0; i < 3; i++) {
			glUseProgram(shader_infos[sets[choice][i]].shader_id);
			float x_displacement = (float)((i - 1) * 2.5);
			mat4 model = translate(rotate_y_deg(angel_model_mat, model_rotation), vec3(x_displacement, -1.75, 0.0));
			glUniformMatrix4fv(shader_infos[sets[choice][i]].M_loc, 1, GL_FALSE, model.m);
			glUniformMatrix4fv(shader_infos[sets[choice][i]].V_loc, 1, GL_FALSE, view_mat.m);
			angel.draw();
		}

		//Rotate the angel
		model_rotation += 0.75f*(float)turn;
		model_rotation = fmod(model_rotation, 360.00);

		//Write scene description on frame
		draw_texts();

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

			// cube-map view matrix has rotation, but not translation
			sky_cube.view = inverse(R);
			sky_cube.update_mats(false, true);
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