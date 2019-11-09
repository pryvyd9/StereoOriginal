#ifndef DOMAIN_TYPES

#define DOMAIN_TYPES

#include "Infrastructure.h"


struct Line
{
	glm::vec3 Start, End;

	GLuint VBO, VAO;
	GLuint ShaderProgram;

	static const uint_fast8_t CoordinateCount = 6;
	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 2;

	float T;
};

struct StereoLine
{
	glm::vec3 Start, End;

	//Line Left, Right;

	static const uint_fast8_t CoordinateCount = 6;
	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 2;

	GLuint VBO, VAO;
	GLuint ShaderLeft, ShaderRight;

	float T;

};





#endif // !DOMAIN_TYPES