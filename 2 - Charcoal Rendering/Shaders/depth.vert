#version 410

in vec3 vp;
uniform mat4 P, V, M;

//Tiny little shader for shadow mapping
void main () {
  gl_Position = P * V * M * vec4 (vp, 1.0);
}