///////////////////////////////////////////////////////////////////////
//
// P3D Course
// (c) 2021 by Jo�o Madeiras Pereira
// Ray Tracing P3F scenes and drawing points with Modern OpenGL
//
///////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <chrono>
#include <conio.h>
#include <limits>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <IL/il.h>

#include "scene.h"
#include "rayAccelerator.h"
#include "maths.h"
#include "macros.h"

bool drawModeEnabled = false; // Enable OpenGL drawing.

bool P3F_scene = true; // choose between P3F scene or a built-in random scene

//bool jittering = true; // Enable jittering

#define MAX_DEPTH 4 // number of bounces

#define CAPTION "Whitted Ray-Tracer"
#define VERTEX_COORD_ATTRIB 0
#define COLOR_ATTRIB 1

unsigned int FrameCount = 0;

// Current Camera Position
float camX, camY, camZ;

// Original Camera position;
Vector Eye;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Camera Spherical Coordinates
float alpha = 0.0f, beta = 0.0f;
float r = 4.0f;

// Frame counting and FPS computation
long myTime, timebase = 0, frame = 0;
char s[32];

// Points defined by 2 attributes: positions which are stored in vertices array and colors which are stored in colors array
float* colors;
float* vertices;
int size_vertices;
int size_colors;

// Array of Pixels to be stored in a file by using DevIL library
uint8_t* img_Data;

GLfloat m[16]; // projection matrix initialized by ortho function

GLuint VaoId;
GLuint VboId[2];

GLuint VertexShaderId, FragmentShaderId, ProgramId;
GLint UniformId;

Scene* scene = NULL;

Grid* grid_ptr = NULL;
BVH* bvh_ptr = NULL;
accelerator Accel_Struct;

int RES_X, RES_Y;

int WindowHandle = 0;

/////////////////////////////////////////////////////////////////////// ERRORS

bool isOpenGLError()
{
	bool isError = false;
	GLenum errCode;
	const GLubyte* errString;
	while ((errCode = glGetError()) != GL_NO_ERROR)
	{
		isError = true;
		errString = gluErrorString(errCode);
		std::cerr << "OpenGL ERROR [" << errString << "]." << std::endl;
	}
	return isError;
}

void checkOpenGLError(std::string error)
{
	if (isOpenGLError())
	{
		std::cerr << error << std::endl;
		exit(EXIT_FAILURE);
	}
}

/////////////////////////////////////////////////////////////////////// SHADERs

const GLchar* VertexShader =
{
	"#version 430 core\n"

	"in vec2 in_Position;\n"
	"in vec3 in_Color;\n"
	"uniform mat4 Matrix;\n"
	"out vec4 color;\n"

	"void main(void)\n"
	"{\n"
	"	vec4 position = vec4(in_Position, 0.0, 1.0);\n"
	"	color = vec4(in_Color, 1.0);\n"
	"	gl_Position = Matrix * position;\n"

	"}\n" };

const GLchar* FragmentShader =
{
	"#version 430 core\n"

	"in vec4 color;\n"
	"out vec4 out_Color;\n"

	"void main(void)\n"
	"{\n"
	"	out_Color = color;\n"
	"}\n" };

void createShaderProgram()
{
	VertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(VertexShaderId, 1, &VertexShader, 0);
	glCompileShader(VertexShaderId);

	FragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShaderId, 1, &FragmentShader, 0);
	glCompileShader(FragmentShaderId);

	ProgramId = glCreateProgram();
	glAttachShader(ProgramId, VertexShaderId);
	glAttachShader(ProgramId, FragmentShaderId);

	glBindAttribLocation(ProgramId, VERTEX_COORD_ATTRIB, "in_Position");
	glBindAttribLocation(ProgramId, COLOR_ATTRIB, "in_Color");

	glLinkProgram(ProgramId);
	UniformId = glGetUniformLocation(ProgramId, "Matrix");

	checkOpenGLError("ERROR: Could not create shaders.");
}

