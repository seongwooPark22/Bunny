#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#include <GL/glu.h>
#include <GL/glext.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <sstream>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stdio.h>
#include <string.h>
#include <string>
#include <queue>
#include <float.h>

int WIDTH = 1280;
int HEIGHT = 1280;

float  					gTotalTimeElapsed = 0;
int 					gTotalFrames = 0;
GLuint 					gTimer;

struct Triangle
{
	unsigned int 	indices[3];
};

std::vector<Triangle>	gTriangles;
std::vector<glm::vec3> vertex_position_buffer;
std::vector<glm::vec3> vertex_normal_buffer;

float PI = 3.141592f;

using namespace glm;

void tokenize(char* string, std::vector<std::string>& tokens, const char* delimiter)
{
	char* token = strtok(string, delimiter);
	while (token != NULL)
	{
		tokens.push_back(std::string(token));
		token = strtok(NULL, delimiter);
	}
}

int face_index(const char* string)
{
	int length = strlen(string);
	char* copy = new char[length + 1];
	memset(copy, 0, length + 1);
	strcpy(copy, string);

	std::vector<std::string> tokens;
	tokenize(copy, tokens, "/");
	delete[] copy;
	if (tokens.front().length() > 0 && tokens.back().length() > 0 && atoi(tokens.front().c_str()) == atoi(tokens.back().c_str()))
	{
		return atoi(tokens.front().c_str());
	}
	else
	{
		printf("ERROR: Bad face specifier!\n");
		exit(0);
	}
}

void load_mesh(std::string fileName)
{
	std::ifstream fin(fileName.c_str());
	if (!fin.is_open())
	{
		printf("ERROR: Unable to load mesh from %s!\n", fileName.c_str());
		exit(0);
	}

	float xmin = FLT_MAX;
	float xmax = -FLT_MAX;
	float ymin = FLT_MAX;
	float ymax = -FLT_MAX;
	float zmin = FLT_MAX;
	float zmax = -FLT_MAX;

	while (true)
	{
		char line[1024] = { 0 };
		fin.getline(line, 1024);

		if (fin.eof())
			break;

		if (strlen(line) <= 1)
			continue;

		std::vector<std::string> tokens;
		tokenize(line, tokens, " ");

		if (tokens[0] == "v")
		{
			float x = atof(tokens[1].c_str());
			float y = atof(tokens[2].c_str());
			float z = atof(tokens[3].c_str());

			xmin = std::min(x, xmin);
			xmax = std::max(x, xmax);
			ymin = std::min(y, ymin);
			ymax = std::max(y, ymax);
			zmin = std::min(z, zmin);
			zmax = std::max(z, zmax);

			vertex_position_buffer.push_back(glm::vec3(x, y, z));
		}
		else if (tokens[0] == "vn")
		{
			float x = atof(tokens[1].c_str());
			float y = atof(tokens[2].c_str());
			float z = atof(tokens[3].c_str());

			vertex_normal_buffer.push_back(glm::vec3(x, y, z));
		}
		else if (tokens[0] == "f")
		{
			unsigned int a = face_index(tokens[1].c_str());
			unsigned int b = face_index(tokens[2].c_str());
			unsigned int c = face_index(tokens[3].c_str());

			Triangle triangle;
			triangle.indices[0] = a - 1;
			triangle.indices[1] = b - 1;
			triangle.indices[2] = c - 1;
			gTriangles.push_back(triangle);
		}
	}

	fin.close();

	printf("Loaded mesh from %s. (%lu vertices, %lu normals, %lu triangles)\n", fileName.c_str(), vertex_position_buffer.size(), vertex_normal_buffer.size(), gTriangles.size());
	printf("Mesh bounding box is: (%0.4f, %0.4f, %0.4f) to (%0.4f, %0.4f, %0.4f)\n", xmin, ymin, zmin, xmax, ymax, zmax);
}

void init_timer()
{
	glGenQueries(1, &gTimer);
}

void start_timing()
{
	glBeginQuery(GL_TIME_ELAPSED, gTimer);
}

