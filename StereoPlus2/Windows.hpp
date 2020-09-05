#pragma once
#include "GLLoader.hpp"
#include "Window.hpp"
#include <functional>
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include "Tools.hpp"
#include "ToolPool.hpp"
#include <set>
#include <sstream>
#include <imgui/imgui_internal.h>
#include <string>
#include <filesystem> // C++17 standard header file name
#include "include/imgui/imgui_stdlib.h"
#include "FileManager.hpp"
//#include <experimental/type_traits>
#include "TemplateExtensions.hpp"
#include "InfrastructureTypes.hpp"


namespace ImGui::Extensions {
	#include <stack>
	static std::stack<bool>& GetIsActive() {
		static std::stack<bool> val;
		return val;
	}
	static bool PushActive(bool isActive) {
		ImGui::Extensions::GetIsActive().push(isActive);
		if (!isActive)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		return true;
	}
	static void PopActive() {
		auto isActive = ImGui::Extensions::GetIsActive().top();
		ImGui::Extensions::GetIsActive().pop();

		if (!isActive)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}
}

namespace fs = std::filesystem;

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
		ImGui::Begin("Scene");


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

class SceneObjectPropertiesWindow : Window, Attributes {
	bool DesignInternal() {
		Object->DesignProperties();

		return true;
	}

public:
	SceneObject* Object;

	virtual SceneObject* GetTarget() {
		return Object;
	}

	virtual bool Init() {
		return true;
	}

	virtual bool Window::Design() {
		if (Object == nullptr) {
			ImGui::Begin("Properties empty");
			ImGui::End();
			return true;
		}

		ImGui::Begin(("Properties " + GetName(Object)).c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		if (Object == nullptr) {
			return true;
		}

		if (ImGui::BeginTabItem(("Properties " + GetName(Object)).c_str())) {
			if (!DesignInternal())
				return false;

			ImGui::EndTabItem();
		}

		return true;
	}



	virtual bool OnExit() {
		return true;
	}

	std::string GetName(SceneObject* obj) {
		return obj->Name;
	}

	virtual void BindTarget(SceneObject* o) {
		Object = o;
	}
	virtual void UnbindTargets() {
		//target = nullptr;
	}
};


class SceneObjectInspectorWindow : Window, MoveCommand::IHolder {
	const Log log = Log::For<SceneObjectInspectorWindow>();

	MoveCommand* moveCommand;

	static int& GetID() {
		static int val = 0;
		return val;
	}

	SceneObjectBuffer::Buffer GetDragDropBuffer(ImGuiDragDropFlags target_flags) {
		return SceneObjectBuffer::GetDragDropPayload("SceneObjects", target_flags);
	}
	void EmplaceDragDropObject(SceneObject* objectPointer) {
		SceneObjectBuffer::EmplaceDragDropSceneObject("SceneObjects", objectPointer, &selectedObjectsBuffer);
	}

	bool DesignRootNode(GroupObject* t) {
		if (t == nullptr) {
			ImGui::Text("No scene loaded. Nothing to show");
			return true;
		}

		ImGui::PushID(GetID()++);

		ImGuiDragDropFlags target_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
		bool open = ImGui::TreeNodeEx(t->Name.c_str(), target_flags);

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (auto buffer = GetDragDropBuffer(target_flags))
			{
				ScheduleMove(t, 0, buffer, GetPosition(Center));
			}
			ImGui::EndDragDropTarget();
		}

		for (size_t i = 0; i < t->children.size(); i++)
			if (!DesignTreeNode(t->children[i], t->children, i))
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
		case StereoPolyLineT:
		case MeshT:
			return DesignTreeLeaf((LeafObject*)t, source, pos);
		case CrossT:
			return true;
		}

		log.Error("Invalid SceneObject type passed: ", t->GetType());
		//std::cout << "[" << "Error" << "]" << "(" << class SceneObjectInspectorWindow 
		// << ")" << "Invalid SceneObject type passed: " << t->GetType() << std::endl;
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

