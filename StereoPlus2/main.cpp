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
public:

	float LineThickness = 20;
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
		//glClearColor(0, 1, 0, 1.0f);
		// This is required before clearing Stencil buffer.
		// Don't know what ~0 is though.
		glStencilMask(~0);

		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glLineWidth(LineThickness);

		//glEnable(GL_STENCIL_TEST);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		
		//glStencilFunc(GL_ALWAYS, 1, 0xFF);
		//glStencilFunc(GL_ALWAYS, 1, 0xFF);
		//glStencilFunc(GL_NEVER, 1, 0xFF);
		//glPointSize(20);

		/*glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPointSize(2);*/

		glEnable(GL_STENCIL_TEST);
		//glClear(GL_STENCIL_BUFFER_BIT);

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0x1, 0xFF);
		//glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
		glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
		//glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

#pragma region left
		for (size_t i = 0; i < lineCount; i++)
		{
			Line left = config.camera.GetLeft(&lines[i]);

			Line lm;
			lm.Start = left.Start;
			lm.Start.z = lines[i].Start.z;
			lm.End = left.End;
			lm.End.z = lines[i].End.z;
			auto whitePartOfShaderLeft = FindWhitePartOfShader(lm.Start, lm.End, config.whiteZ, config.whiteZPrecision, config);
			DrawLine(
				left,
				[left, whitePartOfShaderLeft, this] { UpdateWhitePartOfShader(left.ShaderProgram, whitePartOfShaderLeft); }
			);
		}
#pragma endregion

//#pragma region right
//		for (size_t i = 0; i < lineCount; i++)
//		{
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
//#pragma endregion
//

		glStencilMask(0x00);

		glStencilFunc(GL_EQUAL, 0x1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		DrawSquare(whiteSquare);
		//glLineWidth(500);

		glDisable(GL_STENCIL_TEST);


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

		glBindVertexArray(square.VAOLeftTop); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		glDrawArrays(GL_TRIANGLES, 0, 3);



		// right bottom
		glBindBuffer(GL_ARRAY_BUFFER, square.VBORightBottom);

		glBufferData(GL_ARRAY_BUFFER, WhiteSquare::VerticesSize, square.rightBottom, GL_STREAM_DRAW);

		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(GL_POINTS);

		glBindBuffer(GL_ARRAY_BUFFER, 0);


		// Apply shader
		glUseProgram(square.ShaderProgramRightBottom);

		glBindVertexArray(square.VAORightBottom); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
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
		if (!whiteSquare.Init())
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
