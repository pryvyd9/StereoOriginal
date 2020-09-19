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
	const int stencilBufferMaskBright1 = 0x1;
	const int stencilBufferMaskBright2 = 0x2;
	const int stencilBufferMaskDim1 = 0x4;
	const int stencilBufferMaskDim2 = 0x8;

	glm::vec4 defaultColorLeft = glm::vec4(0, 1, 1, 1);
	glm::vec4 defaultColorRight = glm::vec4(1, 0, 0, 1);
	glm::vec4 dimmedColorLeft = glm::vec4(0, 1, 1, 0.5);
	glm::vec4 dimmedColorRight = glm::vec4(1, 0, 0, 0.5);

	glm::vec4 whiteColorBright = glm::vec4(1, 1, 1, 1);
	glm::vec4 whiteColorDim = glm::vec4(1, 1, 1, 0.5);


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

		UpdateShaderColor(whiteSquare.ShaderProgram, whiteColorBright, "myColor");
		UpdateShaderColor(whiteSquareDim.ShaderProgram, whiteColorDim, "myColor");
	}
	void UpdateShaderColor(glm::vec4 colorLeft, glm::vec4 colorRight) {
		// Available since GL4.1
		UpdateShaderColor(ShaderLeft, colorLeft, "myColor");
		UpdateShaderColor(ShaderRight, colorRight, "myColor");
	}
	void UpdateShaderColor(GLuint shader, glm::vec4 color, const char* name) {
		// Available since GL4.1
		glProgramUniform4f(shader, glGetUniformLocation(shader, name), color.r, color.g, color.b, color.a);
	}

	void DrawSquare(const WhiteSquare& square) {
		glBindVertexArray(square.VAOLeftTop);
		glBindBuffer(GL_ARRAY_BUFFER, square.VBOLeftTop);
		glBufferData(GL_ARRAY_BUFFER, WhiteSquare::VerticesSize, square.vertices, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(square.ShaderProgram);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	void DrawBright(StereoCamera* camera, SceneObject* o) {
		UpdateShaderColor(defaultColorLeft, defaultColorRight);
		o->Draw(
			[&camera](const glm::vec3& p) { return camera->GetLeft(p); },
			[&camera](const glm::vec3& p) { return camera->GetRight(p); },
			ShaderLeft,
			ShaderRight,
			stencilBufferMaskBright1,
			stencilBufferMaskBright2);
	}
	void DrawDim(StereoCamera* camera, SceneObject* o) {
		UpdateShaderColor(dimmedColorLeft, dimmedColorRight);
		o->Draw(
			[&camera](const glm::vec3& p) { return camera->GetLeft(p); },
			[&camera](const glm::vec3& p) { return camera->GetRight(p); },
			ShaderLeft,
			ShaderRight,
			stencilBufferMaskDim1,
			stencilBufferMaskDim2);
	}

	void DrawIntersection(const WhiteSquare& square, GLuint stencilMask) {
		glStencilMask(0x00);

		glStencilFunc(GL_EQUAL, stencilMask, stencilMask);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		DrawSquare(square);
	}

public:
	GLFWwindow* glWindow;
	const char* glsl_version;
	float LineThickness = 1;
	glm::vec4 backgroundColor = glm::vec4(0, 0, 0, 1);

	WhiteSquare whiteSquare;
	WhiteSquare whiteSquareDim;
	//struct less {
	//	constexpr bool operator()(const PON& _Left, const PON& _Right) const {
	//		return _Left < _Right;
	//	}
	//};
	void Pipeline(Scene& scene) {
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
		
		if (SceneObjectSelection::Selected().empty()) {
			for (auto o : scene.objects)
				DrawBright(scene.camera, o.Get());
			DrawBright(scene.camera, scene.cross);
			DrawIntersection(whiteSquare, stencilBufferMaskBright1 | stencilBufferMaskBright2);
		}
		else {
			std::vector<SceneObject*> sceneObjects(scene.objects.size());
			for (size_t i = 0; i < scene.objects.size(); i++)
				//sceneObjects[i] = const_cast<Scene*>(&scene)->objects[i].Get();
				sceneObjects[i] = scene.objects[i].Get();

			std::vector<SceneObject*> dimObjects;

			std::set_difference(
				sceneObjects.begin(),
				sceneObjects.end(),
				SceneObjectSelection::Selected().begin(),
				SceneObjectSelection::Selected().end(),
				std::inserter(dimObjects, dimObjects.begin()));

			for (auto o : dimObjects)
				DrawDim(scene.camera, o);
			DrawIntersection(whiteSquareDim, stencilBufferMaskDim1 | stencilBufferMaskDim2);

			for (auto o : SceneObjectSelection::Selected())
				DrawBright(scene.camera, o);
			DrawBright(scene.camera, scene.cross);
			DrawIntersection(whiteSquare, stencilBufferMaskBright1 | stencilBufferMaskBright2);
		}

		// Anti aliasing
		//glDisable(GL_LINE_SMOOTH | GL_BLEND);

		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
	}

	bool Init() {
		if (!InitGL()
			|| !whiteSquare.Init()
			|| !whiteSquareDim.Init())
			return false;

		CreateShaders();

		return true;
	}
};
