#include "GUI.hpp"
#include "CustomRenderWindow.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <functional>

using namespace std;

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


class Cursor 
{
public:
	glm::vec3 Position = glm::vec3(0);

	std::vector<StereoLine> lines;

	bool Init()
	{
		float size = 0.8;
		float z = -0;
		std::string vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		std::string fragmentShaderSourceLeft = GLLoader::ReadShader("shaders/Left.frag");
		std::string fragmentShaderSourceRight = GLLoader::ReadShader("shaders/Right.frag");

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



class StereoCamera
{
public:
	glm::vec2 screenSize = glm::vec2(1, 1);
	glm::vec2 screenCenter = glm::vec2(0, 0);
	glm::vec3 transformVec = glm::vec3(0, 0, 0);

	glm::vec3 position = glm::vec3(0, 3, -10);
	glm::vec3 left = glm::vec3(1, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
	glm::vec3 forward = glm::vec3(0, 0, -1);


	float eyeToCenterDistance = 0.5;


	Line GetLeft(StereoLine * stereoLine)
	{
		Line line;

		glm::vec3 S = stereoLine->Start;

		float denominator = position.z - S.z - transformVec.z;

		float SleftX = (S.x * position.z - (S.z + transformVec.z) * (position.x - eyeToCenterDistance) + position.z * transformVec.x) / denominator;
		float Sy = (position.z * (transformVec.y - S.y) + position.y * (S.z + transformVec.z)) / denominator;

		line.Start.x = SleftX;
		line.Start.y = Sy;
		line.Start.z = 0;

		glm::vec3 E = stereoLine->End;

		denominator = position.z - E.z - transformVec.z;

		float EleftX = (E.x * position.z - (E.z + transformVec.z) * (position.x - eyeToCenterDistance) + position.z * transformVec.x) / denominator;
		float Ey = (position.z * (transformVec.y - E.y) + position.y * (E.z + transformVec.z)) / denominator;

		line.End.x = EleftX;
		line.End.y = Ey;
		line.End.z = 0;

		line.VAO = stereoLine->VAOLeft;
		line.VBO = stereoLine->VBOLeft;
		line.ShaderProgram = stereoLine->ShaderLeft;

		return line;
	}

	Line GetRight(StereoLine * stereoLine)
	{
		Line line;

		glm::vec3 S = stereoLine->Start;

		float denominator = position.z - S.z - transformVec.z;

		float SrightX = (S.x * position.z - (S.z + transformVec.z) * (position.x + eyeToCenterDistance) + position.z * transformVec.x) / denominator;
		float Sy = (position.z * (transformVec.y - S.y) + position.y * (S.z + transformVec.z)) / denominator;

		line.Start.x = SrightX;
		line.Start.y = Sy;
		line.Start.z = 0;

		glm::vec3 E = stereoLine->End;

		denominator = position.z - E.z - transformVec.z;

		float ErightX = (E.x * position.z - (E.z + transformVec.z) * (position.x + eyeToCenterDistance) + position.z * transformVec.x) / denominator;
		float Ey = (position.z * (transformVec.y - E.y) + position.y * (E.z + transformVec.z)) / denominator;

		line.End.x = ErightX;
		line.End.y = Ey;
		line.End.z = 0;

		line.VAO = stereoLine->VAORight;
		line.VBO = stereoLine->VBORight;
		line.ShaderProgram = stereoLine->ShaderRight;

		return line;
	}

#pragma region Move

	void Move(glm::vec3 value)
	{
		//position += value;
		transformVec += value;
	}

	void MoveLeft(float value) {
		Move(left * value);
	}

	void MoveRight(float value) {
		Move(-left * value);
	}

	void MoveUp(float value) {
		Move(up * value);
	}

	void MoveDown(float value) {
		Move(-up * value);
	}

	void MoveForward(float value) {
		Move(forward * value);
	}

	void MoveBack(float value) {
		Move(-forward * value);
	}

#pragma endregion
};

struct WhitePartOfShader {
	bool isZero;
	bool isX;
	float min, max;
};

struct SceneConfiguration {
	float whiteZ = 0;
	float whiteZPrecision = 0.1;

	StereoCamera camera;
	GLFWwindow* window;
};

//// Render pipeline:
//// Compute white x or y limits for each line
//// Project lines to camera z=0 plane
//// Apply white limits to shaders of lines
//class RenderScenePipeline {
//public:
//	// Determines which parts of line is white.
//	// Finds left and right limits of white in x
//	// or top and bottom limits of white in y.
//	// Z here is relative to camera
//	// >0 is close to camera and <0 is far from camera
//	WhitePartOfShader FindWhitePartOfShader(glm::vec3 start, glm::vec3 end, float whiteZ, float precision, SceneConfiguration& config) {
//		WhitePartOfShader res;
//
//		/*start += config.camera.transformVec;
//		end += config.camera.transformVec;*/
//
//		bool isZero = (start.z == end.z) && (whiteZ + precision > start.z && start.z > whiteZ - precision);
//		if (!isZero)
//		{
//			float xSize = end.x > start.x ? end.x - start.x : start.x - end.x;
//			float ySize = end.y > start.y ? end.y - start.y : start.y - end.y;
//
//			// (z - z1) / (z2 - z1) = t
//			float t = (whiteZ - start.z) / (end.z - start.z);
//			float center;
//
//			if (xSize > ySize)
//			{
//				center = (end.x - start.x) * t + start.x;
//				res.isX = true;
//			}
//			else
//			{
//				center = (end.y - start.y) * t + start.y;
//				res.isX = false;
//			}
//
//			res.min = center - precision;
//			res.max = center + precision;
//		}
//
//		res.isZero = isZero;
//
//		return res;
//	}
//
//	void Pipeline(StereoLine * lines, size_t lineCount, SceneConfiguration & config)
//	{
//		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
//		glClear(GL_COLOR_BUFFER_BIT);
//		glPointSize(2);
//
//		for (size_t i = 0; i < lineCount; i++)
//		{
//
//			Line left = config.camera.GetLeft(&lines[i]);
//
//			Line lm;
//			lm.Start = left.Start;
//			lm.Start.z = lines[i].Start.z;
//			lm.End = left.End;
//			lm.End.z = lines[i].End.z;
//			auto whitePartOfShaderLeft = FindWhitePartOfShader(lm.Start, lm.End, config.whiteZ, config.whiteZPrecision, config);
//			DrawLine(
//				left,
//				[left, whitePartOfShaderLeft, this] { UpdateWhitePartOfShader(left.ShaderProgram, whitePartOfShaderLeft); }
//			);
//
//			Line right = config.camera.GetRight(&lines[i]);
//
//			Line rm;
//			rm.Start = right.Start;
//			rm.Start.z = lines[i].Start.z;
//			rm.End = right.End;
//			rm.End.z = lines[i].End.z;
//			auto whitePartOfShaderRight = FindWhitePartOfShader(rm.Start, rm.End, config.whiteZ, config.whiteZPrecision, config);
//			DrawLine(
//				right,
//				[right, whitePartOfShaderRight, this] { UpdateWhitePartOfShader(right.ShaderProgram, whitePartOfShaderRight); }
//			);
//		}
//	}
//
//
//	void DrawLine(Line line, function<void()> updateWhitePartOfShader)
//	{
//		glBindBuffer(GL_ARRAY_BUFFER, line.VBO);
//
//		glBufferData(GL_ARRAY_BUFFER, Line::VerticesSize, &line, GL_STREAM_DRAW);
//
//		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//		glEnableVertexAttribArray(GL_POINTS);
//
//		glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//		updateWhitePartOfShader();
//
//		// Apply shader
//		glUseProgram(line.ShaderProgram);
//
//		glBindVertexArray(line.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
//		glDrawArrays(GL_LINES, 0, 2);
//	}
//
//	void UpdateWhitePartOfShader(GLuint shader, WhitePartOfShader whitePartOfShader)
//	{
//		// i for integer. I guess bool suits int more but it works in any case.
//		// f for float
//		glUniform1i(glGetUniformLocation(shader, "isZero"), whitePartOfShader.isZero);
//		glUniform1i(glGetUniformLocation(shader, "isX"), whitePartOfShader.isX);
//		glUniform1f(glGetUniformLocation(shader, "min"), whitePartOfShader.min);
//		glUniform1f(glGetUniformLocation(shader, "max"), whitePartOfShader.max);
//	}
//};




int main(int, char**)
{
	//RenderScenePipeline renderPipeline;

	CustomRenderWindow customRenderWindow;
	
	GUI gui;
	gui.windows.push_back((Window*)&customRenderWindow);


	if (!gui.Init())
		return false;

	SceneConfiguration config;
	config.whiteZ = 0;
	config.whiteZPrecision = 0.1;
	config.window = gui.window;

	Cursor cursor;
	cursor.Init();

	//customRenderWindow.customRenderFunc = [lines, mainWindow, &renderPipeline, &config, lineCount] {

	//	config.camera.screenSize = mainWindow.renderSize;

	//	renderPipeline.Pipeline(lines, lineCount, config);

	//	//config.camera.MoveRight(0.001);

	//	//Draw(lines, mainWindow.window);
	//	//MoveCamera();
	//	//MoveLines(lines);
	//	//MoveCross(lines);
	//	return true;
	//};

	if (!gui.MainLoop())
		return false;

	if (!gui.OnExit())
		return false;


    return true;
}
