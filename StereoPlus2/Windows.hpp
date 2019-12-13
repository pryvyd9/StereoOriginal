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

	GLuint createDepthBufferAttachment(int width, int height) {
		GLuint depthBuffer;
		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
		
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
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// update internal dimensions
		renderSize = newSize;
	}

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

//class CameraPropertiesWindow : Window
//{
//public:
//	StereoCamera* Camera;
//
//	virtual bool Init()
//	{
//		return true;
//	}
//
//	virtual bool Design()
//	{
//		ImGui::Begin("Camera Properties");                          // Create a window called "Hello, world!" and append into it.
//		ImGui::InputFloat3("position", (float*)&Camera->position, "%f", 0);
//		ImGui::InputFloat2("view center", (float*)&Camera->viewCenter, "%f", 0);
//		ImGui::InputFloat2("viewsize", (float*)Camera->viewSize, "%f", 0);
//
//		ImGui::InputFloat3("transformVec", (float*)&Camera->transformVec, "%f", 0);
//		ImGui::InputFloat3("left", (float*)&Camera->left, "%f", 0);
//		ImGui::InputFloat3("up", (float*)&Camera->up, "%f", 0);
//		ImGui::InputFloat3("forward", (float*)&Camera->forward, "%f", 0);
//
//		ImGui::SliderFloat("eyeToCenterDistanceSlider", (float*)&Camera->eyeToCenterDistance, 0, 1, "%.2f", 1);
//		ImGui::InputFloat("eyeToCenterDistance", (float*)&Camera->eyeToCenterDistance, 0.01, 0.1, "%.2f", 0);
//
//		ImGui::End();
//		return true;
//	}
//
//	virtual bool OnExit()
//	{
//		return true;
//	}
//};

template<typename T>
class SceneObjectPropertiesWindow : Window
{
public:
	T* Object;

	virtual bool Init()
	{
		if (!std::is_base_of<SceneObject, T>::value)
		{
			std::cout << "Unsupported type" << std::endl;

			return false;
		}

		return true;
	}

	virtual bool Design()
	{
		ImGui::Begin(("Properties " + GetName(Object)).c_str());

		if (!DesignProperties(Object))
			return false;

		ImGui::End();

		return true;
	}

	virtual bool OnExit()
	{
		return true;
	}

	std::string GetName(SceneObject* obj) {
		return obj->Name;
	}

	template<typename T>
	bool DesignProperties(T * obj) {
		return false;
	}

	template<>
	bool DesignProperties(StereoLine * obj) {
		return false;
	}

	template<>
	bool DesignProperties(Cross * obj) {
		if (ImGui::InputFloat3("position", (float*)& obj->Position, "%f", 0)
			|| ImGui::SliderFloat("size", (float*)& obj->size, 1e-3, 10, "%.3f", 2))
		{
			obj->Refresh();
		}
		return true;
	}

	template<>
	bool DesignProperties(StereoCamera* obj) {
		ImGui::InputFloat3("position", (float*)& obj->position, "%f", 0);
		ImGui::InputFloat2("view center", (float*)& obj->viewCenter, "%f", 0);
		ImGui::InputFloat2("viewsize", (float*)obj->viewSize, "%f", 0);

		ImGui::InputFloat3("transformVec", (float*)& obj->transformVec, "%f", 0);
		ImGui::InputFloat3("left", (float*)& obj->left, "%f", 0);
		ImGui::InputFloat3("up", (float*)& obj->up, "%f", 0);
		ImGui::InputFloat3("forward", (float*)& obj->forward, "%f", 0);

		ImGui::SliderFloat("eyeToCenterDistanceSlider", (float*)& obj->eyeToCenterDistance, 0, 1, "%.2f", 1);
		ImGui::InputFloat("eyeToCenterDistance", (float*)& obj->eyeToCenterDistance, 0.01, 0.1, "%.2f", 0);
		return true;
	}
};


class SceneObjectInspectorWindow : Window
{
public:
	GroupObject* rootObject;

	virtual bool Init()
	{
		return true;
	}

	void test() {
		// Reordering is actually a rather odd use case for the drag and drop API which is meant to carry data around. 
// Here we implement a little demo using the drag and drop primitives, but we could perfectly achieve the same results by using a mixture of
//  IsItemActive() on the source item + IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) on target items.
// This demo however serves as a pretext to demonstrate some of the flags you can use with BeginDragDropSource() and AcceptDragDropPayload().
		ImGui::BulletText("Drag and drop to re-order");
		ImGui::Indent();
		static const char* names[6] = { "1. Adbul", "2. Alfonso", "3. Aline", "4. Amelie", "5. Anna", "6. Arthur" };
		int move_from = -1, move_to = -1;
		for (int n = 0; n < IM_ARRAYSIZE(names); n++)
		{
			ImGui::Selectable(names[n]);

			ImGuiDragDropFlags src_flags = 0;
			src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
			src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
			//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip
			if (ImGui::BeginDragDropSource(src_flags))
			{
				if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
					ImGui::Text("Moving \"%s\"", names[n]);
				ImGui::SetDragDropPayload("DND_DEMO_NAME", &n, sizeof(int));
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				ImGuiDragDropFlags target_flags = 0;
				//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
				target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
				if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("DND_DEMO_NAME", target_flags))
				{
					move_from = *(const int*)payload->Data;
					move_to = n;
				}
				ImGui::EndDragDropTarget();
			}
		}

		if (move_from != -1 && move_to != -1)
		{
			// Reorder items
			int copy_dst = (move_from < move_to) ? move_from : move_to + 1;
			int copy_src = (move_from < move_to) ? move_from + 1 : move_to;
			int copy_count = (move_from < move_to) ? move_to - move_from : move_from - move_to;
			const char* tmp = names[move_from];
			//printf("[%05d] move %d->%d (copy %d..%d to %d..%d)\n", ImGui::GetFrameCount(), move_from, move_to, copy_src, copy_src + copy_count - 1, copy_dst, copy_dst + copy_count - 1);
			memmove(&names[copy_dst], &names[copy_src], (size_t)copy_count * sizeof(const char*));
			names[move_to] = tmp;
			ImGui::SetDragDropPayload("DND_DEMO_NAME", &move_to, sizeof(int)); // Update payload immediately so on the next frame if we move the mouse to an earlier item our index payload will be correct. This is odd and showcase how the DnD api isn't best presented in this example.
		}
		ImGui::Unindent();
	}

	virtual bool Design()
	{
		ImGui::Begin("Object inspector");                         

		test();

		bool open = ImGui::TreeNode("Test");
		if (ImGui::BeginDragDropTarget())
		{
			if (ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
			{
			}
			ImGui::EndDragDropTarget();
		}
		if (open)
		{
			ImGui::Text("Hello");
			ImGui::TreePop();
		}


		ImGui::End();
		return true;
	}

	virtual bool OnExit()
	{
		return true;
	}
};
