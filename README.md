# realTimeRendering
Collection of assignment projects for Real-Time Rendering module

Folders 1 and 2 contain source code directly related to the real-time rendering projects indicated by the folder names. The main focus of these projects was the shaders written to implement different rendering styles or techniques.

The folder labelled "Third Party Files" contains various files that I have not written but which I have used to implement functionality such as writing text to screen, or vector and matrix mathematics.

Project Descriptions:

1 - Transmittance and Normal Mapping

Implementations of reflection, refraction, chromatic dispersion, the Frensel effect, normal mapping, and the combination of all the effects.

Demo video: https://youtu.be/qmydj3dCwRk


2 - Charcoal Rendering

This project was inspired by the paper "Charcoal rendering and shading with reflections" byYuxiao Du and Ergun Ackleman. It implements a charcoal drawing rendering style using the following techniques: Oren-Nayar lighting, tri-planar texture projection, texture interpolation, shadow mapping, Gaussian blurring for softer shadows, variable thickness silhouetting, and multisample anti-aliasing.

Demo video: https://youtu.be/U8MOhJI9qpY 

External Libraries used:

• Antons_maths_funcs: a library for vector, matrix and quaternion mathematics.

• gl_utils: (slightly modiﬁed), a library of common OpenGL functionality.

• text: a library for writing text on-screen in an OpengGL application.

• obj_parser: to provide mesh loading functionality.

• stb_image: an image loading library.

• assimp: for loading 3D meshes.

• glew: the OpenGL extension wrangler.

• glfw: an OpenGL development framework.

