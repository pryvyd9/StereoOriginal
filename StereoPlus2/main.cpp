#include "GUI.hpp"
#include "CustomRenderWindow.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

struct Line
{
	glm::vec3 Start, End;

	//static const uint_fast8_t CoordinateCount = 6;
	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 2;

	GLuint VBO, VAO;
	GLuint ShaderProgram;
};

struct StereoLine
{
	glm::vec3 Start, End;

	//static const uint_fast8_t CoordinateCount = 6;
	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 2;

	//GLuint VBO, VAO;
	GLuint VBOLeft, VAOLeft;
	GLuint VBORight, VAORight;
	GLuint ShaderLeft, ShaderRight;
};

struct Config {
	std::string ShaderPath = "shaders/";
}Config;

class GLLoader
{
public:
	static std::string ReadShader(std::string path)
	{
		std::ifstream in(path);
		std::string contents((std::istreambuf_iterator<char>(in)),
			std::istreambuf_iterator<char>());

		return contents;
	}

	static GLuint CreateShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
	{
		int success;
		char infoLog[512];

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		// check for shader compile errors
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
		// link shaders
		GLuint shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		// check for linking errors
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return shaderProgram;
	}
};



class SceneObject
{
public:
	virtual bool Init() = 0;
};

class Cursor : SceneObject
{
public:
	glm::vec3 Position;
	glm::vec3 Scale;

	std::vector<StereoLine> lines;

	virtual bool Init()
	{
		float size = 0.8;
		float z = -0;
		std::string vertexShaderSource = GLLoader::ReadShader(Config.ShaderPath + ".vert");
		std::string fragmentShaderSourceLeft = GLLoader::ReadShader(Config.ShaderPath + "Left.frag");
		std::string fragmentShaderSourceRight = GLLoader::ReadShader(Config.ShaderPath + "Right.frag");

		StereoLine line;

		line;
		line.Start.z = -size;
		line.End.z = size;
		line.ShaderLeft = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSourceLeft.c_str());
		line.ShaderRight = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSourceRight.c_str());
		//glGenVertexArrays(1, &lines[2].VAO);
		//glGenBuffers(1, &lines[2].VBO);

		glGenVertexArrays(1, &line.VAOLeft);
		glGenBuffers(1, &line.VBOLeft);
		glGenVertexArrays(1, &line.VAORight);
		glGenBuffers(1, &line.VBORight);


		lines.push_back(line);

		return true;
	}

};


int main(int, char**)
{
	CustomRenderWindow customRenderWindow;
	
	GUI gui;
	gui.windows.push_back((Window*)&customRenderWindow);


	if (!gui.Init())
		return false;

	Cursor cursor;
	cursor.Init();

	if (!gui.MainLoop())
		return false;

	if (!gui.OnExit())
		return false;


    return 0;
}
