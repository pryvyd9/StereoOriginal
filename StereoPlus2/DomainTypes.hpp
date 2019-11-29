#pragma once
#include "GLLoader.hpp"

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

struct Triangle
{
	glm::vec3 p1, p2, p3;

	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 3;

	GLuint VBO, VAO;
	GLuint ShaderProgram;
};

// Created for the sole purpose of crutching the broken 
// Line - triangle drawing mechanism.
// When you draw line/triangle and then change to other type then 
// First triangle and First line with the last frame shader's vertices get mixed.
// Dunno how it happens nor how to fix it. 
// Will return to it after some more immediate tasks are done.
struct ZeroLine
{
	Line line;

	bool Init()
	{
		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/zero.frag");

		line.ShaderProgram = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &line.VAO);
		glGenBuffers(1, &line.VBO);

		line.Start = line.End = glm::vec3(-1000);

		return true;
	}
};
struct ZeroTriangle
{
	Triangle triangle;

	bool Init()
	{
		for (size_t i = 0; i < 9; i++)
		{
			((float*)&triangle)[i] = -1000;
		}

		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/zero.frag");

		triangle.ShaderProgram = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &triangle.VAO);
		glGenBuffers(1, &triangle.VBO);

		return true;
	}
};


class WhiteSquare
{
public:

	float leftTop[9] = {
		-1, -1, 0,
		 1, -1, 0,
		-1,  1, 0,
	};

	float rightBottom[9] = {
		 1, -1, 0,
		 1,  1, 0,
		-1,  1, 0,
	};

	static const uint_fast8_t VerticesSize = sizeof(leftTop);

	GLuint VBOLeftTop, VAOLeftTop;
	GLuint VBORightBottom, VAORightBottom;
	GLuint ShaderProgramLeftTop, ShaderProgramRightBottom;


	bool Init()
	{

		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/WhiteSquare.frag");

		ShaderProgramLeftTop = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
		ShaderProgramRightBottom = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &VAOLeftTop);
		glGenBuffers(1, &VBOLeftTop);
		glGenVertexArrays(1, &VAORightBottom);
		glGenBuffers(1, &VBORightBottom);

		return true;
	}

	//glm::vec3 leftTop = glm::vec3(-1, -1, 0);
	//glm::vec3 leftBottom = glm::vec3(-1, 1, 0);
	//glm::vec3 rightTop = glm::vec3(-1, -1, 0);
	//glm::vec3 rightBottom = glm::vec3(-1, -1, 0);
};

class Cross
{
	std::string vertexShaderSource;
	std::string fragmentShaderSourceLeft;
	std::string fragmentShaderSourceRight;


	void CreateShaders(StereoLine& line, const char* vertexShaderSource, const char* fragmentShaderSourceLeft, const char* fragmentShaderSourceRight)
	{
		line.ShaderLeft = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSourceLeft);
		line.ShaderRight = GLLoader::CreateShaderProgram(vertexShaderSource, fragmentShaderSourceRight);
	}

	void CreateBuffers(StereoLine& line)
	{
		glGenVertexArrays(1, &line.VAOLeft);
		glGenBuffers(1, &line.VBOLeft);
		glGenVertexArrays(1, &line.VAORight);
		glGenBuffers(1, &line.VBORight);
	}

	bool CreateLines()
	{
		const char* vertexShaderSource = this->vertexShaderSource.c_str();
		const char* fragmentShaderSourceLeft = this->fragmentShaderSourceLeft.c_str();
		const char* fragmentShaderSourceRight = this->fragmentShaderSourceRight.c_str();

		{

			lines[0].Start = Position;
			lines[0].End = Position;
			lines[0].Start.x -= size;
			lines[0].End.x += size;

			CreateShaders(lines[0], vertexShaderSource, fragmentShaderSourceLeft, fragmentShaderSourceRight);
			CreateBuffers(lines[0]);
		}
		{

			lines[1].Start = Position;
			lines[1].End = Position;
			lines[1].Start.y -= size;
			lines[1].End.y += size;

			CreateShaders(lines[1], vertexShaderSource, fragmentShaderSourceLeft, fragmentShaderSourceRight);
			CreateBuffers(lines[1]);

		}
		{
			lines[2].Start = Position;
			lines[2].End = Position;
			lines[2].Start.z -= size;
			lines[2].End.z += size;

			CreateShaders(lines[2], vertexShaderSource, fragmentShaderSourceLeft, fragmentShaderSourceRight);
			CreateBuffers(lines[2]);

		}
		return true;
	}


	bool RefreshLines()
	{
		{
			lines[0].Start = Position;
			lines[0].End = Position;
			lines[0].Start.x -= size;
			lines[0].End.x += size;
		}
		{

			lines[1].Start = Position;
			lines[1].End = Position;
			lines[1].Start.y -= size;
			lines[1].End.y += size;
		}
		{
			lines[2].Start = Position;
			lines[2].End = Position;
			lines[2].Start.z -= size;
			lines[2].End.z += size;
		}
		return true;
	}

public:
	glm::vec3 Position = glm::vec3(0);

	StereoLine lines[3];

	float size = 0.8;
	bool isCreated = false;

	bool Refresh()
	{
		if (!isCreated)
		{
			isCreated = true;
			return CreateLines();
		}

		return RefreshLines();
	}


	bool Init()
	{
		vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		fragmentShaderSourceLeft = GLLoader::ReadShader("shaders/Left.frag");
		fragmentShaderSourceRight = GLLoader::ReadShader("shaders/Right.frag");

		return CreateLines();
	}
};



class StereoCamera
{
public:
	glm::vec2* viewSize = nullptr;
	glm::vec2 viewCenter = glm::vec2(0, 0);
	glm::vec3 transformVec = glm::vec3(0, 0, 0);

	glm::vec3 position = glm::vec3(0, 3, -10);
	glm::vec3 left = glm::vec3(1, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
	glm::vec3 forward = glm::vec3(0, 0, -1);


	float eyeToCenterDistance = 0.5;


	Line GetLeft(StereoLine* stereoLine)
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

	Line GetRight(StereoLine* stereoLine)
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
		viewCenter.x += value.x;
		viewCenter.y += value.y;

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