void destroyShaderProgram()
{
	glUseProgram(0);
	glDetachShader(ProgramId, VertexShaderId);
	glDetachShader(ProgramId, FragmentShaderId);

	glDeleteShader(FragmentShaderId);
	glDeleteShader(VertexShaderId);
	glDeleteProgram(ProgramId);

	checkOpenGLError("ERROR: Could not destroy shaders.");
}

/////////////////////////////////////////////////////////////////////// VAOs & VBOs

void createBufferObjects()
{
	glGenVertexArrays(1, &VaoId);
	glBindVertexArray(VaoId);
	glGenBuffers(2, VboId);
	glBindBuffer(GL_ARRAY_BUFFER, VboId[0]);

	/* S� se faz a aloca��o dos arrays glBufferData (NULL), e o envio dos pontos para a placa gr�fica
	� feito na drawPoints com GlBufferSubData em tempo de execu��o pois os arrays s�o GL_DYNAMIC_DRAW */
	glBufferData(GL_ARRAY_BUFFER, size_vertices, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(VERTEX_COORD_ATTRIB);
	glVertexAttribPointer(VERTEX_COORD_ATTRIB, 2, GL_FLOAT, 0, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, VboId[1]);
	glBufferData(GL_ARRAY_BUFFER, size_colors, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(COLOR_ATTRIB);
	glVertexAttribPointer(COLOR_ATTRIB, 3, GL_FLOAT, 0, 0, 0);

	// unbind the VAO
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//	glDisableVertexAttribArray(VERTEX_COORD_ATTRIB);
	//	glDisableVertexAttribArray(COLOR_ATTRIB);
	checkOpenGLError("ERROR: Could not create VAOs and VBOs.");
}

void destroyBufferObjects()
{
	glDisableVertexAttribArray(VERTEX_COORD_ATTRIB);
	glDisableVertexAttribArray(COLOR_ATTRIB);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDeleteBuffers(1, VboId);
	glDeleteVertexArrays(1, &VaoId);
	checkOpenGLError("ERROR: Could not destroy VAOs and VBOs.");
}

void drawPoints()
{
	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(VaoId);
	glUseProgram(ProgramId);

	glBindBuffer(GL_ARRAY_BUFFER, VboId[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size_vertices, vertices);
	glBindBuffer(GL_ARRAY_BUFFER, VboId[1]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, size_colors, colors);

	glUniformMatrix4fv(UniformId, 1, GL_FALSE, m);
	glDrawArrays(GL_POINTS, 0, RES_X * RES_Y);
	glFinish();

	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	checkOpenGLError("ERROR: Could not draw scene.");
}

ILuint saveImgFile(const char* filename)
{
	ILuint ImageId;

	ilEnable(IL_FILE_OVERWRITE);
	ilGenImages(1, &ImageId);
	ilBindImage(ImageId);

	ilTexImage(RES_X, RES_Y, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, img_Data /*Texture*/);
	ilSaveImage(filename);

	ilDisable(IL_FILE_OVERWRITE);
	ilDeleteImages(1, &ImageId);
	if (ilGetError() != IL_NO_ERROR)
		return ilGetError();

	return IL_NO_ERROR;
}

/////////////////////////////////////////////////////////////////////// CALLBACKS

void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << RES_X << "x" << RES_Y << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
	FrameCount = 0;
	glutTimerFunc(1000, timer, 0);
}

// Callback function for glutCloseFunc
void cleanup()
{
	destroyShaderProgram();
	destroyBufferObjects();
}

void ortho(float left, float right, float bottom, float top,
	float nearp, float farp)
{
	m[0 * 4 + 0] = 2 / (right - left);
	m[0 * 4 + 1] = 0.0;
	m[0 * 4 + 2] = 0.0;
	m[0 * 4 + 3] = 0.0;
	m[1 * 4 + 0] = 0.0;
	m[1 * 4 + 1] = 2 / (top - bottom);
	m[1 * 4 + 2] = 0.0;
	m[1 * 4 + 3] = 0.0;
	m[2 * 4 + 0] = 0.0;
	m[2 * 4 + 1] = 0.0;
	m[2 * 4 + 2] = -2 / (farp - nearp);
	m[2 * 4 + 3] = 0.0;
	m[3 * 4 + 0] = -(right + left) / (right - left);
	m[3 * 4 + 1] = -(top + bottom) / (top - bottom);
	m[3 * 4 + 2] = -(farp + nearp) / (farp - nearp);
	m[3 * 4 + 3] = 1.0;
}

void reshape(int w, int h)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, w, h);
	ortho(0, (float)RES_X, 0, (float)RES_Y, -1.0, 1.0);
}

