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
		line.Start.x = 0;
		line.Start.y = 0;
		line.Start.z = -size;
		line.End.x = 0;
		line.End.y = 0;
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
	glm::vec2* viewSize = nullptr;
	glm::vec2 screenCenter = glm::vec2(0, 0);
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