			EmplaceDragDropObject(t);

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (auto buffer = GetDragDropBuffer(target_flags))
			{
				InsertPosition relativePosition = GetPosition(Any);
				if (relativePosition == Center)
					ScheduleMove(t, 0, buffer, relativePosition);
				else
					ScheduleMove(const_cast<SceneObject*>(t->GetParent()), pos, buffer, relativePosition);
			}
			ImGui::EndDragDropTarget();
		}

		if (open)
		{
			for (size_t i = 0; i < t->children.size(); i++)
				if (!DesignTreeNode(t->children[i], t->children, i))
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
			
			EmplaceDragDropObject(t);
			
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (auto buffer = GetDragDropBuffer(target_flags))
			{
				ScheduleMove(const_cast<SceneObject*>(t->GetParent()), pos, buffer, GetPosition(Top | Bottom));
			}
			ImGui::EndDragDropTarget();
		}


		ImGui::Unindent(indent);
		ImGui::PopID();

		return true;
	}

	InsertPosition GetPosition(int positionMask) {
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

		if ((positionMask & Center) == 0) {
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

	void ScheduleMove(SceneObject* target, int targetPos, std::set<SceneObject*> * items, InsertPosition pos) {
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
	std::set<SceneObject*>* selectedObjectsBuffer;

	GroupObject** rootObject;
	float indent = 1;
	float centerSizeHalf = 3;

	// We divide height by this number. 
	// For some reason height/2 isn't center.
	const float magicNumber = 1.25;

	virtual bool Init() {
		return true;
	}
	virtual bool Design()
	{
		ImGui::Begin("Object inspector");                         

		// Reset elements' IDs.
		GetID() = 0;

		DesignRootNode(*rootObject);

		ImGui::End();
		return true;
	}
	virtual bool OnExit()
	{
		return true;
	}
};




template<ObjectType type>
class PointPenToolWindow : Window, Attributes {
	const Log log = Log::For<SceneObjectInspectorWindow>();


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
		case StereoPolyLineT:
			return "PolyLine";
		default:
			return "noname";
		}
	}
	std::string GetName(ObjectType type, SceneObject* obj) {
		return
			(obj) != nullptr && type == (obj)->GetType()
			? (obj)->Name
			: "Empty";
	}

	bool DesignInternal() {
		ImGui::Text(GetName(type, GetTarget()).c_str());

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			std::vector<SceneObject*> objects;
			if (SceneObjectBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects))
			{
				if (objects.size() > 1) {
					log.Warning("Drawing instrument can't accept multiple scene objects");
				}
				else {
					if (!tool->BindSceneObjects(objects))
						return false;
				}
			}

			ImGui::EndDragDropTarget();
		}

		{
			bool isActive = (GetTarget()) != nullptr;
			if (IsActive(isActive))
			{
				if (ImGui::Button("Release"))
				{
					tool->UnbindSceneObjects();
				}
				PopIsActive();
			}
		}


		{
			static int mode = 0;
			if (ImGui::RadioButton("ImmediateMode", &mode, 0))
				tool->SetMode(PointPenEditingToolMode::Immediate);
			if (ImGui::RadioButton("StepMode", &mode, 1))
				tool->SetMode(PointPenEditingToolMode::Step);
		}

		return true;
	}

public:
	// If this is null then the window probably wasn't initialized.
	//SceneObject* target = nullptr;

	PointPenEditingTool<type>* tool = nullptr;

	virtual SceneObject* GetTarget() {
		if (tool == nullptr)
			return nullptr;

		return tool->GetTarget();
	}
	virtual bool Init() {
		if (tool == nullptr) {
			log.Error("Tool wasn't assigned");
			return false;
		}
		
		//target = tool->GetTarget();
		Window::name = Attributes::name = "PointPen " + GetName(type);
		Attributes::isInitialized = true;

		return true;
	}
	virtual bool Window::Design() {
		ImGui::Begin(Window::name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		if (ImGui::BeginTabItem(Attributes::name.c_str()))
		{
			if (!DesignInternal())
				return false;

			ImGui::EndTabItem();
		}

		return true;
	}
	virtual bool OnExit() {
		UnbindTargets();
		return true;
	}
	virtual void UnbindTargets() {
		//target = nullptr;
	}
};


template<ObjectType type>
class ExtrusionToolWindow : Window, Attributes {
	
	std::string GetName(ObjectType type) {
		switch (type)
		{
		case StereoPolyLineT:
			return "PolyLine";
		default:
			return "noname";
		}
	}
	std::string GetName(ObjectType type, SceneObject* obj) {
		return 
			(obj) != nullptr && type == (obj)->GetType()
			? (obj)->Name
			: "Empty";
	}

