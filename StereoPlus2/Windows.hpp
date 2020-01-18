#pragma once
#include "GLLoader.hpp"
#include "Window.hpp"
#include <functional>
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include "Tools.hpp"
#include <set>
#include <sstream>
#include <imgui/imgui_internal.h>

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


		ImGui::SliderFloat("eyeToCenterDistanceSlider", (float*)& obj->eyeToCenterDistance, 0, 1, "%.2f", 1);
		ImGui::InputFloat("eyeToCenterDistance", (float*)& obj->eyeToCenterDistance, 0.01, 0.1, "%.2f", 0);
		return true;
	}
};


class SceneObjectInspectorWindow : Window, MoveCommand::IHolder
{
	MoveCommand* moveCommand;

	static int& GetID() {
		static int val = 0;
		return val;
	}


	bool DesignRootNode(GroupObject* t) {
		ImGui::PushID(GetID()++);

		ImGuiDragDropFlags target_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;
		bool open = ImGui::TreeNodeEx(t->Name.c_str(), target_flags);

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("SceneObjects", target_flags))
			{
				ScheduleMove(&t->Children, 0, GetBuffer(payload->Data), GetPosition(Center));
			}
			ImGui::EndDragDropTarget();
		}

		for (size_t i = 0; i < t->Children.size(); i++)
			if (!DesignTreeNode(t->Children[i], t->Children, i))
			{
				ImGui::TreePop();
				ImGui::PopID();

				return false;
			}

		ImGui::TreePop();
		ImGui::PopID();

		return true;
	}
	bool DesignTreeNode(SceneObject* t, std::vector<SceneObject*>& source, int pos) {
		switch (t->GetType()) {
		case Group:
			return DesignTreeNode((GroupObject*)t, source, pos);
		case Leaf:
		case StereoLineT:
		case StereoPolyLineT:
			return DesignTreeLeaf((LeafObject*)t, source, pos);
		}

		std::cout << "Invalid SceneObject type passed" << std::endl;
		return false;
	}
	bool DesignTreeNode(GroupObject* t, std::vector<SceneObject*>& source, int pos) {
		ImGui::PushID(GetID()++);

		ImGui::Indent(indent);
		bool open = ImGui::TreeNode(t->Name.c_str());

		ImGuiDragDropFlags src_flags = 0;
		src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
		src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
		//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip
		if (ImGui::BeginDragDropSource(src_flags))
		{
			if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
				ImGui::Text("Moving \"%s\"", t->Name.c_str());
			ObjectPointer item;
			item.source = &source;
			item.pos = pos;
			selectedObjectsBuffer->emplace(item);

			ImGui::SetDragDropPayload("SceneObjects", &selectedObjectsBuffer, sizeof(std::set<ObjectPointer, ObjectPointerComparator>*));

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("SceneObjects", target_flags))
			{
				MoveCommandPosition relativePosition = GetPosition(Any);
				if (relativePosition == Center)
					ScheduleMove(&t->Children, 0, GetBuffer(payload->Data), relativePosition);
				else
					ScheduleMove(&source, pos, GetBuffer(payload->Data), relativePosition);
			}
			ImGui::EndDragDropTarget();
		}

		if (open)
		{
			for (size_t i = 0; i < t->Children.size(); i++)
				if (!DesignTreeNode(t->Children[i], t->Children, i))
				{
					ImGui::TreePop();
					ImGui::PopID();

					return false;
				}

			ImGui::TreePop();
		}


		ImGui::Unindent(indent);
		ImGui::PopID();

		return true;
	}
	bool DesignTreeLeaf(LeafObject*  t, std::vector<SceneObject*>& source, int pos) {
		ImGui::PushID(GetID()++);

		ImGui::Indent(indent);

		ImGui::Selectable(t->Name.c_str());

		ImGuiDragDropFlags src_flags = 0;
		src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
		src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
		//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip
		if (ImGui::BeginDragDropSource(src_flags))
		{
			if (!(src_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
				ImGui::Text("Moving \"%s\"", t->Name.c_str());
			ObjectPointer item;
			item.source = &source;
			item.pos = pos;
			selectedObjectsBuffer->emplace(item);

			//SceneObjectClipBoard::Push(selectedObjectsBuffer);
			ImGui::SetDragDropPayload("SceneObjects", &selectedObjectsBuffer, sizeof(std::set<ObjectPointer, ObjectPointerComparator>*));

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("SceneObjects", target_flags))
			{
				ScheduleMove(&source, pos, GetBuffer(payload->Data), GetPosition(Top | Bottom));
			}
			ImGui::EndDragDropTarget();
		}


		ImGui::Unindent(indent);
		ImGui::PopID();

		return true;
	}

	MoveCommandPosition GetPosition(int positionMask) {
		glm::vec2 nodeScreenPos = ImGui::GetCursorScreenPos();
		glm::vec2 size = ImGui::GetItemRectSize();
		glm::vec2 mouseScreenPos = ImGui::GetMousePos();

		// vertical center centered relative position.
		float vertPos = mouseScreenPos.y - nodeScreenPos.y + size.y / magicNumber;

		if (positionMask == Center)
			return Center;
		if (positionMask == Top)
			return Top;
		if (positionMask == Bottom)
			return Bottom;

		if ((positionMask & Center) == 0)
		{
			if (vertPos > 0)
				return Bottom;

			return Top;
		}

		if (vertPos > centerSizeHalf)
			return Bottom;

		else if (vertPos < -centerSizeHalf)
			return Top;

		return Center;
	}

	std::set<ObjectPointer, ObjectPointerComparator>* GetBuffer(void* data) {
		return *(std::set<ObjectPointer, ObjectPointerComparator> * *)data;
	}

	void ScheduleMove(std::vector<SceneObject*> * target, int targetPos, std::set<ObjectPointer, ObjectPointerComparator> * items, MoveCommandPosition pos) {
		if (isCommandEmpty)
		{
			moveCommand = new MoveCommand();
			isCommandEmpty = false;
		}

		moveCommand->SetReady();
		moveCommand->target = target;
		moveCommand->targetPos = targetPos;
		moveCommand->items = items;
		moveCommand->pos = pos;
		moveCommand->caller = (IHolder*)this;
	}

public:
	std::set<ObjectPointer, ObjectPointerComparator>* selectedObjectsBuffer;

	GroupObject* rootObject;
	float indent = 1;
	float centerSizeHalf = 3;

	// We divide height by this number. 
	// For some reason height/2 isn't center.
	const float magicNumber = 1.25;

	virtual bool Init()
	{
		return true;
	}

	virtual bool Design()
	{
		ImGui::Begin("Object inspector");                         

		// Reset elements' IDs.
		GetID() = 0;

		DesignRootNode(rootObject);

		ImGui::End();
		return true;
	}

	virtual bool OnExit()
	{
		return true;
	}
};