void processKeys(unsigned char key, int xx, int yy)
{
	switch (key)
	{

	case 27:
		glutLeaveMainLoop();
		break;

	case 'r':
		camX = Eye.x;
		camY = Eye.y;
		camZ = Eye.z;
		r = Eye.length();
		beta = asinf(camY / r) * 180.0f / 3.14f;
		alpha = atanf(camX / camZ) * 180.0f / 3.14f;
		break;

	case 'c':
		printf("Camera Spherical Coordinates (%f, %f, %f)\n", r, beta, alpha);
		printf("Camera Cartesian Coordinates (%f, %f, %f)\n", camX, camY, camZ);
		break;
	}
}

// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)
	{
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	// stop tracking the mouse
	else if (state == GLUT_UP)
	{
		if (tracking == 1)
		{
			alpha -= (xx - startX);
			beta += (yy - startY);
		}
		else if (tracking == 2)
		{
			r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX = -xx + startX;
	deltaY = yy - startY;

	// left mouse button: move camera
	if (tracking == 1)
	{

		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2)
	{

		alphaAux = alpha;
		betaAux = beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;
	}

	camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	camY = rAux * sin(betaAux * 3.14f / 180.0f);
}

void mouseWheel(int wheel, int direction, int x, int y)
{

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(beta * 3.14f / 180.0f);
	camY = r * sin(beta * 3.14f / 180.0f);
}

void setupGLEW()
{
	glewExperimental = GL_TRUE;
	GLenum result = glewInit();
	if (result != GLEW_OK)
	{
		std::cerr << "ERROR glewInit: " << glewGetString(result) << std::endl;
		exit(EXIT_FAILURE);
	}
	GLenum err_code = glGetError();
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void setupGLUT(int argc, char* argv[])
{
	glutInit(&argc, argv);

	glutInitContextVersion(4, 3);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	glutInitWindowPosition(100, 250);
	glutInitWindowSize(RES_X, RES_Y);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glDisable(GL_DEPTH_TEST);
	WindowHandle = glutCreateWindow(CAPTION);
	if (WindowHandle < 1)
	{
		std::cerr << "ERROR: Could not create a new rendering window." << std::endl;
		exit(EXIT_FAILURE);
	}
}

float get_rand(float min, float max) {
	return min + rand_float() * (max - min);
}

Vector rand_in_unit_sphere() {

	float theta = rand_float() * 2.0 * PI;
	float phi = rand_float() * 2.0 * PI;
	float r = rand_float();

	float x = r * sin(theta) * cos(phi);
	float y = r * sin(theta) * sin(phi);
	float z = r * cos(theta);

	return Vector(x, y, z);
}

Vector rand_in_unit_circle() {

	float theta = rand_float() * 2.0 * PI;
	float r = rand_float();
	float x = r * cos(theta);
	float y = r * sin(theta);

	return Vector(x, y, 0);
}

Color getDiffuseNSpecular(Ray shadow_ray, Material* material, Vector hit_ray_dir, Vector normal_vec, Vector light_dir, Color light_colour, float light_normal_dot_prod) {
	Color colour;
	bool in_shadow = false;
	float hit_dist;

	if (Accel_Struct == GRID_ACC) {
		in_shadow = grid_ptr->Traverse(shadow_ray);
	}
	else if (Accel_Struct == BVH_ACC) {
		in_shadow = bvh_ptr->Traverse(shadow_ray);
	}
	else {
		float max_dist = shadow_ray.direction.length();

		for (int j = 0; j < scene->getNumObjects(); j++) {
			if (scene->getObject(j)->intercepts(shadow_ray, hit_dist) && hit_dist < max_dist) {
				in_shadow = true;
				break;
			}
		}
	}

	if (in_shadow)
		return colour;

	Color diffuse_colour = material->GetDiffColor() * material->GetDiffuse() * light_normal_dot_prod;

	float specular = material->GetSpecular();

	Color specular_colour;
	if (specular > 0) {
		// Halfway vector approximation
		Vector halfway_vec = (light_dir + hit_ray_dir).normalize();

		float halfway_product = halfway_vec * normal_vec;

		if (halfway_product > 0) {
			specular_colour = material->GetSpecColor() * specular * pow(halfway_product, material->GetShine());
		}
	}

	colour += light_colour * (diffuse_colour + specular_colour);
	return colour;
}

Color rayTracing(Ray ray, int depth, float ior_i);

Color getReflection(Vector normal_vec, float cos_theta_i, Vector rev_ray_dir, Vector hit_point,
	Material* material, int depth, float ior_i) {

	Vector refl_ray_dir = normal_vec * cos_theta_i * 2 - rev_ray_dir;
	float roughness = material->GetRoughness();
	int spp = scene->GetSamplesPerPixel();
	Color colour;

	if (roughness > 0) { // Fuzzy reflections
	
		// Anti-aliasing already shoots multiple rays,
		// so we only need to shoot more than one here
		// if anti-aliasing is deactivated
		int sqrt_num_samples = (spp == 0) ? 2 : 1;
		Vector mod_refl_ray_dir;

		for (int p = 0; p < sqrt_num_samples; p++) {
			for (int q = 0; q < sqrt_num_samples; q++) {
				mod_refl_ray_dir = (refl_ray_dir + rand_in_unit_sphere() * roughness).normalize(); // our implementation of random
				//mod_refl_ray_dir = (refl_ray_dir + rnd_unit_sphere() * roughness).normalize();
				
				if (mod_refl_ray_dir * normal_vec < 0) {
					continue;
				}

				Ray refl_ray(hit_point, mod_refl_ray_dir);

				Color refl_colour = rayTracing(refl_ray, depth + 1, ior_i);
				colour += material->GetSpecColor() * refl_colour * material->GetReflection();
			}
		}

		return colour * (1.0 / pow(sqrt_num_samples, 2)); // Average the added colours
	}

	Ray refl_ray(hit_point, refl_ray_dir);

	Color refl_colour = rayTracing(refl_ray, depth + 1, ior_i);
	refl_colour = material->GetSpecColor() * refl_colour * material->GetReflection();

	return refl_colour;
}

Color getRefraction(Vector hit_point, Vector normal_vec, Vector tangent_vec, float sin_theta_t, 
	float cos_theta_t, Material* material, int depth, float ior_t, float Kr) {

	Vector refr_hit_point = hit_point - normal_vec * EPSILON;
	Vector refr_ray_dir = tangent_vec * sin_theta_t - normal_vec * cos_theta_t;
	float roughness = material->GetRoughness();
	int spp = scene->GetSamplesPerPixel();
	Color colour;

	// THIS DOES NOT WORK!!!
	// if (roughness > 0) { // Fuzzy refractions
	// 	// Anti-aliasing already shoots multiple rays,
	// 	// so we only need to shoot more than one here
	// 	// if anti-aliasing is deactivated
	// 	int sqrt_num_samples = (spp == 0) ? 2 : 1;
	// 	Vector mod_refr_ray_dir;

	// 	for (int p = 0; p < sqrt_num_samples; p++) {
	// 		for (int q = 0; q < sqrt_num_samples; q++) {
	// 			mod_refr_ray_dir = (refr_ray_dir + rand_in_unit_sphere() * roughness).normalize(); // our implementation of random

	// 			Ray refr_ray(refr_hit_point, mod_refr_ray_dir);

	// 			colour += rayTracing(refr_ray, depth + 1, ior_t);
	// 		}
	// 	}

	// 	return colour * (1 - Kr) * (1.0 / pow(sqrt_num_samples, 2)); // Average the added colours
	// }

	Ray refr_ray(refr_hit_point, refr_ray_dir);
	Color refr_colour = rayTracing(refr_ray, depth + 1, ior_t);
	return refr_colour * (1 - Kr);
}

Color rayTracing(Ray ray, int depth, float ior_i) // index of refraction of medium 1 where the ray is travelling
{
	Color colour = Color();
	float hit_dist, shortest_hit_dist = std::numeric_limits<float>::max();
	Object* shortest_hit_object = nullptr;
	Vector hit_point;
	bool skybox_flg = scene->GetSkyBoxFlg();
	bool hit = false;

	if (Accel_Struct == NONE) { //no acceleration; your code here}
	
		for (int i = 0; i < scene->getNumObjects(); i++) {
			Object* object = scene->getObject(i); 
			if (object->intercepts(ray, hit_dist) && hit_dist < shortest_hit_dist) {
				hit = true;
				shortest_hit_dist = hit_dist;
				shortest_hit_object = object;
			}
		}

		hit_point = ray.origin + ray.direction * shortest_hit_dist;
	}
	else if (Accel_Struct == GRID_ACC) { // regular Grid
		hit = grid_ptr->Traverse(ray, &shortest_hit_object, hit_point);

	}
	else if (Accel_Struct == BVH_ACC) { //BVH
		hit = bvh_ptr->Traverse(ray, &shortest_hit_object, hit_point);
	}

	if (!hit) {
		if (skybox_flg)
			colour = scene->GetSkyboxColor(ray);
		else
			colour = scene->GetBackgroundColor();
		return colour;
	}
		
	Material* material = shortest_hit_object->GetMaterial();

	Vector rev_ray_dir = ray.direction * (-1);

	// Negate normal vector's direction if the ray comes from inside the object
	Vector normal_vec = shortest_hit_object->getShadingNormal(rev_ray_dir, hit_point);

	// To account for acne spots
	Vector refl_hit_point = hit_point + normal_vec * EPSILON;
	
	float sqrt_spl;

	for (int i = 0; i < scene->getNumLights(); i++)
	{
		Light* light = scene->getLight(i);
		Vector light_dir;
		sqrt_spl = sqrt(light->spl);
		
		if (scene->GetSamplesPerPixel() == 0) {

			if (light->spl == 0) {

				light_dir = light->position - hit_point;

				float light_normal_dot_product = (light_dir / light_dir.length()) * normal_vec;

				if (light_normal_dot_product <= 0)
					continue;

				Ray shadow_ray(refl_hit_point, light_dir);

				//WARNING: added a coefficient to light color to make it less bright
				colour += getDiffuseNSpecular(shadow_ray, material, rev_ray_dir, normal_vec, light_dir.normalize(), light->color * 0.6, light_normal_dot_product);

			} else {
				for (int w = 0; w < sqrt_spl; w++) {
					for (int h = 0; h < light->spl / sqrt_spl; h++) {

						float e = rand_float();

						//WARNING: added random variation as in anti-aliasing jittering
						Vector light_pos = Vector(light->position.x + (w + e) * (light->width / sqrt_spl), light->position.y, light->position.z + (h + e) * (light->height / sqrt_spl));
						light_dir = light_pos - hit_point;

						float light_normal_dot_product = (light_dir / light_dir.length()) * normal_vec;

						if (light_normal_dot_product <= 0)
							continue;

						Ray shadow_ray(refl_hit_point, light_dir);

						//WARNING: added a coefficient to light color to make it less bright
						Color light_color = light->color * light->getPointIntensity();
						colour += getDiffuseNSpecular(shadow_ray, material, rev_ray_dir, normal_vec, light_dir.normalize(), light_color * 0.6, light_normal_dot_product);
					}
				}
			}
		}
		else {
			
			//with jittering
			Vector light_point = light->spl == 0 ? light->position : light->getRandomLightPoint();

			light_dir = light_point - hit_point;

			float light_normal_dot_product = (light_dir / light_dir.length()) * normal_vec;

			if (light_normal_dot_product <= 0)
				continue;

			Ray shadow_ray(refl_hit_point, light_dir);

			//WARNING: added a coefficient to light color to make it less bright
			colour += getDiffuseNSpecular(shadow_ray, material, rev_ray_dir, normal_vec, light_dir.normalize(), light->color * 0.6, light_normal_dot_product);
		}
	}

	if (depth >= MAX_DEPTH || (material->GetTransmittance() == 0 && material->GetReflection() == 0))
		return colour;

	//--------------------------------------------Only Reflective-------------------------------

	float cos_theta_i = normal_vec * rev_ray_dir; // Because both vectors are normalised

	if (material->GetTransmittance() == 0)
		return colour + getReflection(normal_vec, cos_theta_i, rev_ray_dir, refl_hit_point, material, depth, ior_i);

	//-----------------------------Dielectric (reflection + refraction)--------------------------

	Vector tangent_vec = normal_vec * cos_theta_i - rev_ray_dir;
	float sin_theta_i = tangent_vec.length();
	tangent_vec.normalize();

	// Index of refraction of new medium where the ray will travel
	float ior_t = material->GetRefrIndex();

	float sin_theta_t = sin_theta_i * ior_i / ior_t;

	//Total Reflection
	if (sin_theta_t > 1)
		return colour + getReflection(normal_vec, cos_theta_i, rev_ray_dir, refl_hit_point, material, depth, ior_i);

	float cos_theta_t = sqrt(1 - pow(sin_theta_t, 2));

	//Schlick's approximation
	float R0 = pow((ior_i - ior_t) / (ior_i + ior_t), 2);

	float cos_theta_Kr = ior_i > ior_t ? cos_theta_t : cos_theta_i;
	float Kr = R0 + (1 - R0) * pow(1 - cos_theta_Kr, 5);

	// Reflection --> only if object isn't diffuse
	// TODO: Fuzzy Reflections needed here?
	if (material->GetReflection() > 0) {
		Vector refl_ray_dir = normal_vec * cos_theta_i * 2 - rev_ray_dir;
		Ray reflected_ray(refl_hit_point, refl_ray_dir);

		Color reflected_colour = rayTracing(reflected_ray, depth + 1, ior_i);
		colour += material->GetSpecColor() * reflected_colour * Kr;
	}

	return colour + getRefraction(hit_point, normal_vec, tangent_vec, sin_theta_t, cos_theta_t, material, depth, ior_t, Kr);
}

// Render function by primary ray casting from the eye towards the scene's objects
void renderScene()
{
	int index_pos = 0;
	int index_col = 0;
	unsigned int counter = 0;
	float sqrt_spp = sqrt(scene->GetSamplesPerPixel());
	Camera* camera = scene->GetCamera();
	Accel_Struct = scene->GetAccelStruct();

	// viewport coordinates
	Vector pixel_sample, lens_sample;
	float aperture = camera->GetAperture();

	set_rand_seed(time(NULL) * time(NULL));

	if (drawModeEnabled)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		camera->SetEye(Vector(camX, camY, camZ)); // Camera motion
	}

	for (int y = 0; y < RES_Y; y++)
	{
		for (int x = 0; x < RES_X; x++)
		{
			Color color = Color();

			if (aperture <= 0) { // No depth of field
				if (sqrt_spp == 0) { // No anti-aliasing => Only one pixel sample!

					pixel_sample.x = x + 0.5f;
					pixel_sample.y = y + 0.5f;

					Ray ray = camera->PrimaryRay(pixel_sample); // function from camera.h
					color = rayTracing(ray, 1, 1.0);
				}
				else { // Anti-aliasing => Average each ray's colour
					for (int p = 0; p < sqrt_spp; p++) {
						for (int q = 0; q < sqrt_spp; q++) {

							pixel_sample.x = x + get_rand(p, p + 1) / sqrt_spp;
							pixel_sample.y = y + get_rand(q, q + 1) / sqrt_spp;

							Ray ray = camera->PrimaryRay(pixel_sample);
							color += rayTracing(ray, 1, 1.0);
						}
					}

					color = color * (1 / pow(sqrt_spp, 2));
				}
			}
			else { // Add depth of field
				int sqrt_spp_dof;
				if (sqrt_spp == 0) { // No anti-aliasing => Only one pixel sample!
					sqrt_spp_dof = 2;
					pixel_sample.x = x + 0.5f;
					pixel_sample.y = y + 0.5f;
				}
				else {
					sqrt_spp_dof = sqrt_spp;
				}

				// Average each ray's colour
				for (int p = 0; p < sqrt_spp_dof; p++) {
					for (int q = 0; q < sqrt_spp_dof; q++) {
						lens_sample = rand_in_unit_circle() * aperture; //our implementation of random
						//lens_sample = rnd_unit_disk() * aperture;
						
						if (sqrt_spp != 0) { // Anti-aliasing => Each pixel sample is different
							pixel_sample.x = x + get_rand(p, p + 1) / sqrt_spp_dof;
							pixel_sample.y = y + get_rand(p, p + 1) / sqrt_spp_dof;
							
						}

						Ray ray = camera->PrimaryRay(lens_sample, pixel_sample);
						Color a = rayTracing(ray, 1, 1.0);
						color += a;
						
					}
				}

				color = color * (1 / pow(sqrt_spp_dof, 2));
			}
			color = color.clamp();
			
			img_Data[counter++] = u8fromfloat((float)color.r());
			img_Data[counter++] = u8fromfloat((float)color.g());
			img_Data[counter++] = u8fromfloat((float)color.b());

			if (drawModeEnabled)
			{
				vertices[index_pos++] = (float)x;
				vertices[index_pos++] = (float)y;
				colors[index_col++] = (float)color.r();

				colors[index_col++] = (float)color.g();

				colors[index_col++] = (float)color.b();
			}
		}
	}
	if (drawModeEnabled)
	{
		drawPoints();
		glutSwapBuffers();
	}
	else
	{
		printf("Terminou o desenho!\n");
		if (saveImgFile("RT_Output.png") != IL_NO_ERROR)
		{
			printf("Error saving Image file\n");
			exit(0);
		}
		printf("Image file created\n");
	}
}

///////////////////////////////////////////////////////////////////////  SETUP     ///////////////////////////////////////////////////////

void setupCallbacks()
{
	glutKeyboardFunc(processKeys);
	glutCloseFunc(cleanup);
	glutDisplayFunc(renderScene);
	glutReshapeFunc(reshape);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc(mouseWheel);

	glutIdleFunc(renderScene);
	glutTimerFunc(0, timer, 0);
}
void init(int argc, char* argv[])
{
	// set the initial camera position on its spherical coordinates
	Eye = scene->GetCamera()->GetEye();
	camX = Eye.x;
	camY = Eye.y;
	camZ = Eye.z;
	r = Eye.length();
	beta = asinf(camY / r) * 180.0f / 3.14f;
	alpha = atanf(camX / camZ) * 180.0f / 3.14f;

	setupGLUT(argc, argv);
	setupGLEW();
	std::cerr << "CONTEXT: OpenGL v" << glGetString(GL_VERSION) << std::endl;
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	createShaderProgram();
	createBufferObjects();
	setupCallbacks();
}

void init_scene(void)
{
	char scenes_dir[70] = "P3D_Scenes/";
	char input_user[50];
	char scene_name[70];

	scene = new Scene();

	if (P3F_scene)
	{ // Loading a P3F scene

		while (true)
		{
			cout << "Input the Scene Name: ";
			cin >> input_user;
			strcpy_s(scene_name, sizeof(scene_name), scenes_dir);
			strcat_s(scene_name, sizeof(scene_name), input_user);

			ifstream file(scene_name, ios::in);
			if (file.fail())
			{
				printf("\nError opening P3F file.\n");
			}
			else
				break;
		}

		scene->load_p3f(scene_name);
		printf("Scene loaded.\n\n");
	}
	else
	{
		printf("Creating a Random Scene.\n\n");
		scene->create_random_scene();
	}

	RES_X = scene->GetCamera()->GetResX();
	RES_Y = scene->GetCamera()->GetResY();
	printf("\nResolutionX = %d  ResolutionY= %d.\n", RES_X, RES_Y);

	// Pixel buffer to be used in the Save Image function
	img_Data = (uint8_t*)malloc(3 * RES_X * RES_Y * sizeof(uint8_t));
	if (img_Data == NULL)
		exit(1);

	Accel_Struct = scene->GetAccelStruct(); // Type of acceleration data structure

	if (Accel_Struct == GRID_ACC)
	{
		grid_ptr = new Grid();
		vector<Object*> objs;
		int num_objects = scene->getNumObjects();

		for (int o = 0; o < num_objects; o++)
		{
			objs.push_back(scene->getObject(o));
		}
		grid_ptr->Build(objs);
		printf("Grid built.\n\n");
	}
	else if (Accel_Struct == BVH_ACC)
	{
		vector<Object*> objs;
		int num_objects = scene->getNumObjects();
		bvh_ptr = new BVH();

		for (int o = 0; o < num_objects; o++)
		{
			objs.push_back(scene->getObject(o));
		}

		bvh_ptr->Build(objs);
		printf("BVH built.\n\n");
	}
	else
		printf("No acceleration data structure.\n\n");

	unsigned int spp = scene->GetSamplesPerPixel();
	if (spp == 0)
		printf("Whitted Ray-Tracing\n");
	else
		printf("Distribution Ray-Tracing\n");
}

int main(int argc, char* argv[])
{
	// Initialization of DevIL
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	int ch;
	if (!drawModeEnabled) {
		do {
			init_scene();

			auto timeStart = std::chrono::high_resolution_clock::now();
			renderScene(); // Just creating an image file
			auto timeEnd = std::chrono::high_resolution_clock::now();
			auto passedTime = std::chrono::duration<double, std::milli>(timeEnd - timeStart).count();
			printf("\nDone: %.2f (sec)\n", passedTime / 1000);
			if (!P3F_scene)
				break;
			cout << "\nPress 'y' to render another image or another key to terminate!\n";
			delete (scene);
			free(img_Data);
			ch = _getch();
		} while ((toupper(ch) == 'Y'));
	}

	else
	{ // Use OpenGL to draw image in the screen
		printf("OPENGL DRAWING MODE\n\n");
		init_scene();
		size_vertices = 2 * RES_X * RES_Y * sizeof(float);
		size_colors = 3 * RES_X * RES_Y * sizeof(float);
		vertices = (float*)malloc(size_vertices);
		if (vertices == NULL)
			exit(1);
		colors = (float*)malloc(size_colors);
		if (colors == NULL)
			exit(1);
		memset(colors, 0, size_colors);

		/* Setup GLUT and GLEW */
		init(argc, argv);
		glutMainLoop();
	}

	free(colors);
	free(vertices);
	printf("Program ended normally\n");
	exit(EXIT_SUCCESS);
}
///////////////////////////////////////////////////////////////////////