	bool DesignInternal() {
		ImGui::Text(GetName(type, GetTarget()).c_str());

		if (ImGui::BeginDragDropTarget()) {
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			std::vector<SceneObject*> objects;
			if (SceneObjectBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects)) {
				if (objects.size() > 1)
					std::cout << "Drawing instrument can't accept multiple scene objects" << std::endl;
				else if (!tool->BindSceneObjects(objects))
					return false;
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::Extensions::PushActive(GetTarget() != nullptr)) {
			if (ImGui::Button("Release"))
				tool->UnbindSceneObjects();
			ImGui::Extensions::PopActive();
		}

		if (ImGui::Extensions::PushActive(GetTarget() != nullptr)) {
			if (ImGui::Button("New"))
				tool->Create();
			ImGui::Extensions::PopActive();
		}


		{
			static int mode = 0;
			if (ImGui::RadioButton("ImmediateMode", &mode, 0))
				tool->SetMode(ExtrusionEditingToolMode::Immediate);
			if (ImGui::RadioButton("StepMode", &mode, 1))
				tool->SetMode(ExtrusionEditingToolMode::Step);
		}

		return true;
	}

public:
	//SceneObject* target = nullptr;

	ExtrusionEditingTool<type>* tool = nullptr;

	virtual SceneObject* GetTarget() {
		if (tool == nullptr)
			return nullptr;

		return tool->GetTarget();
	}
	virtual bool Init() {
		if (tool == nullptr)
		{
			std::cout << "Tool wasn't assigned" << std::endl;
			return false;
		}
		
		//target = tool->GetTarget();
		Window::name = Attributes::name = "Extrusion " + GetName(type);
		Attributes::isInitialized = true;
		
		return true;
	}
	virtual bool Window::Design() {
		ImGui::Begin(Window::name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		if (ImGui::BeginTabItem(Attributes::name.c_str()))
		{
			if (!DesignInternal())
				return false;

			ImGui::EndTabItem();
		}

		return true;
	}
	virtual bool OnExit() {
		UnbindTargets();
		return true;
	}
	virtual void UnbindTargets() {
		//target = nullptr;
	}
};



class TransformToolWindow : Window, Attributes {
	int maxPrecision = 5;
	int transformToolMode;

	std::string GetName(SceneObject* obj) {
		return
			(obj) != nullptr
			? (obj)->Name
			: "Empty";
	}

	int getPrecision(float v) {
		int precision = 0;
		for (size_t i = 0; i < maxPrecision; i++) {
			v *= 10;
			if ((int)v % 10 != 0)
				precision = i + 1;
		}
		return precision;
	}

	void DragVector(glm::vec3& v, std::string s1, std::string s2, std::string s3, float speed) {
		std::stringstream ss;
		ss << "%." << getPrecision(v.x) << "f";
		ImGui::DragFloat(s1.c_str(), &v.x, speed, 0, 0, ss.str().c_str());
		ss.str(std::string());
		ss << "%." << getPrecision(v.y) << "f";
		ImGui::DragFloat(s2.c_str(), &v.y, speed, 0, 0, ss.str().c_str());
		ss.str(std::string());
		ss << "%." << getPrecision(v.z) << "f";
		ImGui::DragFloat(s3.c_str(), &v.z, speed, 0, 0, ss.str().c_str());
	}
	void DragVector(glm::vec3& v, std::string s1, std::string s2, std::string s3, std::string f, float speed) {
		ImGui::DragFloat(s1.c_str(), &v.x, speed, 0, 0, f.c_str());
		ImGui::DragFloat(s2.c_str(), &v.y, speed, 0, 0, f.c_str());
		ImGui::DragFloat(s3.c_str(), &v.z, speed, 0, 0, f.c_str());
	}