class CreatingToolWindow : Window {
	CreatingTool<StereoLine> lineTool;
	CreatingTool<StereoPolyLine> polyLineTool;
public:
	Scene* scene = nullptr;

	virtual bool Init() {
		if (scene == nullptr)
		{
			std::cout << "Scene wasn't assigned" << std::endl;
			return false;
		}

		lineTool.BindScene(scene);
		lineTool.BindSource(&((GroupObject*)scene->objects[0])->Children);
		lineTool.initFunc = [](SceneObject * o) {
			static int id = 0;
			std::stringstream ss;
			ss << "Line" << id++;
			o->Name = ss.str();
			return true;
		};

		polyLineTool.BindScene(scene);
		polyLineTool.BindSource(&((GroupObject*)scene->objects[0])->Children);
		polyLineTool.initFunc = [](SceneObject * o) {
			static int id = 0;
			std::stringstream ss;
			ss << "PolyLine" << id++;
			o->Name = ss.str();
			return true;
		};

		return true;
	}
	virtual bool Design() {
		ImGui::Begin("Creating tool window");

		if (ImGui::Button("Line")) {
			lineTool.Create(nullptr);
		}

		if (ImGui::Button("PolyLine")) {
			polyLineTool.Create(nullptr);
		}

		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}
};


template<ObjectType type>
class PointPenToolWindow : Window
{
	SceneObject** target = nullptr;

	std::stack<bool>& GetIsActive() {
		static std::stack<bool> val;
		return val;
	}
	bool IsActive(bool isActive) {
		GetIsActive().push(isActive);
		if (!isActive)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		return true;
	}
	void PopIsActive() {
		auto isActive = GetIsActive().top();
		GetIsActive().pop();

		if (!isActive)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}

	std::string GetName(ObjectType type) {
		switch (type)
		{
		case Group:
		case Leaf:
		case StereoLineT:
			return "noname";
		case StereoPolyLineT:
			return "PolyLine";
		default:
			return "noname";
		}
	}
	std::string GetName(ObjectType type, SceneObject** obj) {
		return
			(*obj) != nullptr && type == (*obj)->GetType()
			? (*obj)->Name
			: "Empty";
	}

	std::set<ObjectPointer, ObjectPointerComparator>* GetBuffer(void* data) {
		return *(std::set<ObjectPointer, ObjectPointerComparator> * *)data;
	}

public:
	PointPenEditingTool<type>* tool = nullptr;

	virtual bool Init() {
		if (tool == nullptr)
		{
			std::cout << "Tool wasn't assigned" << std::endl;
			return false;
		}

		target = tool->GetTarget();

		return true;
	}
	virtual bool Design() {
		ImGui::Begin(GetName(type).c_str());

		ImGui::Text(GetName(type, target).c_str());

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("SceneObjects", target_flags))
			{
				auto objectPointers = GetBuffer(payload->Data);

				if (objectPointers->size() > 1) {
					std::cout << "Drawing instrument can't accept multiple scene objects" << std::endl;
				}
				else {
					auto objectPointer = objectPointers->begin()._Ptr->_Myval;

					auto target = (*objectPointer.source)[objectPointer.pos];
					
					if (!tool->SelectTarget(target))
						return false;
				}

				// Clear selected scene object buffer.
				objectPointers->clear();
			}
			ImGui::EndDragDropTarget();
		}

		{
			bool isActive = (*target) != nullptr;
			if (IsActive(isActive))
			{
				if (ImGui::Button("Release"))
				{
					tool->ReleaseTarget();
				}
				PopIsActive();
			}
		}


		{
			static int mode = 0;
			if (ImGui::RadioButton("ImmediateMode", &mode, 0))
				tool->SetPointPenEditingToolMode(PointPenEditingToolMode::Immediate);
			if (ImGui::RadioButton("StepMode", &mode, 1))
				tool->SetPointPenEditingToolMode(PointPenEditingToolMode::Step);
		}

		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}
};


