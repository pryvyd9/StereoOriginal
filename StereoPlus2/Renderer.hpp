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
	enum class Shader {
		BrightLeft,
		BrightRight,
		DimLeft,
		DimRight,
	};

	std::map<Shader, GLuint> shaders;

	GLuint VAO;

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
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);
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
		std::string vertexShaderSource1	= GLLoader::ReadShader("shaders/.vert");
		std::string fragmentShaderSource1 = GLLoader::ReadShader("shaders/.frag");

		const char* vertexShaderSource = vertexShaderSource1.c_str();
		const char* fragmentShaderSource = fragmentShaderSource1.c_str();
		
		// We need separate shaders to be able to set different colors.
		// It's possible to change colors while switching between left and right but it's slow.
		shaders[Shader::BrightLeft] = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
		shaders[Shader::BrightRight] = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
		shaders[Shader::DimLeft] = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
		shaders[Shader::DimRight] = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

		UpdateShaderColor(shaders[Shader::BrightLeft], Settings::ColorLeft().Get(), "myColor");
		UpdateShaderColor(shaders[Shader::BrightRight], Settings::ColorRight().Get(), "myColor");
		UpdateShaderColor(shaders[Shader::DimLeft], Settings::DimmedColorLeft().Get(), "myColor");
		UpdateShaderColor(shaders[Shader::DimRight], Settings::DimmedColorRight().Get(), "myColor");
	}

	void UpdateShaderColor(GLuint shader, glm::vec4 color, const char* name) {
		// Available since GL4.1
		glProgramUniform4f(shader, glGetUniformLocation(shader, name), color.r, color.g, color.b, color.a);
	}

	//void DrawSquare(const WhiteSquare& square) {
	//	glBindVertexArray(square.VAOLeftTop);
	//	glBindBuffer(GL_ARRAY_BUFFER, square.VBOLeftTop);
	//	glBufferData(GL_ARRAY_BUFFER, WhiteSquare::VerticesSize, square.vertices, GL_STREAM_DRAW);

	//	glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	//	glEnableVertexAttribArray(GL_POINTS);
	//	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//	// Apply shader
	//	glUseProgram(square.ShaderProgram);
	//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//}

	void DrawWithShader(Camera* camera, std::vector<PON>& os, GLuint shader, std::function<void(SceneObject*, GLuint)> drawFunc) {
		glUseProgram(shader);
		for (auto& o : os) {
			o->UdateBuffer(
				[&camera](const glm::vec3& p) { return camera->GetLeft(p); },
				[&camera](const glm::vec3& p) { return camera->GetRight(p); });

			drawFunc(o.Get(), shader);
		}
	}
	void DrawWithShader(Camera* camera, SceneObject* o, GLuint shader, std::function<void(SceneObject*, GLuint)> drawFunc) {
		glUseProgram(shader);

		o->UdateBuffer(
			[&camera](const glm::vec3& p) { return camera->GetLeft(p); },
			[&camera](const glm::vec3& p) { return camera->GetRight(p); });

		drawFunc(o, shader);
	}

public:
	GLFWwindow* glWindow;
	const char* glsl_version;
	float LineThickness = 1;
	glm::vec4 backgroundColor = glm::vec4(0, 0, 0, 0);

	void Pipeline(Scene& scene) {
		glDisable(GL_DEPTH_TEST);
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glLineWidth(LineThickness);
		

		// Anti aliasing
		{
			//glEnable(GL_LINE_SMOOTH);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//glEnable(GL_BLEND);
			//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		}
		
		glEnable(GL_BLEND);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_ONE, GL_ONE);

		if (ObjectSelection::Selected().empty()) {

			DrawWithShader(scene.camera, scene.Objects().Get(), shaders[Shader::BrightLeft], [](SceneObject* o, GLuint shader) { o->DrawLeft(shader); });
			DrawWithShader(scene.camera, scene.Objects().Get(), shaders[Shader::BrightRight], [](SceneObject* o, GLuint shader) { o->DrawRight(shader); });
			
			DrawWithShader(scene.camera, &scene.cross().Get(), shaders[Shader::BrightLeft], [](SceneObject* o, GLuint shader) { o->DrawLeft(shader); });
			DrawWithShader(scene.camera, &scene.cross().Get(), shaders[Shader::BrightRight], [](SceneObject* o, GLuint shader) { o->DrawRight(shader); });
		}
		else {
			std::set<PON> objectsSorted;
			for (auto& o : scene.Objects().Get())
				objectsSorted.emplace(o);

			std::vector<PON> dimObjects;
			std::set_difference(
				objectsSorted.begin(),
				objectsSorted.end(),
				ObjectSelection::Selected().begin(),
				ObjectSelection::Selected().end(),
				std::inserter(dimObjects, dimObjects.begin()));

			std::vector<PON> selectedExistent;
			for (auto& o : ObjectSelection::Selected())
				if (o.HasValue())
					selectedExistent.push_back(o);

			DrawWithShader(scene.camera, dimObjects, shaders[Shader::DimLeft], [](SceneObject* o, GLuint shader) { o->DrawLeft(shader); });
			DrawWithShader(scene.camera, dimObjects, shaders[Shader::DimRight], [](SceneObject* o, GLuint shader) { o->DrawRight(shader); });
			
			DrawWithShader(scene.camera, selectedExistent, shaders[Shader::BrightLeft], [](SceneObject* o, GLuint shader) { o->DrawLeft(shader); });
			DrawWithShader(scene.camera, selectedExistent, shaders[Shader::BrightRight], [](SceneObject* o, GLuint shader) { o->DrawRight(shader); });

			DrawWithShader(scene.camera, &scene.cross().Get(), shaders[Shader::BrightLeft], [](SceneObject* o, GLuint shader) { o->DrawLeft(shader); });
			DrawWithShader(scene.camera, &scene.cross().Get(), shaders[Shader::BrightRight], [](SceneObject* o, GLuint shader) { o->DrawRight(shader); });
		}

		// Anti aliasing
		//glDisable(GL_LINE_SMOOTH | GL_BLEND);

		glDisable(GL_BLEND);

		glEnable(GL_DEPTH_TEST);
	}

	bool Init() {
		if (!InitGL())
			return false;

		CreateShaders();

		// Not sure what exactly Vertex Array Object is.
		// From what I understand it's related to index order in objects.
		// The main thing is if we draw all objects the same way 
		// 1 VAO should be enough for the whole scene.
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		Settings::ColorLeft().OnChanged() += [&](glm::vec4 color) { UpdateShaderColor(shaders[Shader::BrightLeft], color, "myColor"); };
		Settings::ColorRight().OnChanged() += [&](glm::vec4 color) { UpdateShaderColor(shaders[Shader::BrightRight], color, "myColor"); };
		Settings::DimmedColorLeft().OnChanged() += [&](glm::vec4 color) { UpdateShaderColor(shaders[Shader::DimLeft], color, "myColor"); };
		Settings::DimmedColorRight().OnChanged() += [&](glm::vec4 color) { UpdateShaderColor(shaders[Shader::DimRight], color, "myColor"); };

		return true;
	}

	bool OnExit() {
		glDeleteVertexArrays(1, &VAO);
		return true;
	}
};