	bool DesignInternal() {
		ImGui::Text(GetName(GetTarget()).c_str());

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			std::vector<SceneObject*> objects;
			if (SceneObjectBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects))
			{
				if (objects.size() > 1) {
					std::cout << "Drawing instrument can't accept multiple scene objects" << std::endl;
				}
				else {
					if (!tool->BindSceneObjects(objects))
						return false;
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::Extensions::PushActive(GetTarget() != nullptr))
		{
			if (ImGui::Button("Release"))
				tool->UnbindSceneObjects();
			if (ImGui::Button("Cancel"))
				tool->Cancel();

			ImGui::Extensions::PopActive();
		}

		transformToolMode = (int)tool->GetMode();
		{
			if (ImGui::RadioButton("Move", &transformToolMode, (int)TransformToolMode::Translate))
				tool->SetMode(TransformToolMode::Translate);
			if (ImGui::RadioButton("Scale", &transformToolMode, (int)TransformToolMode::Scale))
				tool->SetMode(TransformToolMode::Scale);
			if (ImGui::RadioButton("Rotate", &transformToolMode, (int)TransformToolMode::Rotate))
				tool->SetMode(TransformToolMode::Rotate);
		}

		if (transformToolMode == (int)TransformToolMode::Translate) {
			ImGui::Separator();
			DragVector(tool->transformPos, "X", "Y", "Z", "%.5f", 0.01);
		}
		else if (transformToolMode == (int)TransformToolMode::Scale) {
			ImGui::Separator();
			ImGui::DragFloat("scale", &tool->scale, 0.01, 0, 0, "%.2f");
		}
		else if (transformToolMode == (int)TransformToolMode::Rotate) {
			ImGui::Separator();
			DragVector(tool->angle, "X", "Y", "Z", 1);
		}

		return true;
	}

public:
	//SceneObject* target = nullptr;
	TransformTool* tool = nullptr;

	virtual SceneObject* GetTarget() {
		if (tool == nullptr)
			return nullptr;

		return tool->GetTarget();
	}

	virtual bool Init() {
		if (tool == nullptr)
		{
			std::cout << "Tool wasn't assigned" << std::endl;
			return false;
		}
		
		//target = tool->GetTarget();
		Window::name = Attributes::name = "Transformation";
		Attributes::isInitialized = true;
		
		return true;
	}
	virtual bool Window::Design() {
		ImGui::Begin(Window::name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		if (ImGui::BeginTabItem(Attributes::name.c_str()))
		{
			if (!DesignInternal())
				return false;

			ImGui::EndTabItem();
		}

		return true;
	}
	virtual bool OnExit() {
		UnbindTargets();
		return true;
	}
	virtual void UnbindTargets() {
		//target = nullptr;
	}
	

};


class AttributesWindow : public Window {
	Attributes* toolAttributes = nullptr;
	Attributes* targetAttributes = nullptr;
	
public:
	std::function<void()> onUnbindTool;

	virtual bool Init() {
		return true;
	}
	virtual bool Design() {
		ImGui::Begin("Attributes");

		ImGui::BeginTabBar("#attributes window tab bar");

		if (toolAttributes != nullptr && !toolAttributes->Design())
			return false;

		if (targetAttributes != nullptr && toolAttributes != nullptr && toolAttributes->GetTarget() != nullptr) {
			targetAttributes->BindTarget(toolAttributes->GetTarget());
			if (!targetAttributes->Design())
				return false;
		}
		

		ImGui::EndTabBar();
		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}

	bool BindTool(Attributes* toolAttributes) {
		if (this->toolAttributes != nullptr)
			this->toolAttributes->OnExit();

		this->toolAttributes = toolAttributes;
		if (!toolAttributes->IsInitialized())
			return toolAttributes->Init();
		return true;
	}
	bool UnbindTool() {
		try
		{
			onUnbindTool();
		}
		catch (const std::exception&)
		{

		}
		toolAttributes = nullptr;
		return true;
	}

	bool BindTarget(Attributes* targetAttributes) {
		if (this->targetAttributes != nullptr)
			this->targetAttributes->OnExit();

		this->targetAttributes = targetAttributes;
		if (!targetAttributes->IsInitialized())
			return targetAttributes->Init();
		return true;
	}
	bool UnbindTarget() {
		targetAttributes = nullptr;
		return true;
	}
};



class ToolWindow : Window {
	const Log log = Log::For<ToolWindow>();

	CreatingTool<StereoPolyLine> polyLineTool;
	CreatingTool<GroupObject> groupObjectTool;


	template<typename T>
	using unbindSceneObjects = decltype(std::declval<T>().UnbindSceneObjects());

	template<typename T>
	static constexpr bool hasUnbindSceneobjects = is_detected_v<unbindSceneObjects, T>;

