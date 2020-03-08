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

	SceneObjectBuffer::Buffer GetDragDropBuffer(ImGuiDragDropFlags target_flags) {
		return SceneObjectBuffer::GetDragDropPayload("SceneObjects", target_flags);
	}
	void EmplaceDragDropObject(ObjectPointer objectPointer) {
		SceneObjectBuffer::EmplaceDragDropSceneObject("SceneObjects", objectPointer, &selectedObjectsBuffer);
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
			if (auto buffer = GetDragDropBuffer(target_flags))
			{
				ScheduleMove(&t->Children, 0, buffer, GetPosition(Center));
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
		case MeshT:
		case LineMeshT:
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

			EmplaceDragDropObject(ObjectPointer{ &source , pos });

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (auto buffer = GetDragDropBuffer(target_flags))
			{
				MoveCommandPosition relativePosition = GetPosition(Any);
				if (relativePosition == Center)
					ScheduleMove(&t->Children, 0, buffer, relativePosition);
				else
					ScheduleMove(&source, pos, buffer, relativePosition);
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
			
			EmplaceDragDropObject(ObjectPointer{ &source , pos });
			
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags target_flags = 0;
			//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
			//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
			if (auto buffer = GetDragDropBuffer(target_flags))
			{
				ScheduleMove(&source, pos, buffer, GetPosition(Top | Bottom));
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
		lineTool.BindSource(&((GroupObject*)scene->root)->Children);
		lineTool.func = [](SceneObject * o) {
			static int id = 0;
			std::stringstream ss;
			ss << "Line" << id++;
			o->Name = ss.str();
		};

		polyLineTool.BindScene(scene);
		polyLineTool.BindSource(&((GroupObject*)scene->root)->Children);
		polyLineTool.func = [](SceneObject * o) {
			static int id = 0;
			std::stringstream ss;
			ss << "PolyLine" << id++;
			o->Name = ss.str();
		};

		return true;
	}
	virtual bool Design() {
		ImGui::Begin("Creating tool window");
		
		if (ImGui::Button("Line")) {
			lineTool.Create();
		}

		if (ImGui::Button("PolyLine")) {
			polyLineTool.Create();
		}

		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}
};


template<ObjectType type>
class PointPenToolWindow : Window, Attributes
{
	// If this is null then the window probably wasn't initialized.
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

	bool DesignInternal() {
		ImGui::Text(GetName(type, target).c_str());

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

		{
			bool isActive = (*target) != nullptr;
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
	PointPenEditingTool<type>* tool = nullptr;

	virtual bool Init() {
		if (tool == nullptr)
		{
			std::cout << "Tool wasn't assigned" << std::endl;
			return false;
		}
		target = tool->GetTarget();
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
		return true;
	}
};


template<ObjectType type>
class ExtrusionToolWindow : Window, Attributes
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

	bool DesignInternal() {
		ImGui::Text(GetName(type, target).c_str());

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

		if (IsActive(*target != nullptr))
		{
			if (ImGui::Button("Release"))
			{
				tool->UnbindSceneObjects();
			}
			PopIsActive();
		}

		if (IsActive(*target != nullptr))
		{
			if (ImGui::Button("New"))
			{
				tool->Create();
			}
			PopIsActive();
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
	ExtrusionEditingTool<type>* tool = nullptr;

	virtual bool Init() {
		if (tool == nullptr)
		{
			std::cout << "Tool wasn't assigned" << std::endl;
			return false;
		}

		target = tool->GetTarget();
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
		return true;
	}
};


class AttributesWindow : Window {
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

		if (targetAttributes != nullptr && !targetAttributes->Design())
			return false;

		ImGui::EndTabBar();
		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}

	bool BindTool(Attributes* toolAttributes) {
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
		this->targetAttributes = targetAttributes;
		if (!toolAttributes->IsInitialized())
			return toolAttributes->Init();
		return true;
	}
	bool UnbindTarget() {
		targetAttributes = nullptr;
		return true;
	}
};

class ToolWindow : Window {
	template<typename TWindow, typename TTool>
	void ApplyTool() {
		auto tool = new TWindow();
		tool->tool = ToolPool::GetTool<TTool>();

		attributesWindow->UnbindTool();
		attributesWindow->BindTool((Attributes*)tool);
		attributesWindow->onUnbindTool = [t = tool] {
			delete t;
		};
	}
public:
	AttributesWindow* attributesWindow;

	virtual bool Init() {
		if (attributesWindow == nullptr)
		{
			std::cout << "AttributesWindow was null" << std::endl;
			return false;
		}

		return true;
	}
	virtual bool Design() {
		ImGui::Begin("Toolbar");

		if (ImGui::Button("extrusion")) {
			ApplyTool<ExtrusionToolWindow<StereoPolyLineT>, ExtrusionEditingTool<StereoPolyLineT>>();
		}

		if (ImGui::Button("penTool")) {
			ApplyTool<PointPenToolWindow<StereoPolyLineT>, PointPenEditingTool<StereoPolyLineT>>();
		}

		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}

};

class OpenFileWindow : Window {
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
	};

	Path path;

	void ListFiles() {
		ImGui::ListBoxHeader("");

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

		for (const auto& a : files) {
			const std::string fileName = a.path().filename().u8string();
			ImGui::Selectable(fileName.c_str());
		}

		ImGui::ListBoxFooter();
	}


public:
	virtual bool Init() {
		path.apply(".");

		return true;
	}


	virtual bool Design() {
		ImGui::Begin("Open File");

		{
			ImGui::InputText("Path", &path.getBuffer());

			if (ImGui::Button("Submit"))
				path.apply();
		}

		ListFiles();

		if (ImGui::Button("Open")) {

		}

		if (ImGui::Button("Cancel")) {

		}

		ImGui::End();

		return true;
	}
	virtual bool OnExit() {
		return true;
	}

};