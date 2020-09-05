#pragma once
#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <functional>

using namespace std;

// GL4.1 required
class Renderer {
	glm::vec4 defaultColorLeft = glm::vec4(0, 1, 1, 1);
	glm::vec4 defaultColorRight = glm::vec4(1, 0, 0, 1);
	glm::vec4 dimmedColorLeft = glm::vec4(0, 1, 1, 0.5);
	glm::vec4 dimmedColorRight = glm::vec4(1, 0, 0, 0.5);


	GLuint ShaderLeft, ShaderRight;

	static void glfw_error_callback(int error, const char* description)
	{
		fprintf(stderr, "Glfw Error %d: %s\n", error, description);
	}

	bool InitGL()
	{
		// Setup window
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			return 1;

		// Decide GL+GLSL versions
#if __APPLE__
	// GL 3.2 + GLSL 150
		const char* glsl_version = "#version 150";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
		glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
		//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
		glWindow = glfwCreateWindow(1280, 720, "StereoOriginal", NULL, NULL);
		if (glWindow == NULL)
			return false;
		glfwMakeContextCurrent(glWindow);
		glfwSwapInterval(1); // Enable vsync

		// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
		bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
		bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
		bool err = gladLoadGL() == 0;
#else
		bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
		if (err)
		{
			fprintf(stderr, "Failed to initialize OpenGL loader!\n");
			return false;
		}

		return true;
	}

	void CreateShaders()
	{
		std::string vertexShaderSource1		   = GLLoader::ReadShader("shaders/.vert");
		std::string fragmentShaderSourceLeft1  = GLLoader::ReadShader("shaders/Left.frag");
		std::string fragmentShaderSourceRight1 = GLLoader::ReadShader("shaders/Right.frag");

		const char* vertexShaderSource = vertexShaderSource1.c_str();
		const char* fragmentShaderSourceLeft = fragmentShaderSourceLeft1.c_str();
		const char* fragmentShaderSourceRight = fragmentShaderSourceRight1.c_str();

		ShaderLeft = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSourceLeft);
		ShaderRight = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSourceRight);

		UpdateShaderColor(defaultColorLeft, defaultColorRight);
	}


	void UpdateShaderColor(glm::vec4 colorLeft, glm::vec4 colorRight) {
		// Available since GL4.1
		glProgramUniform4f(ShaderLeft, glGetUniformLocation(ShaderLeft, "myColor"), colorLeft.r, colorLeft.g, colorLeft.b, colorLeft.a);
		glProgramUniform4f(ShaderRight, glGetUniformLocation(ShaderRight, "myColor"), colorRight.r, colorRight.g, colorRight.b, colorRight.a);
	}

	void DrawSquare(WhiteSquare& square) {
		glBindVertexArray(square.VAOLeftTop);
		glBindBuffer(GL_ARRAY_BUFFER, square.VBOLeftTop);
		glBufferData(GL_ARRAY_BUFFER, WhiteSquare::VerticesSize, square.vertices, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(square.ShaderProgramLeftTop);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	void DrawObject(StereoCamera* camera, SceneObject* o) {
		if (!SceneObjectSelection::Selected().empty() && !exists(SceneObjectSelection::Selected(), o))
			UpdateShaderColor(dimmedColorLeft, dimmedColorRight);
		else
			UpdateShaderColor(defaultColorLeft, defaultColorRight);

		o->Draw(
			[&camera](const glm::vec3& p) { return camera->GetLeft(p); },
			[&camera](const glm::vec3& p) { return camera->GetRight(p); },
			ShaderLeft,
			ShaderRight);
	}

public:
	GLFWwindow* glWindow;
	const char* glsl_version;
	float LineThickness = 1;
	glm::vec4 backgroundColor = glm::vec4(0, 0, 0, 1);

	WhiteSquare whiteSquare;

	void Pipeline(const Scene& scene) {
		glDisable(GL_DEPTH_TEST);
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

		// This is required before clearing Stencil buffer.
		// Don't know why though.
		// ~ is bitwise negation 
		glStencilMask(~0);

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glLineWidth(LineThickness);

		glEnable(GL_STENCIL_TEST);

		// Anti aliasing
		{
			//glEnable(GL_LINE_SMOOTH);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//glEnable(GL_BLEND);
			//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		}

		for (auto o : scene.objects)
			DrawObject(scene.camera, o);

		DrawObject(scene.camera, scene.cross);
		
		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 0x1 | 0x2, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		DrawSquare(whiteSquare);

		// Anti aliasing
		//glDisable(GL_LINE_SMOOTH | GL_BLEND);

		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
	}

	bool Init() {
		if (!InitGL()
			|| !whiteSquare.Init())
			return false;

		CreateShaders();

		return true;
	}
};