	template<typename TWindow, typename TTool, std::enable_if_t<hasUnbindSceneobjects<TTool>> * = nullptr>
	void ApplyTool() {
		auto tool = new TWindow();
		tool->tool = ToolPool::GetTool<TTool>();

		auto targetWindow = new SceneObjectPropertiesWindow();
		attributesWindow->UnbindTarget();
		attributesWindow->UnbindTool();
		attributesWindow->BindTool((Attributes*)tool);
		attributesWindow->BindTarget((Attributes*)targetWindow);

		auto deleteAllhandlerId = scene->OnDeleteAll() += [t = tool] {
			t->UnbindTargets();
			t->tool->UnbindSceneObjects();
		};
		attributesWindow->onUnbindTool = [t = tool, d = deleteAllhandlerId, s = scene] {
			t->tool->UnbindSceneObjects();
			s->OnDeleteAll() -= d;
			t->OnExit();
			delete t;
		};
	}


	template<typename T>
	void ConfigureCreationTool(CreatingTool<T>& creatingTool, std::function<void(SceneObject*)> initFunc) {
		creatingTool.BindScene(scene);
		creatingTool.BindDestination(&scene->root);
		creatingTool.func = initFunc;
	}

public:
	AttributesWindow* attributesWindow;
	Scene* scene = nullptr;

	virtual bool Init() {
		if (attributesWindow == nullptr)
		{
			log.Error("AttributesWindow was null");
			return false;
		}

		if (scene == nullptr)
		{
			log.Error("Scene wasn't assigned");
			return false;
		}

		ConfigureCreationTool(polyLineTool, [](SceneObject* o) {
			static int id = 0;
			std::stringstream ss;
			ss << "PolyLine" << id++;
			o->Name = ss.str();
		});
		ConfigureCreationTool(groupObjectTool, [](SceneObject* o) {
			static int id = 0;
			std::stringstream ss;
			ss << "Group" << id++;
			o->Name = ss.str();
		});

		return true;
	}
	virtual bool Design() {
		ImGui::Begin("Toolbar");

		if (ImGui::Button("PolyLine"))
			polyLineTool.Create();
		if (ImGui::Button("Group"))
			groupObjectTool.Create();

		ImGui::Separator();

		if (ImGui::Button("extrusion")) 
			ApplyTool<ExtrusionToolWindow<StereoPolyLineT>, ExtrusionEditingTool<StereoPolyLineT>>();
		if (ImGui::Button("pen")) 
			ApplyTool<PointPenToolWindow<StereoPolyLineT>, PointPenEditingTool<StereoPolyLineT>>();
		if (ImGui::Button("transform"))
			ApplyTool<TransformToolWindow, TransformTool>();

		//{
		//	ImGui::Separator();
		//	static int v = (int)GlobalToolConfiguration::GetCoordinateMode();
		//	if (ImGui::RadioButton("Object", &v, (int)CoordinateMode::Object))
		//		GlobalToolConfiguration::SetCoordinateMode(CoordinateMode::Object);
		//	if (ImGui::RadioButton("Vertex", &v, (int)CoordinateMode::Vertex))
		//		GlobalToolConfiguration::SetCoordinateMode(CoordinateMode::Vertex);
		//}
		{
			ImGui::Separator();
			static int v = (int)GlobalToolConfiguration::SpaceMode().Get();
			if (ImGui::RadioButton("World", &v, (int)SpaceMode::World))
				GlobalToolConfiguration::SpaceMode().Set(SpaceMode::World);
			if (ImGui::RadioButton("Local", &v, (int)SpaceMode::Local))
				GlobalToolConfiguration::SpaceMode().Set(SpaceMode::Local);
		}
		{
			ImGui::Separator();
			ImGui::Text("Action on parent change:");
			static int v = (int)GlobalToolConfiguration::MoveCoordinateAction().Get();
			if (ImGui::RadioButton("Adapt Coordinates", &v, (int)MoveCoordinateAction::Adapt))
				GlobalToolConfiguration::MoveCoordinateAction().Set(MoveCoordinateAction::Adapt);
			if (ImGui::RadioButton("None", &v, (int)MoveCoordinateAction::None))
				GlobalToolConfiguration::MoveCoordinateAction().Set(MoveCoordinateAction::None);
		}


		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}

};

class FileWindow : public Window {
public:
	enum Mode {
		Load,
		Save,
	};
private:
	bool iequals(const std::string& a, const std::string& b)
	{
		return std::equal(a.begin(), a.end(),
			b.begin(), b.end(),
			[](char a, char b) {
				return tolower(a) == tolower(b);
			});
	}
	const Log log = Log::For<FileWindow>();

