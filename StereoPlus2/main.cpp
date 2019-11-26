#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "SceneConfig.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <functional>

using namespace std;


// Render pipeline:
// Compute white x or y limits for each line
// Project lines to camera z=0 plane
// Apply white limits to shaders of lines
class RenderScenePipeline {
	struct WhitePartOfShader {
		bool isZero;
		bool isX;
		float min, max;
	};

	ZeroLine zeroLine;
	ZeroTriangle zeroTriangle;
public:

	float LineThickness = 1;
	glm::vec4 backgroundColor = glm::vec4(0,0,0,1);

	WhiteSquare whiteSquare;


	// Determines which parts of line is white.
	// Finds left and right limits of white in x
	// or top and bottom limits of white in y.
	// Z here is relative to camera
	// >0 is close to camera and <0 is far from camera
	WhitePartOfShader FindWhitePartOfShader(glm::vec3 start, glm::vec3 end, float whiteZ, float precision, SceneConfiguration& config) {
		WhitePartOfShader res;

		/*start += config.camera.transformVec;
		end += config.camera.transformVec;*/

		bool isZero = (start.z == end.z) && (whiteZ + precision > start.z && start.z > whiteZ - precision);
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

	void Pipeline(StereoLine * lines, size_t lineCount, SceneConfiguration & config)
	{
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

		// This is required before clearing Stencil buffer.
		// Don't know why though.
		// ~ is bitwise negation 
		glStencilMask(~0);

		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glLineWidth(LineThickness);

		glEnable(GL_STENCIL_TEST);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0x1, 0xFF);
		glStencilOp(GL_KEEP, GL_INCR, GL_INCR);

		Line right = config.camera.GetRight(&lines[0]);
		DrawLine(right);

		for (size_t i = 0; i < lineCount; i++)
		{
			Line left = config.camera.GetLeft(&lines[i]);
			Line right = config.camera.GetRight(&lines[i]);

			// Lines have to be rendered left - right - left - right...
			// This is due to the bug of messing shaders.
			DrawLine(left);
			DrawLine(right);
		}

		
		// Crutch to overcome bug with messing fragment shaders and vertices up.
		// Presumably fragment and vertex are messed up.
		{
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glStencilMask(0x00);
			DrawLine(zeroLine.line);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		glStencilMask(0x00);
		glStencilFunc(GL_LESS, 0x1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


		// Crutch to overcome bug with messing fragment shaders and vertices up.
		// Presumably fragment and vertex are messed up.
		{
			DrawSquare(whiteSquare);
		}

		DrawSquare(whiteSquare);

		glDisable(GL_STENCIL_TEST);


	}

	//void DrawZeroLine()
	//{
	//
	//	glBindBuffer(GL_ARRAY_BUFFER, zeroLine.line.VBO);
	//	glBufferData(GL_ARRAY_BUFFER, Line::VerticesSize, &line, GL_STREAM_DRAW);

	//	glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	//	glEnableVertexAttribArray(GL_POINTS);
	//	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//	glBindBuffer(GL_ARRAY_BUFFER, 0);


	//	// Apply shader
	//	glUseProgram(line.ShaderProgram);

	//	glBindVertexArray(line.VAO);
	//	glDrawArrays(GL_LINES, 0, 2);
	//}

	void DrawLine(Line &line)
	{
		glBindBuffer(GL_ARRAY_BUFFER, line.VBO);
		glBufferData(GL_ARRAY_BUFFER, Line::VerticesSize, &line, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// Apply shader
		glUseProgram(line.ShaderProgram);

		glBindVertexArray(line.VAO); 
		glDrawArrays(GL_LINES, 0, 2);

		//glBindVertexArray(0);

	}

	void DrawLine(Line line, function<void()> updateWhitePartOfShader)
	{
		glBindBuffer(GL_ARRAY_BUFFER, line.VBO);

		glBufferData(GL_ARRAY_BUFFER, Line::VerticesSize, &line, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		updateWhitePartOfShader();

		// Apply shader
		glUseProgram(line.ShaderProgram);

		glBindVertexArray(line.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		glDrawArrays(GL_LINES, 0, 2);
	}

	void DrawTriangle(Triangle& triangle)
	{
		// left top
		glBindBuffer(GL_ARRAY_BUFFER, triangle.VBO);
		glBufferData(GL_ARRAY_BUFFER, Triangle::VerticesSize, &triangle, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(triangle.ShaderProgram);
		glBindVertexArray(triangle.VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	void DrawSquare(WhiteSquare &square)
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


	void UpdateWhitePartOfShader(GLuint shader, WhitePartOfShader whitePartOfShader)
	{
		// i for integer. I guess bool suits int more but it works in any case.
		// f for float
		glUniform1i(glGetUniformLocation(shader, "isZero"), whitePartOfShader.isZero);
		glUniform1i(glGetUniformLocation(shader, "isX"), whitePartOfShader.isX);
		glUniform1f(glGetUniformLocation(shader, "min"), whitePartOfShader.min);
		glUniform1f(glGetUniformLocation(shader, "max"), whitePartOfShader.max);
	}


	bool Init()
	{
		if (!whiteSquare.Init()
			|| !zeroLine.Init()
			|| !zeroTriangle.Init())
			return false;

		return true;
	}
};




int main(int, char**)
{
	RenderScenePipeline renderPipeline;

	GUI gui;

	CustomRenderWindow customRenderWindow;
	CameraPropertiesWindow cameraPropertiesWindow;
	CrossPropertiesWindow crossPropertiesWindow;

	if (false
		|| !gui.Init() 
		|| !customRenderWindow.Init()
		|| !cameraPropertiesWindow.Init()
		|| !crossPropertiesWindow.Init())
		return false;


	SceneConfiguration config;
	config.whiteZ = 0;
	config.whiteZPrecision = 0.1;
	config.window = gui.window;
	config.camera.viewSize = &customRenderWindow.renderSize;

	cameraPropertiesWindow.Camera = &config.camera;
	gui.sceneConfig = &config;

	Cross cross;

	if (!cross.Init())
		return false;
	!renderPipeline.Init();
	crossPropertiesWindow.Cross = &cross;


	gui.windows.push_back((Window*)& customRenderWindow);
	gui.windows.push_back((Window*)& cameraPropertiesWindow);
	gui.windows.push_back((Window*)& crossPropertiesWindow);

	customRenderWindow.customRenderFunc = [&cross, &config, &gui, &renderPipeline] {
		renderPipeline.Pipeline(&cross.lines[0], cross.lines.size(), config);

		return true;
	};

	if (!gui.MainLoop()
		|| !gui.OnExit())
		return false;

    return true;
}
