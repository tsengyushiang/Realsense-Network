#pragma once

#include "imgui.h"
#include "./Imgui/ImGuiFileDialog/ImGuiFileDialog.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

#include <GL/gl3w.h>            // Initialize with gl3wInit()
#include <GLFW/glfw3.h>         // Include glfw3.h after our OpenGL definitions

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>

class GLShader {

	static bool check_shader_compile_status(GLuint obj);
	static bool check_program_link_status(GLuint obj);

	static GLuint compileAndLink(
		std::string vertex_source, std::string fragment_source,
		GLuint& shader_program, GLuint& vertex_shader, GLuint& fragment_shader, GLFWwindow* window);

	static void loadshaders();
	static std::map<std::string, std::string> shaderLibs;

public:

	static GLuint genShaderProgram(GLFWwindow* window, std::string vertexshader, std::string fragmentshdaer);
};

class GLFrameBuffer {

	int width, height;
	unsigned int framebuffer;
	unsigned int rbo;

public :
	static void createFrameBuffer(GLuint* framebuffer, GLuint* texColorBuffer, GLuint* depthBuffer, GLuint* rbo, int w, int h);

	unsigned int texColorBuffer;
	unsigned int depthBuffer;

	GLFrameBuffer(int w, int h);
	void render(std::function<void()> callback, int CULLINGTYPE = GL_FRONT);
	unsigned char* getRawColorData();
	float* getRawDepthData();
};

class ImguiOpeGL3App
{
public:

	//---------------------following method implement in ImguiOpeGL3App.cpp

	GLFrameBuffer* main;
	int width, height;
	GLFWwindow* window;
	void initImguiOpenGL3(int width = 1920, int height = 1080);
	~ImguiOpeGL3App() {
		if (main)free(main);
		if (window) free(window);
	};
	virtual void initGL();
	virtual void mainloop();
	virtual void framebufferRender();
	virtual void addGui();
	virtual void addMenu();
	//return false to block mouse drag viewport
	virtual bool addOpenGLPanelGui();
	virtual void mousedrag(float,float);
	virtual void onBeforeRender();
	virtual void onAfterRender();
	static void glfw_error_callback(int error, const char* description);

	float time = 0;

	// opengl camera
	void setcamera(float width, float height);
	float fov = 60;
	float distance = 3;
	float distancemin = 0;
	float distanceMax = 5;
	float PolarAngle = 1.57;
	float PolarAngleMax = 3.0;
	float PolarAnglemin = 0.1;
	float AzimuthAngle = 0.1;
	float AzimuthAngleMax = 6.28;
	float AzimuthAnglemin = 0;
	float autoRotateSpeed = 0.0;

	float sensity = 5e-2;
	glm::vec3 lookAtPoint;
	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;

	//---------------------following method implement in ImguiOpeGL3App_gl.cpp

	static void setTexture(GLuint& image,const unsigned char* vertexData, int width, int height);
	
	static void setTrianglesVAOIBO(GLuint& vao, GLuint& vbo, GLuint& ibo, GLfloat* vertexData, int vertexSize,unsigned int* indices,int indicesSize);
	static void renderElements(glm::mat4& mvp, float pointsize, GLuint shader_program, GLuint vao, int size, int type);

	static void setPointsVAO(GLuint& vao, GLuint& vbo,GLfloat* vertexData, float size);
	static void render(glm::mat4& mvp, float pointsize,GLuint shader_program, GLuint vao, float size, int type);
	static void activateTextures(GLuint shader_program, std::string* uniformName, GLuint* textureId, int textureCount);
	static void setUniformFloats(GLuint shader_program, std::string* uniformName, float* values, int count);
	static void setUniformMat(GLuint shader_program, std::string uniformName, glm::mat4& mat);
	//---------------------following method implement in ImguiOpeGL3App_glhelper.cpp

	//helper shape
	static void genCameraHelper(GLuint& vao, GLuint& vbo, 
		float width, float height, float ppx, float ppy, float fx, float fy, 
		glm::vec3 color, float size,bool isPlane);
	static void genOrigionAxis(GLuint& vao, GLuint& vbo);
	static void genBoundingboxWireframe(GLuint& vao, GLuint& vbo, glm::vec3 max, glm::vec3 min);
};

//custum rener and init demo
//class App :public ImguiOpeGL3App {
//
//public:
//	App() :ImguiOpeGL3App() {}
//
//	void initGL() override {
//		std::cout << "create shader, VAO" << std::endl;
//	}
//	void mainloop() override {
//		std::cout << "Render" << std::endl;
//	}
//};