	class Path {
		fs::path path;
		std::string pathBuffer;
	public:
		const fs::path& get() {
			return path;
		}

		std::string& getBuffer() {
			return pathBuffer;
		}

		void apply() {
			apply(pathBuffer);
		}

		void apply(fs::path n) {
			path = fs::absolute(n);
			pathBuffer = path.u8string();
		}

		bool isSome() {
			return !pathBuffer.empty();
		}

		std::string join(Path path) {
			auto last = pathBuffer[pathBuffer.size() - 1];
		
			if (last == '/' || last == '\\')
				return pathBuffer + path.getBuffer();

			return pathBuffer + '/' + path.getBuffer();
		}
	};



	Path path;
	Path selectedFile;
	Scene* scene;
	


	void ListFiles() {
		if (ImGui::ListBoxHeader("")) {
			if (ImGui::Selectable(".."))
				path.apply(path.get().parent_path());

			std::vector<fs::directory_entry> folders;
			std::vector<fs::directory_entry> files;

			for (const auto& entry : fs::directory_iterator(path.get())) {
				if (entry.is_directory()) {
					folders.push_back(entry);
				}
				else if (entry.is_regular_file()) {
					files.push_back(entry);
				}
				else {
					std::cout << "Unknown file type was discovered" << entry.path() << std::endl;
				}
			}

			for (const auto& a : folders)
				if (const std::string directoryName = '[' + a.path().filename().u8string() + ']';
					ImGui::Selectable(directoryName.c_str())) {
				path.apply(a);
				ImGui::ListBoxFooter();
				return;
			}

			for (const auto& a : files)
				if (const std::string fileName = a.path().filename().u8string();
					ImGui::Selectable(fileName.c_str()))
					selectedFile.apply(a);


			ImGui::ListBoxFooter();
		}
	}


	void ShowPath() {
		ImGui::InputText("Path", &path.getBuffer());

		if (ImGui::Extensions::PushActive(path.isSome())) {
			if (ImGui::Button("Submit"))
				path.apply();

			ImGui::Extensions::PopActive();
		}
	}

	void CloseButton() {
		if (ImGui::Button("Cancel")) {
			shouldClose = true;
		}
	}

	template<Mode mode>
	bool Design() {
		log.Error("Unsupported mode given");
		return false;
	}

	template<>
	bool Design<Load>() {
		ImGui::Begin("Open File");

		ShowPath();

		ListFiles();

		ImGui::InputText("File", &selectedFile.getBuffer());

		if (ImGui::Extensions::PushActive(selectedFile.isSome())) {
			if (ImGui::Button("Open")) {
				try
				{
					if (selectedFile.get().is_absolute())
						FileManager::Load(selectedFile.getBuffer(), scene);
					else
						FileManager::Load(path.join(selectedFile), scene);

					shouldClose = true;
				}
				catch (const FileException& e)
				{
					// TODO: Show error message to user
				}
			}

			ImGui::Extensions::PopActive();
		}

		CloseButton();

		ImGui::End();

		return true;
	}


	template<>
	bool Design<Save>() {
		ImGui::Begin("Save File");

		ShowPath();

		ListFiles();

		ImGui::InputText("File", &selectedFile.getBuffer());

		if (ImGui::Extensions::PushActive(selectedFile.isSome())) {
			if (ImGui::Button("Save")) {
				try
				{
					if (selectedFile.get().is_absolute())
						FileManager::Save(selectedFile.getBuffer(), scene);
					else
						FileManager::Save(path.join(selectedFile), scene);
					
					shouldClose = true;
				}
				catch (const FileException & e)
				{
					log.Error("Failed to save file");
				}
			}

			ImGui::Extensions::PopActive();
		}

		CloseButton();

		ImGui::End();

		return true;
	}

public:

	Mode mode;

	virtual bool Init() {
		if (!scene) {
			log.Error("Scene was null");
			return false;
		}

		path.apply(".");

		return true;
	}


	virtual bool Design() {
		switch (mode)
		{
		case FileWindow::Load:
			return Design<Load>();
		case FileWindow::Save:
			return Design<Save>();
		default:
			log.Error("Unsupported mode given");
			return false;
		}
	}

	bool BindScene(Scene* scene) {
		if (this->scene = scene)
			return true;

		log.Error("Scene was null");
		return false;
	}

};