float stop_timing()
{
	glEndQuery(GL_TIME_ELAPSED);

	GLint available = GL_FALSE;
	while (available == GL_FALSE)
		glGetQueryObjectiv(gTimer, GL_QUERY_RESULT_AVAILABLE, &available);

	GLint result;
	glGetQueryObjectiv(gTimer, GL_QUERY_RESULT, &result);

	float timeElapsed = result / (1000.0f * 1000.0f * 1000.0f);
	return timeElapsed;
}

void resize_callback(GLFWwindow*, int nw, int nh) 
{
	WIDTH = nw;
	HEIGHT = nh;

	glViewport(0, 0, nw, nh);
}

void set_material(glm::vec4 ka, glm::vec4 kd, glm::vec4 ks, float specular_power) {
	glMaterialfv(GL_FRONT, GL_AMBIENT, &ka[0]);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &kd[0]);
	glMaterialfv(GL_FRONT, GL_SPECULAR, &ks[0]);
	glMaterialf(GL_FRONT, GL_SHININESS, specular_power);
}


int main(int argc, char* argv[])
{
	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(WIDTH, HEIGHT, "Q2-OpenGL Bunny", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, WIDTH, HEIGHT);

	glewExperimental = true;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		printf("SOMETHING IS WRONG\n");
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glDepthFunc(GL_LESS);

	init_timer();

	load_mesh("bunny.obj");

	std::vector<float> v_buffer;
	for (int vertex_ind = 0; vertex_ind < vertex_position_buffer.size(); vertex_ind++) {
		glm::vec3 v = vertex_position_buffer[vertex_ind];
		glm::vec3 v_n = vertex_normal_buffer[vertex_ind];

		v_n = glm::normalize(v_n);

		v_buffer.push_back(v.x);
		v_buffer.push_back(v.y);
		v_buffer.push_back(v.z);
		v_buffer.push_back(v_n.x);
		v_buffer.push_back(v_n.y);
		v_buffer.push_back(v_n.z);
	}

	std::vector<int> indices;
	for (int triangle_ind = 0; triangle_ind < gTriangles.size(); triangle_ind++) {
		indices.push_back(gTriangles[triangle_ind].indices[0]);
		indices.push_back(gTriangles[triangle_ind].indices[1]);
		indices.push_back(gTriangles[triangle_ind].indices[2]);
	}

	unsigned int VBO, VAO, EBO;

	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(v_buffer[0]) * v_buffer.size(), &v_buffer[0], GL_STATIC_DRAW);

	glVertexPointer(3, GL_FLOAT, 6 * sizeof(float), (void*)0);
	glNormalPointer(GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(float) * indices.size(), &indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	float light_pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	float Ia[] = { 0.2f, 0.2f, 0.2f, 0.0f };
	float la[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float ld[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	float ls[] = { 1.0f, 1.0f, 1.0f, 0.0f };

	set_material(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Ia);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, la);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, ld);
	glLightfv(GL_LIGHT0, GL_SPECULAR, ls);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//Camera
	gluLookAt(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	//Model
	float scale = 10.0f;
	glTranslatef(0.1f, -1.0f, -1.5f);
	glScalef(scale, scale, scale);

	//Projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float l, r, b, t, n, f;
	l = -0.1f; r = 0.1f; b = -0.1f, t = 0.1f, n = 0.1f, f = 1000.0f;
	glFrustum(l, r, b, t, n, f);
	glViewport(0, 0, WIDTH, HEIGHT);

	float avg_fps = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		start_timing();

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		float timeElapsed = stop_timing();
		gTotalFrames++;
		gTotalTimeElapsed += timeElapsed;
		float fps = gTotalFrames / gTotalTimeElapsed;
		char string[1024] = { 0 };
		sprintf(string, "Q2-OpenGL Bunny: %0.2f FPS", fps);
		avg_fps += fps;
		glfwSetWindowTitle(window, string);

		glfwSwapBuffers(window);
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	avg_fps = avg_fps / (float)gTotalFrames;
	printf("\nTotal Frame Passed : %d\nQ2 Average FPS : %f\n", gTotalFrames, avg_fps);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

