#pragma once
#include "GLLoader.hpp"
#include "Window.hpp"
#include <functional>
#include "DomainTypes.hpp"


class CustomRenderWindow : Window
{
	GLuint fbo;
	GLuint texture;
	GLuint depthBuffer;
	GLuint depthBufferTexture;

	GLuint createFrameBuffer() {
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		int g = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		//	int u = 0;
		//}

		return fbo;
	}

	GLuint createTextureAttachment(int width, int height) {
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

		return texture;
	}

	//GLuint createDepthTextureAttachment(int width, int height) {
	//	GLuint texture;
	//	glGenTextures(1, &texture);
	//	glBindTexture(GL_TEXTURE_2D, texture);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);

	//	return texture;
	//}

	//GLuint createDepthBufferAttachment(int width, int height) {
	//	GLuint depthBuffer;
	//	glGenRenderbuffers(1, &depthBuffer);
	//	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	//	return depthBuffer;
	//}

	GLuint createDepthBufferAttachment(int width, int height) {
		GLuint depthBuffer;
		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

		//glTexImage2D(
		//	GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
		//	GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
		//);

		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
		
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			int j = 0;
		}
		return depthBuffer;
	}


	void bindFrameBuffer(GLuint fbo, int width, int height) {
		glBindTexture(GL_TEXTURE_2D, 0);// make sure the texture isn't bound
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
	}

	void unbindCurrentFrameBuffer(int width, int height) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
	}

	void ResizeCustomRenderCanvas(glm::vec2 newSize) {
		// resize color attachment
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newSize.x, newSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		// resize depth attachment
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, newSize.x, newSize.y);
	/*	glTexImage2D(
			GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0,
			GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL
		);*/

		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// update internal dimensions
		renderSize = newSize;
	}

	//void ResizeCustomRenderCanvas(glm::vec2 newSize) {
	//	// resize color attachment
	//	glBindTexture(GL_TEXTURE_2D, texture);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newSize.x, newSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	//	glBindTexture(GL_TEXTURE_2D, 0);

	//	// resize depth attachment
	//	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newSize.x, newSize.y);
	//	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	//	// update internal dimensions
	//	renderSize = newSize;
	//}

public:
	std::function<bool()> customRenderFunc;
	glm::vec2 renderSize = glm::vec3(1);

	virtual bool Init()
	{

		fbo = createFrameBuffer();
		texture = createTextureAttachment(renderSize.x, renderSize.y);
		depthBuffer = createDepthBufferAttachment(renderSize.x, renderSize.y);
		unbindCurrentFrameBuffer(renderSize.x, renderSize.y);

		return true;
	}

	void HandleResize()
	{
		// handle custom render window resize
		glm::vec2 vMin = ImGui::GetWindowContentRegionMin();
		glm::vec2 vMax = ImGui::GetWindowContentRegionMax();

		glm::vec2 newSize = vMax - vMin;

		if (renderSize != newSize)
		{
			ResizeCustomRenderCanvas(newSize);
		}
	}

	virtual bool Design()
	{
		ImGui::Begin("Custom render");


		bindFrameBuffer(fbo, renderSize.x, renderSize.y);

		if (!customRenderFunc())
			return false;

		unbindCurrentFrameBuffer(renderSize.x, renderSize.y);

		//glBindRenderbuffer(GL_RENDERBUFFER, 0);
		ImGui::Image((void*)(intptr_t)texture, renderSize);
		
		HandleResize();

		ImGui::End();

		return true;
	}

	virtual bool OnExit()
	{
		glDeleteFramebuffers(1, &fbo);
		glDeleteTextures(1, &texture);

		return true;
	}
};

class CameraPropertiesWindow : Window
{
public:
	StereoCamera* Camera;

	virtual bool Init()
	{
		return true;
	}

	virtual bool Design()
	{
		ImGui::Begin("Camera Properties");                          // Create a window called "Hello, world!" and append into it.
		ImGui::InputFloat3("position", (float*)&Camera->position, "%f", 0);
		ImGui::InputFloat2("view center", (float*)&Camera->viewCenter, "%f", 0);
		ImGui::InputFloat2("viewsize", (float*)Camera->viewSize, "%f", 0);

		ImGui::InputFloat3("transformVec", (float*)&Camera->transformVec, "%f", 0);
		ImGui::InputFloat3("left", (float*)&Camera->left, "%f", 0);
		ImGui::InputFloat3("up", (float*)&Camera->up, "%f", 0);
		ImGui::InputFloat3("forward", (float*)&Camera->forward, "%f", 0);

		ImGui::SliderFloat("eyeToCenterDistanceSlider", (float*)&Camera->eyeToCenterDistance, 0, 1, "%.2f", 1);
		ImGui::InputFloat("eyeToCenterDistance", (float*)&Camera->eyeToCenterDistance, 0.01, 0.1, "%.2f", 0);

		ImGui::End();
		return true;
	}

	virtual bool OnExit()
	{
		return true;
	}
};

class CrossPropertiesWindow : Window
{
public:
	Cross* Cross;

	virtual bool Init()
	{
		return true;
	}

	virtual bool Design()
	{
		ImGui::Begin("Cross Properties");                          // Create a window called "Hello, world!" and append into it.
		if (ImGui::InputFloat3("position", (float*)& Cross->Position, "%f", 0)
			|| ImGui::SliderFloat("size", (float*)& Cross->size, 1e-3, 10, "%.3f", 2))
		{
			Cross->Refresh();
		}

	/*	ImGui::InputFloat2("view center", (float*)& Camera->viewCenter, "%f", 0);
		ImGui::InputFloat2("viewsize", (float*)Camera->viewSize, "%f", 0);

		ImGui::InputFloat3("transformVec", (float*)& Camera->transformVec, "%f", 0);
		ImGui::InputFloat3("left", (float*)& Camera->left, "%f", 0);
		ImGui::InputFloat3("up", (float*)& Camera->up, "%f", 0);
		ImGui::InputFloat3("forward", (float*)& Camera->forward, "%f", 0);

		ImGui::SliderFloat("eyeToCenterDistanceSlider", (float*)& Camera->eyeToCenterDistance, 0, 1, "%.2f", 1);
		ImGui::InputFloat("eyeToCenterDistance", (float*)& Camera->eyeToCenterDistance, 0.01, 0.1, "%.2f", 0);*/

		ImGui::End();
		return true;
	}

	virtual bool OnExit()
	{
		return true;
	}
};
