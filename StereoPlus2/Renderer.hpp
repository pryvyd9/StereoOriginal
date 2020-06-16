#pragma once
#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
//#include <glm/gtc/quaternion.hpp>
#include <functional>

using namespace std;


// Render pipeline:
// Compute white x or y limits for each line
// Project lines to camera z=0 plane
// Apply white limits to shaders of lines
class Renderer {
	struct WhitePartOfShader {
		bool isZero;
		bool isX;
		float min, max;
	};

	GLuint VAOLeft, VAORight, VBOLeft, VBORight, ShaderLeft, ShaderRight;


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
		std::string vertexShaderSource1		  = GLLoader::ReadShader("shaders/.vert");
		std::string fragmentShaderSourceLeft1 = GLLoader::ReadShader("shaders/Left.frag");
		std::string fragmentShaderSourceRight1 = GLLoader::ReadShader("shaders/Right.frag");

		const char* vertexShaderSource = vertexShaderSource1.c_str();
		const char* fragmentShaderSourceLeft = fragmentShaderSourceLeft1.c_str();
		const char* fragmentShaderSourceRight = fragmentShaderSourceRight1.c_str();

		ShaderLeft = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSourceLeft);
		ShaderRight = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSourceRight);
	}
	void CreateBuffers()
	{
		glGenVertexArrays(1, &VAOLeft);
		glGenBuffers(1, &VBOLeft);
		glGenVertexArrays(1, &VAORight);
		glGenBuffers(1, &VBORight);
	}

	void DrawLineLeft(const Pair& line)
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Pair::p1) * 2, &line, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// Apply shader
		glUseProgram(ShaderLeft);

		glBindVertexArray(VAOLeft);
		glDrawArrays(GL_LINES, 0, 2);
	}
	void DrawLineRight(const Pair& line)
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Pair::p1) * 2, &line, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// Apply shader
		glUseProgram(ShaderRight);

		glBindVertexArray(VAORight);
		glDrawArrays(GL_LINES, 0, 2);
	}


	void DrawSquare(WhiteSquare& square)
	{
		// left top
		glBindBuffer(GL_ARRAY_BUFFER, square.VBOLeftTop);
		glBufferData(GL_ARRAY_BUFFER, WhiteSquare::VerticesSize, square.leftTop, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(square.ShaderProgramLeftTop);
		glBindVertexArray(square.VAOLeftTop);
		glDrawArrays(GL_TRIANGLES, 0, 3);



		// right bottom
		glBindBuffer(GL_ARRAY_BUFFER, square.VBORightBottom);
		glBufferData(GL_ARRAY_BUFFER, WhiteSquare::VerticesSize, square.rightBottom, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(square.ShaderProgramRightBottom);
		glBindVertexArray(square.VAORightBottom);
		glDrawArrays(GL_TRIANGLES, 0, 3);

	}

	void DrawObject(StereoCamera* camera, SceneObject* o) {
		for (auto l : o->GetLines()) {
			// Lines have to be rendered left - right - left - right...
			// This is due to the bug of messing shaders.
			glStencilMask(0x1);
			glStencilFunc(GL_ALWAYS, 0x1, 0xFF);
			glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
			DrawLineLeft(camera->GetLeft(l));

			glStencilMask(0x2);
			glStencilFunc(GL_ALWAYS, 0x2, 0xFF);
			glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
			DrawLineRight(camera->GetRight(l));
		}
	}

	// Determines which parts of line is white.
	// Finds left and right limits of white in x
	// or top and bottom limits of white in y.
	// Z here is relative to camera
	// >0 is close to camera and <0 is far from camera
	WhitePartOfShader FindWhitePartOfShader(glm::vec3 start, glm::vec3 end, float whiteZ, float precision, Scene& config) {
		WhitePartOfShader res;

		/*start += config.camera.transformVec;
		end += config.camera.transformVec;*/

		bool isZero = (start.z == end.z) && (whiteZ + precision > start.z&& start.z > whiteZ - precision);
		if (!isZero)
		{
			float xSize = end.x > start.x ? end.x - start.x : start.x - end.x;
			float ySize = end.y > start.y ? end.y - start.y : start.y - end.y;

			// (z - z1) / (z2 - z1) = t
			float t = (whiteZ - start.z) / (end.z - start.z);
			float center;

			if (xSize > ySize)
			{
				center = (end.x - start.x) * t + start.x;
				res.isX = true;
			}
			else
			{
				center = (end.y - start.y) * t + start.y;
				res.isX = false;
			}

			res.min = center - precision;
			res.max = center + precision;
		}

		res.isZero = isZero;

		return res;
	}

	void UpdateWhitePartOfShader(GLuint shader, WhitePartOfShader whitePartOfShader)
	{
		// i for integer. I guess bool suits int more but it works in any case.
		// f for float
		glUniform1i(glGetUniformLocation(shader, "isZero"), whitePartOfShader.isZero);
		glUniform1i(glGetUniformLocation(shader, "isX"), whitePartOfShader.isX);
		glUniform1f(glGetUniformLocation(shader, "min"), whitePartOfShader.min);
		glUniform1f(glGetUniformLocation(shader, "max"), whitePartOfShader.max);
	}


public:
	GLFWwindow* glWindow;


	const char* glsl_version;
	float LineThickness = 1;
	glm::vec4 backgroundColor = glm::vec4(0, 0, 0, 1);

	WhiteSquare whiteSquare;

	void Pipeline(const Scene& scene)
	{
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


		// Crutch
		{
			glStencilMask(0x2);
			glStencilFunc(GL_ALWAYS, 0x2, 0xFF);
			glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
			DrawLineRight(Pair());
		}

		for (auto o : scene.objects)
			DrawObject(scene.camera, o);

		DrawObject(scene.camera, scene.cross);

		// Crutch
		{
			glStencilMask(0x1);
			glStencilFunc(GL_ALWAYS, 0x1, 0xFF);
			glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
			DrawLineLeft(Pair());
		}

		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 0x1 | 0x2, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		// Crutch to overcome bug with messing fragment shaders and vertices up.
		// Presumably fragment and vertex are messed up.
		{
			DrawSquare(whiteSquare);
		}

		DrawSquare(whiteSquare);

		// Anti aliasing
		//glDisable(GL_LINE_SMOOTH | GL_BLEND);

		glDisable(GL_STENCIL_TEST);
		glEnable(GL_DEPTH_TEST);
	}


	bool Init()
	{
		if (!InitGL()
			|| !whiteSquare.Init())
			return false;

		CreateShaders();
		CreateBuffers();

		return true;
	}
};
