#pragma once
#include "GLLoader.hpp"
#include "Window.hpp"
#include <functional>
#include "DomainUtils.hpp"
#include "Commands.hpp"
#include "Tools.hpp"
#include "ToolPool.hpp"
#include <set>
#include <sstream>
#include <string>
#include "include/imgui/imgui_stdlib.h"
#include "FileManager.hpp"
#include "TemplateExtensions.hpp"
#include "InfrastructureTypes.hpp"
#include "Localization.hpp"
#include "ImGuiExtensions.hpp"
#include "include/stb/stb_image_write.h"


namespace ImGui::Extensions {
	static bool RadioButton(const std::string& localizationPath, int* v, int v_button, const std::string& toolTipLocalizationPath) {
		auto res = ImGui::RadioButton(LocaleProvider::GetC(localizationPath), v, v_button);
		ImGui::SameLine();
		ImGui::Extensions::HelpMarker(LocaleProvider::GetC(toolTipLocalizationPath));
		return res;
	}
	static bool RadioButton(const std::string& localizationPath, int* v, int v_button) {
		return ImGui::RadioButton(LocaleProvider::GetC(localizationPath), v, v_button);
	}
}

//class TemplateWindow : Window {
//	const Log log = Log::For<TemplateWindow>();
//public:
//	virtual bool Init() {
//		Window::name = "templateWindow";
//
//		return true;
//	}
//	virtual bool Design() {
//		return true;
//	}
//	virtual bool OnExit() {
//		return true;
//	}
//};

class CustomRenderWindow : Window {
	GLuint fbo;
	GLuint texture;
	GLuint depthBuffer;
	GLuint depthBufferTexture;

	glm::vec4 windowBackgroundColor = glm::vec4(0, 0, 0, 1);

	Event<> onResize;


	void createFrameBuffer() {
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		//int g = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		//	int u = 0;
		//}
	}
	void createTextureAttachment(int width, int height) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
	}
	void createDepthBufferAttachment(int width, int height) {
		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
		
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			int j = 0;
		}
	}


	void bindFrameBuffer(GLuint fbo, int width, int height) {
		glBindTexture(GL_TEXTURE_2D, 0);// make sure the texture isn't bound
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
	}
	void unbindCurrentFrameBuffer(int width, int height) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
		RenderSize = newSize;
	}

	void saveImage(const char* filepath, int width, int height) {
		GLsizei nrChannels = 3;
		GLsizei stride = nrChannels * width;
		stride += (stride % 4) ? (4 - stride % 4) : 0;
		GLsizei bufferSize = stride * height;
		std::vector<char> buffer(bufferSize);
		glPixelStorei(GL_PACK_ALIGNMENT, 4);
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);
	}

	void RenderToFileAdvanced() {
		if (!shouldSaveAdvancedImage.Get())
			return;

		shouldSaveAdvancedImage = false;

		auto copyRenderSize = RenderSize.Get();
		auto nrs = glm::vec2(4000, 4000);

		ResizeCustomRenderCanvas(nrs);
		onResize.Invoke();

		bindFrameBuffer(fbo, RenderSize->x, RenderSize->y);


		customRenderFunc();

		std::stringstream ss;
		ss << "image_" << Time::GetTime() << "a.png";
		saveImage(ss.str().c_str(), RenderSize->x, RenderSize->y);

		ResizeCustomRenderCanvas(copyRenderSize);
		onResize.Invoke();
	}
	void RenderToFileBasic() {
		if (!shouldSaveViewportImage.Get())
			return;

		shouldSaveViewportImage = false;

		std::stringstream ss;
		ss << "image_" << Time::GetTime() << ".png";
		saveImage(ss.str().c_str(), RenderSize->x, RenderSize->y);
	}
public:
	std::function<bool()> customRenderFunc;
	Property<glm::vec2> RenderSize;

	Property<bool> shouldSaveViewportImage;
	Property<bool> shouldSaveAdvancedImage;

	IEvent<>& OnResize() {
		return onResize;
	}

	virtual bool Init() {
		Window::name = "renderWindow";
		createFrameBuffer();
		createTextureAttachment(RenderSize->x, RenderSize->y);
		createDepthBufferAttachment(RenderSize->x, RenderSize->y);
		unbindCurrentFrameBuffer(RenderSize->x, RenderSize->y);

		Settings::CustomRenderWindowAlpha().OnChanged() += [&](const float& v) { windowBackgroundColor.a = v; };
		windowBackgroundColor.a = Settings::CustomRenderWindowAlpha().Get();

		return true;
	}

	void HandleResize() {
		// handle custom render window resize
		glm::vec2 vMin = ImGui::GetWindowContentRegionMin();
		glm::vec2 vMax = ImGui::GetWindowContentRegionMax();
		glm::vec2 newSize = vMax - vMin;
		
		if (RenderSize.Get() != newSize) {
			ResizeCustomRenderCanvas(newSize);
			onResize.Invoke();
		}
	}

	virtual bool Design() {
		auto name = LocaleProvider::Get(Window::name) + "###" + Window::name;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, glm::vec2());
		
		// If the window is detached.
		ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBackgroundColor);
		// If the window is docked.
		ImGui::PushStyleColor(ImGuiCol_ChildBg, windowBackgroundColor);

		ImGui::Begin(name.c_str());

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		RenderToFileAdvanced();
		bindFrameBuffer(fbo, RenderSize->x, RenderSize->y);
		if (!customRenderFunc())
			return false;
		RenderToFileBasic();
		unbindCurrentFrameBuffer(RenderSize->x, RenderSize->y);

		ImGui::PushStyleColor(ImGuiCol_Button, glm::vec4());
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, glm::vec4());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, glm::vec4());
		
		ImGui::ImageButton((void*)(intptr_t)texture, RenderSize.Get(), glm::vec2(), glm::vec2(1), 0, glm::vec4(), glm::vec4(1), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

		ImGui::PopStyleColor(3);

		// true when mouse is pressed in rectangle of the item.
		Input::IsCustomRenderImageActive() = ImGui::IsItemActive();

		HandleResize();

		ImGui::End();

		return true;
	}

	virtual bool OnExit() {
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
		Window::name = "propertiesWindow";
		return true;
	}

	virtual bool Window::Design() {
		if (Object == nullptr) {
			auto name = LocaleProvider::Get(Window::name);
			ImGui::Begin(name.c_str());
			ImGui::End();
			return true;
		}

		auto name = LocaleProvider::Get(Window::name) + " " + GetName(Object);
		ImGui::Begin(name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		if (Object == nullptr) {
			return true;
		}

		auto name = LocaleProvider::Get(Window::name) + " " + GetName(Object);
		if (ImGui::BeginTabItem(name.c_str())) {
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
	const glm::vec4 selectedColor = glm::vec4(0, 0.2, 0.4, 1);
	const glm::vec4 selectedHoveredColor = glm::vec4(0, 0.4, 1, 1);
	const glm::vec4 selectedActiveColor = glm::vec4(0, 0, 0.8, 1);
	const glm::vec4 unselectedColor = glm::vec4(0, 0, 0, 0);

	bool hasMovementOccured = false;
	StaticField(int, GetID)
	
	MoveCommand* moveCommand;


	bool IsMovedToItself(const SceneObject* target, std::set<PON>& buffer) {
		for (auto o : buffer) {
			if (o.Get() == target)
				return true;
			else if (auto parent = target->GetParent();
				parent != nullptr && (parent == o.Get() || IsMovedToItself(parent, buffer)))
				return true;
		}

		return false;
	}

	void Select(SceneObject* t, bool isSelected = false, bool forceRecursive = false, bool selectChildren = false) {
		auto isCtrlPressed = input->IsPressed(Key::Modifier::Control);
		bool isRecursive = forceRecursive || input->IsPressed(Key::Modifier::Alt);

		std::function<void(SceneObject*)> func = isSelected && isCtrlPressed
			? ObjectSelection::Remove
			: ObjectSelection::Add;
		bool mustRemoveAllBeforeSelect = !isCtrlPressed;


		if (mustRemoveAllBeforeSelect)
			ObjectSelection::RemoveAll();

		if (selectChildren)
			for (auto c : t->children)
				func(c);
		else if (isRecursive)
			t->CallRecursive([func](SceneObject* o) { func(o); });
		else
			func(t);
	}

	bool TryDragDropTarget(SceneObject* o, int pos, int positionMask, ImGuiDragDropFlags flags = 0) {
		if (!ImGui::BeginDragDropTarget())
			return false;

		//flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
		//flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
		if (auto buffer = GetDragDropBuffer(flags)) {
			// We can't move object into itself
			if (IsMovedToItself(o, *buffer)) {
				ImGui::EndDragDropTarget();
				buffer->clear();
				return true;
			}

			InsertPosition relativePosition = GetPosition(positionMask);
			if (relativePosition == Center)
				ScheduleMove(o, 0, buffer, Center);
			else
				ScheduleMove(const_cast<SceneObject*>(o->GetParent()), pos, buffer, relativePosition);
		}
		ImGui::EndDragDropTarget();

		return true;
	}
	bool TryDragDropSource(SceneObject* o, bool isSelected, ImGuiDragDropFlags flags = 0) {
		if (!ImGui::BeginDragDropSource(flags))
			return false;

		if (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
			ImGui::Text("Moving \"%s\"", o->Name.c_str());

		if (!isSelected)
			Select(o);

		EmplaceDragDropSelected();

		ImGui::EndDragDropSource();

		return true;
	}
	bool TryContextWindow(SceneObject* t, bool isSelected, int flags) {
		static int clickedItemIDRightMouse = -1;

		if (Input::IsDown(Key::MouseRight))
			clickedItemIDRightMouse = ImGui::GetHoveredID();

		if (ImGui::GetItemID() == clickedItemIDRightMouse && ImGui::BeginPopupContextWindow()) {

			auto treeSelectionSupported = (!(flags & ImGuiTreeNodeFlags_Leaf) || (flags & ImGuiTreeNodeFlags_DefaultOpen));
			if (treeSelectionSupported && ImGui::Selectable("Select Tree")) {

				ImGui::EndPopup();

				TrySelect(t, isSelected, false, true);

				// Close popup
				clickedItemIDRightMouse = -1;

				return true;
			}
			if (treeSelectionSupported && ImGui::Selectable("Select Children")) {

				ImGui::EndPopup();

				TrySelect(t, isSelected, false, false, true);

				// Close popup
				clickedItemIDRightMouse = -1;

				return true;
			}
			ImGui::EndPopup();
		}

		return false;
	}
	bool TrySelect(SceneObject* t, bool isSelected, bool isFullySelectable = false, bool forceTreeSelection = false, bool forceChildrenSelection = false) {
		static SceneObject* clickedItemLeftMouse;

		if (hasMovementOccured)
			clickedItemLeftMouse = nullptr;
		else if (ImGui::IsItemClicked(Key::MouseLeft.code))
			clickedItemLeftMouse = t;

		if (forceTreeSelection) {
			Select(t, isSelected, true);
			return true;
		}
		else if (forceChildrenSelection) {
			Select(t, isSelected, false, true);
			return true;
		}


		if (!input->IsUp(Key::MouseLeft) || !ImGui::IsItemHovered() || clickedItemLeftMouse != t)
			return false;

		if (!isFullySelectable && GetSelectPosition() != Rest)
			return false;

		Select(t, isSelected);

		return true;
	}



	bool TreeNode(SceneObject* t, bool& isSelected, int& flags) {
		isSelected = exists(ObjectSelection::Selected(), t, std::function([](const PON& o) { return o.Get(); }));
		if (isSelected) {
			ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, selectedHoveredColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, selectedActiveColor);
		}

		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Framed;

		if (t->children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

		bool open = ImGui::TreeNodeEx(t->Name.c_str(), flags);

		if (isSelected) ImGui::PopStyleColor(3);

		return open;
	}

	DragDropBuffer::Buffer GetDragDropBuffer(ImGuiDragDropFlags target_flags) {
		return DragDropBuffer::GetDragDropPayload("SceneObjects", target_flags);
	}
	void EmplaceDragDropSelected() {
		DragDropBuffer::EmplaceDragDropSelected("SceneObjects");
	}

	bool DesignRootNode(GroupObject* t) {
		if (t == nullptr) {
			ImGui::Text("No scene loaded. Nothing to show");
			return true;
		}

		ImGui::PushID(GetID()++);

		ImGui::PushStyleColor(ImGuiCol_Header, unselectedColor);
		ImGuiDragDropFlags node_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
		bool isSelected;
		bool open = TreeNode(t, isSelected, node_flags);

		!TryDragDropTarget(t, 0, Center) && !TryDragDropSource(t, isSelected) && !TryContextWindow(t, isSelected, node_flags) && TrySelect(t, isSelected);

		for (int i = 0; i < t->children.size(); i++)
			if (!DesignTreeNode(t->children[i], t->children, i)) {
				ImGui::TreePop();
				ImGui::PopID();

				return false;
			}

		ImGui::TreePop();
		ImGui::PopStyleColor();
		ImGui::PopID();

		return true;
	}
	bool DesignTreeNode(SceneObject* t, std::vector<SceneObject*>& source, int pos) {
		ImGui::PushID(GetID()++);

		ImGui::Indent(indent);

		ImGuiDragDropFlags node_flags = 0;
		bool isSelected;
		bool open = TreeNode(t, isSelected, node_flags);

		ImGuiDragDropFlags dragdrop_flags = 0;
		dragdrop_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
		dragdrop_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
		//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip

		!TryDragDropTarget(t, pos, Any) && !TryDragDropSource(t, isSelected, dragdrop_flags) && !TryContextWindow(t, isSelected, node_flags) && TrySelect(t, isSelected);

		if (open) {
			for (int i = 0; i < t->children.size(); i++)
				if (!DesignTreeNode(t->children[i], t->children, i)) {
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
	SelectPosition GetSelectPosition() {
		glm::vec2 nodeScreenPos = ImGui::GetCursorScreenPos();
		glm::vec2 mouseScreenPos = ImGui::GetMousePos();

		float horPos = mouseScreenPos.x - nodeScreenPos.x;

		if (horPos < 16)
			return Anchor;

		return Rest;
	}

	void ScheduleMove(SceneObject* target, int targetPos, std::set<PON>* items, InsertPosition pos) {
		if (isCommandEmpty) {
			moveCommand = new MoveCommand();
			isCommandEmpty = false;
		}

		moveCommand->SetReady();
		moveCommand->target = target;
		moveCommand->targetPos = targetPos;
		moveCommand->items = items;
		moveCommand->pos = pos;
		moveCommand->caller = (IHolder*)this;
		moveCommand->callback = [&] {
			hasMovementOccured = true;
		};
	}

public:
	ReadonlyProperty<PON> rootObject;
	Input* input;
	float indent = 1;
	float centerSizeHalf = 3;

	// We divide height by this number. 
	// For some reason height/2 isn't center.
	const float magicNumber = 1.25;

	virtual bool Init() {
		Window::name = "objectInspectorWindow";
		return true;
	}
	virtual bool Design() {
		auto name = LocaleProvider::Get(Window::name) + "###" + Window::name;
		ImGui::Begin(name.c_str());

		// Reset elements' IDs.
		GetID() = 0;

		DesignRootNode((GroupObject*)rootObject.Get().Get());
		hasMovementOccured = false;

		ImGui::End();
		return true;
	}
	virtual bool OnExit() {
		return true;
	}
};

class PenToolWindow : Window, Attributes {
	const Log log = Log::For<PenToolWindow>();


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
		case PolyLineT:
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
		//ImGui::Text(GetName(type, GetTarget()).c_str());
		//if (ImGui::BeginDragDropTarget())
		//{
		//	ImGuiDragDropFlags target_flags = 0;
		//	//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
		//	//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
		//	std::vector<PON> objects;
		//	if (DragDropBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects))
		//	{
		//		if (objects.size() > 1) {
		//			log.Warning("Drawing instrument can't accept multiple scene objects");
		//		}
		//		else {
		//			if (!tool->BindSceneObjects(objects))
		//				return false;
		//		}
		//	}

		//	ImGui::EndDragDropTarget();
		//}

		{
			bool isActive = (GetTarget()) != nullptr;
			if (IsActive(isActive))
			{
				if (ImGui::Button(isActive ? "Release" : "No objects bind"))
				{
					tool->UnbindSceneObjects();
				}
				PopIsActive();
			}
		}


		{
			static int mode = 0;
			if (ImGui::RadioButton("ImmediateMode", &mode, 0))
				tool->SetMode(PolylinePenEditingToolMode::Immediate);
			if (ImGui::RadioButton("StepMode", &mode, 1))
				tool->SetMode(PolylinePenEditingToolMode::Step);
		}

		return true;
	}

public:
	// If this is null then the window probably wasn't initialized.
	//SceneObject* target = nullptr;

	PenTool* tool = nullptr;

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
		Window::name = Attributes::name = "pen";
		Attributes::isInitialized = true;

		return true;
	}
	virtual bool Window::Design() {
		auto name = LocaleProvider::Get("tool:" + Window::name) + "###" + Window::name + "Window";
		ImGui::Begin(name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		auto name = LocaleProvider::Get("tool:" + Attributes::name) + "###" + Attributes::name + "Window";
		if (ImGui::BeginTabItem(name.c_str()))
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

class CosinePenToolWindow : Window, Attributes {
	const Log log = Log::For<CosinePenToolWindow>();


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
		case SineCurveT:
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
		//ImGui::Text(GetName(type, GetTarget()).c_str());
		//if (ImGui::BeginDragDropTarget())
		//{
		//	ImGuiDragDropFlags target_flags = 0;
		//	//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
		//	//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
		//	std::vector<PON> objects;
		//	if (DragDropBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects))
		//	{
		//		if (objects.size() > 1) {
		//			log.Warning("Drawing instrument can't accept multiple scene objects");
		//		}
		//		else {
		//			if (!tool->BindSceneObjects(objects))
		//				return false;
		//		}
		//	}

		//	ImGui::EndDragDropTarget();
		//}

		{
			bool isActive = (GetTarget()) != nullptr;
			if (IsActive(isActive))
			{
				if (ImGui::Button(isActive ? "Release" : "No objects bind"))
				{
					tool->UnbindSceneObjects();
				}
				PopIsActive();
			}
		}


		{
			static int mode = 0;
			if (ImGui::RadioButton("Step123", &mode, 0))
				tool->SetMode(CosinePenEditingToolMode::Step123);
			if (ImGui::RadioButton("Step132", &mode, 1))
				tool->SetMode(CosinePenEditingToolMode::Step132);
		}

		ImGui::Separator();

		if (auto v = Settings::ShouldMoveCrossOnCosinePenModeChange().Get();
			ImGui::Checkbox("shouldMoveCrossOnCosinePenModeChange", &v))
			Settings::ShouldMoveCrossOnCosinePenModeChange() = v;

		return true;
	}

public:
	// If this is null then the window probably wasn't initialized.
	//SceneObject* target = nullptr;

	CosinePenTool* tool = nullptr;

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
		Window::name = Attributes::name = "cosinepen";
		Attributes::isInitialized = true;

		return true;
	}
	virtual bool Window::Design() {
		auto name = LocaleProvider::Get("tool:" + Window::name) + "###" + Window::name + "Window";
		ImGui::Begin(name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		auto name = LocaleProvider::Get("tool:" + Attributes::name) + "###" + Attributes::name + "Window";
		if (ImGui::BeginTabItem(name.c_str()))
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
class PointPenToolWindow : Window, Attributes {
	const Log log = Log::For<PointPenToolWindow>();


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
		case PointT:
			return "Point";
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
		//ImGui::Text(GetName(type, GetTarget()).c_str());
		//if (ImGui::BeginDragDropTarget())
		//{
		//	ImGuiDragDropFlags target_flags = 0;
		//	//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
		//	//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
		//	std::vector<PON> objects;
		//	if (DragDropBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects))
		//	{
		//		if (objects.size() > 1) {
		//			log.Warning("Drawing instrument can't accept multiple scene objects");
		//		}
		//		else {
		//			if (!tool->BindSceneObjects(objects))
		//				return false;
		//		}
		//	}

		//	ImGui::EndDragDropTarget();
		//}

		{
			bool isActive = (GetTarget()) != nullptr;
			if (IsActive(isActive))
			{
				if (ImGui::Button(isActive ? "Release" : "No objects bind"))
				{
					tool->UnbindSceneObjects();
				}
				PopIsActive();
			}
		}

		return true;
	}

public:
	// If this is null then the window probably wasn't initialized.
	//SceneObject* target = nullptr;

	PointPenTool* tool = nullptr;

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
		Window::name = Attributes::name = "pointpen";
		Attributes::isInitialized = true;

		return true;
	}
	virtual bool Window::Design() {
		auto name = LocaleProvider::Get("tool:" + Window::name) + "###" + Window::name + "Window";
		ImGui::Begin(name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		auto name = LocaleProvider::Get("tool:" + Attributes::name) + "###" + Attributes::name + "Window";
		if (ImGui::BeginTabItem(name.c_str()))
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
		case PolyLineT:
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
			std::vector<PON> objects;
			if (DragDropBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects)) {
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
		
		Window::name = Attributes::name = "extrusion";
		Attributes::isInitialized = true;
		
		return true;
	}
	virtual bool Window::Design() {
		auto name = LocaleProvider::Get("tool:" + Window::name) + "###" + Window::name + "Window";
		ImGui::Begin(name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		auto name = LocaleProvider::Get("tool:" + Attributes::name) + "###" + Attributes::name + "Window";
		if (ImGui::BeginTabItem(name.c_str()))
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
	bool isRelativeMode;

	glm::vec3 oldAngle = glm::vec3();

	std::string GetName(SceneObject* obj) {
		return
			(obj) != nullptr
			? (obj)->Name
			: "Empty";
	}

	int getPrecision(float v) {
		int precision = 0;
		for (int i = 0; i < maxPrecision; i++) {
			v *= 10;
			if ((int)v % 10 != 0)
				precision = i + 1;
		}
		return precision;
	}

	bool DragVector(glm::vec3& v, std::string s1, std::string s2, std::string s3, float speed) {
		bool r = false;
		std::stringstream ss;
		ss << "%." << getPrecision(v.x) << "f";
		r |= ImGui::DragFloat(s1.c_str(), &v.x, speed, 0, 0, ss.str().c_str());
		ss.str(std::string());
		ss << "%." << getPrecision(v.y) << "f";
		r |= ImGui::DragFloat(s2.c_str(), &v.y, speed, 0, 0, ss.str().c_str());
		ss.str(std::string());
		ss << "%." << getPrecision(v.z) << "f";
		r |= ImGui::DragFloat(s3.c_str(), &v.z, speed, 0, 0, ss.str().c_str());
		return r;
	}
	bool DragVector(glm::vec3& v, const char* s1, const char* s2, const char* s3, const char* f, float speed) {
		return ImGui::DragFloat(s1, &v.x, speed, 0, 0, f)
			| ImGui::DragFloat(s2, &v.y, speed, 0, 0, f)
			| ImGui::DragFloat(s3, &v.z, speed, 0, 0, f);
	}

	bool DesignInternal() {
		//ImGui::Text(GetName(GetTarget()).c_str());
		//if (ImGui::BeginDragDropTarget()) {
		//	ImGuiDragDropFlags target_flags = 0;
		//	//target_flags |= ImGuiDragDropFlags_AcceptBeforeDelivery;    // Don't wait until the delivery (release mouse button on a target) to do something
		//	//target_flags |= ImGuiDragDropFlags_AcceptNoDrawDefaultRect; // Don't display the yellow rectangle
		//	std::vector<PON> objects;
		//	if (DragDropBuffer::PopDragDropPayload("SceneObjects", target_flags, &objects))
		//	{
		//		if (!tool->BindSceneObjects(objects))
		//			return false;
		//	}
		//	ImGui::EndDragDropTarget();
		//}

		//if (ImGui::Extensions::PushActive(GetTarget() != nullptr)) {
		//	if (ImGui::Button("Release"))
		//		tool->UnbindSceneObjects();

		//	ImGui::Extensions::PopActive();
		//}

		auto transformToolModeCopy = tool->GetMode();
		{
			if (ImGui::RadioButton("Move", (int*)&transformToolModeCopy, (int)TransformToolMode::Translate))
				tool->SetMode(TransformToolMode::Translate);
			if (ImGui::RadioButton("Scale", (int*)&transformToolModeCopy, (int)TransformToolMode::Scale))
				tool->SetMode(TransformToolMode::Scale);
			if (ImGui::RadioButton("Rotate", (int*)&transformToolModeCopy, (int)TransformToolMode::Rotate))
				tool->SetMode(TransformToolMode::Rotate);
		}
		ImGui::Checkbox("Trace", &tool->shouldTrace);

		switch (transformToolModeCopy) {
		case TransformToolMode::Translate:
			ImGui::Separator();
			ImGui::Checkbox("Relative", &isRelativeMode);

			if (isRelativeMode)
				DragVector(tool->transformPos, "X", "Y", "Z", "%.5f", 1);
			else {
				auto crossPosCopy = tool->cross->GetLocalPosition();
				if (DragVector(crossPosCopy, "X", "Y", "Z", "%.5f", 1))
					tool->transformPos += crossPosCopy - tool->cross->GetLocalPosition();
			}
			break;
		case TransformToolMode::Scale:
			ImGui::Separator();
			ImGui::DragFloat("scale", &tool->scale, 0.01f, 0, 0, "%.2f");
			break;
		case TransformToolMode::Rotate:
			if (!Input::IsContinuousInputNoDelay())
				oldAngle = tool->angle;

			ImGui::Separator();
			if (auto angle = tool->angle - oldAngle;
				DragVector(angle, "X", "Y", "Z", 1))
				tool->angle = angle;

			break;
		default:
			break;
		}

		return true;
	}

public:
	TransformTool* tool = nullptr;

	virtual SceneObject* GetTarget() {
		if (tool == nullptr)
			return nullptr;

		return tool->GetTarget();
	}

	virtual bool Init() {
		if (tool == nullptr) {
			std::cout << "Tool wasn't assigned" << std::endl;
			return false;
		}

		Window::name = Attributes::name = "transformation";
		Attributes::isInitialized = true;

		return true;
	}
	virtual bool Window::Design() {
		auto name = LocaleProvider::Get("tool:" + Window::name) + "###" + Window::name + "Window";
		ImGui::Begin(name.c_str());

		if (!DesignInternal())
			return false;

		ImGui::End();

		return true;
	}
	virtual bool Attributes::Design() {
		auto name = LocaleProvider::Get("tool:" + Attributes::name) + "###" + Attributes::name + "Window";
		if (ImGui::BeginTabItem(name.c_str()))
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
	virtual void UnbindTargets() {}
};


class AttributesWindow : public Window {
	Attributes* toolAttributes = nullptr;
	Attributes* targetAttributes = nullptr;
	
public:
	std::function<void()> onUnbindTool = [] {};

	virtual bool Init() {
		Window::name = "attributesWindow";
		return true;
	}
	virtual bool Design() {
		auto name = LocaleProvider::Get(Window::name) + "###" + Window::name;
		ImGui::Begin(name.c_str());

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
		if (!toolAttributes)
			return true;

		onUnbindTool();
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

	
	CreatingTool<PolyLine> polyLineTool;
	CreatingTool<GroupObject> groupObjectTool;
	CreatingTool<SineCurve> sineCurveTool;
	CreatingTool<PointObject> pointTool;



	template<typename T>
	void ConfigureCreationTool(CreatingTool<T>& creatingTool, std::function<void(SceneObject*)> initFunc) {
		creatingTool.destination <<= Scene::root();
		creatingTool.init = initFunc;
	}

public:
	StaticProperty(::AttributesWindow*, AttributesWindow)
	StaticProperty(std::function<void()>, ApplyDefaultTool)

	template<typename T>
	using unbindTool = decltype(std::declval<T>().UnbindTool());

	template<typename T>
	static constexpr bool hasUnbindTool = is_detected_v<unbindTool, T>;

	template<typename TWindow, typename TTool, std::enable_if_t<hasUnbindTool<TTool>>* = nullptr>
	static void ApplyTool() {
		AttributesWindow()->UnbindTarget();
		AttributesWindow()->UnbindTool();

		auto targetWindow = new SceneObjectPropertiesWindow();
		auto tool = new TWindow();
		tool->tool = ToolPool::GetTool<TTool>();
		tool->tool->Activate();
		AttributesWindow()->BindTool((Attributes*)tool);
		AttributesWindow()->BindTarget((Attributes*)targetWindow);

		auto deleteAllhandlerId = Scene::OnDeleteAll() += [t = tool] {
			t->UnbindTargets();
			t->tool->UnbindTool();
		};
		AttributesWindow()->onUnbindTool = [t = tool, d = deleteAllhandlerId] {
			t->tool->UnbindTool();
			Scene::OnDeleteAll().RemoveHandler(d);
			t->OnExit();
			delete t;
		};
	}

	virtual bool Init() {
		if (!AttributesWindow().IsAssigned())
		{
			log.Error("AttributesWindow was null");
			return false;
		}

		ConfigureCreationTool(polyLineTool, [](SceneObject* o) {
			Scene::AssignUniqueName(o, LocaleProvider::Get("object:polyline"));
		});
		ConfigureCreationTool(sineCurveTool, [](SceneObject* o) {
			Scene::AssignUniqueName(o, LocaleProvider::Get("object:sinecurve"));
			});
		ConfigureCreationTool(groupObjectTool, [](SceneObject* o) {
			Scene::AssignUniqueName(o, LocaleProvider::Get("object:group"));
		});
		ConfigureCreationTool(pointTool, [](SceneObject* o) {
			Scene::AssignUniqueName(o, LocaleProvider::Get("object:point"));
			});

		Window::name = "toolWindow";

		return true;
	}
	virtual bool Design() {
		auto windowName = LocaleProvider::Get(Window::name) + "###" + Window::name;
		ImGui::Begin(windowName.c_str());
		{
			if (ImGui::Button(LocaleProvider::GetC("object:group")))
				groupObjectTool.Create();
		}
		{
			ImGui::Separator();
			if (ImGui::Button(LocaleProvider::GetC("tool:extrusion")))
				ApplyTool<ExtrusionToolWindow<PolyLineT>, ExtrusionEditingTool<PolyLineT>>();
			if (ImGui::Button(LocaleProvider::GetC("tool:pen")))
				ApplyTool<PenToolWindow, PenTool>();
			if (ImGui::Button(LocaleProvider::GetC("tool:cosinepen")))
				ApplyTool<CosinePenToolWindow, CosinePenTool>();
			if (ImGui::Button(LocaleProvider::GetC("tool:pointpen")))
				ApplyTool<PointPenToolWindow, PointPenTool>();
			if (ImGui::Button(LocaleProvider::GetC("tool:transformation")))
				ApplyTool<TransformToolWindow, TransformTool>();
		}
		{
			ImGui::Separator();
			auto v = (int)Settings::SpaceMode().Get();
			if (ImGui::Extensions::RadioButton("spaceMode:world", &v, (int)SpaceMode::World, "spaceMode:worldTip"))
				Settings::SpaceMode() = SpaceMode::World;
			if (ImGui::Extensions::RadioButton("spaceMode:local", &v, (int)SpaceMode::Local, "spaceMode:localTip"))
				Settings::SpaceMode() = SpaceMode::Local;
		}
		{
			ImGui::Separator();
			if (Settings::ShouldRestrictTargetModeToPivot().Get()) {
				auto v = (int)TargetMode::Pivot;
				ImGui::Extensions::PushActive(false);
				if (ImGui::Extensions::RadioButton("targetMode:object", &v, (int)TargetMode::Object, "targetMode:objectTip"))
					Settings::TargetMode() = TargetMode::Object;
				if (ImGui::Extensions::RadioButton("targetMode:cross", &v, (int)TargetMode::Pivot, "targetMode:crossTip"))
					Settings::TargetMode() = TargetMode::Pivot;
				ImGui::Extensions::PopActive();
			}
			else {
				auto v = (int)Settings::TargetMode().Get();
				if (ImGui::Extensions::RadioButton("targetMode:object", &v, (int)TargetMode::Object, "targetMode:objectTip"))
					Settings::TargetMode() = TargetMode::Object;
				if (ImGui::Extensions::RadioButton("targetMode:cross", &v, (int)TargetMode::Pivot, "targetMode:crossTip"))
					Settings::TargetMode() = TargetMode::Pivot;
			}
		}
		{
			ImGui::Separator();
			if (bool v = Settings::UseDiscreteMovement().Get();
				ImGui::Checkbox(LocaleProvider::GetC(Settings::Name(&Settings::UseDiscreteMovement)), &v))
				Settings::UseDiscreteMovement() = v;
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

	const Log log = Log::For<FileWindow>();

	Path path;
	Path selectedFile;
	Scene* scene;

	//bool iequals(const std::string& a, const std::string& b)
	//{
	//	return std::equal(a.begin(), a.end(),
	//		b.begin(), b.end(),
	//		[](char a, char b) {
	//			return tolower(a) == tolower(b);
	//		});
	//}

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
		ImGui::InputText(LocaleProvider::GetC("path"), &path.getBuffer());

		if (ImGui::Extensions::PushActive(path.isSome())) {
			if (ImGui::Button(LocaleProvider::GetC("submit")))
				path.apply();

			ImGui::Extensions::PopActive();
		}
	}
	void CloseButton() {
		if (ImGui::Button(LocaleProvider::GetC("cancel"))) {
			shouldClose = true;
		}
	}

public:
	Mode mode;

	virtual bool Init() {
		if (!scene) {
			log.Error("Scene was null");
			return false;
		}

		path.apply("./scenes");

		// Create directory if not exists
		fs::create_directory(path.get());

		return true;
	}
	virtual bool Design() {
		auto windowName = mode == FileWindow::Load ? "openFileWindow" : "saveFileWindow";
		auto name = LocaleProvider::Get(windowName) + "###" + "fileManagerWindow";
		ImGui::Begin(name.c_str());

		ShowPath();
		ListFiles();

		ImGui::InputText(LocaleProvider::GetC("file"), &selectedFile.getBuffer());

		if (ImGui::Extensions::PushActive(selectedFile.isSome())) {
			if (ImGui::Button(mode == FileWindow::Load ? LocaleProvider::GetC("open") : LocaleProvider::GetC("save"))) {
				try {
					auto fileName = selectedFile.get().is_absolute() 
						? selectedFile.getBuffer() 
						: path.join(selectedFile);

					auto action = mode == FileWindow::Load 
						? FileManager::Load 
						: FileManager::Save;

					if (mode == FileWindow::Load)
						scene->DeleteAll();

					action(fileName, scene);

					if (mode == FileWindow::Load)
						Changes::Commit();

					shouldClose = true;
				}
				catch (const FileException & e) {
					// TODO: Show error message to user
				}
			}

			ImGui::Extensions::PopActive();
		}

		CloseButton();

		ImGui::End();

		return true;
	}
	bool BindScene(Scene* scene) {
		if (this->scene = scene)
			return true;

		log.Error("Scene was null");
		return false;
	}
};

class SettingsWindow : Window {
	const Log log = Log::For<SettingsWindow>();

	static const char* GetC(const char* path, void* settingReference) {
		return LocaleProvider::GetC(path + Settings::Name(settingReference));
	}

	template<typename T>
	static void SettingField(Property<T>& (*settingReference)(), std::function<bool(const char*, T&)> field, bool shouldForceUpdateCache = false) {
		if (auto v = (&settingReference())->Get();
			field(LocaleProvider::GetC(Settings::Name(settingReference)), v)) {
			*(&settingReference()) = v;
			if (shouldForceUpdateCache)
				Scene::root().Get()->ForceUpdateCache();
		}
	}

	template<typename T>
	static void SettingField(const char* prefix, Property<T>& (*settingReference)(), std::function<bool(const char*, T&)> field, bool shouldForceUpdateCache = false) {
		if (auto v = (&settingReference())->Get();
			field(LocaleProvider::GetC(prefix + Settings::Name(settingReference)), v)) {
			*(&settingReference()) = v;
			if (shouldForceUpdateCache)
				Scene::root().Get()->ForceUpdateCache();
		}
	}
public:

	Property<bool> IsOpen;


	virtual bool Init() {
		Window::name = "settingsWindow";

		return true;
	}
	virtual bool Design() {
		if (!IsOpen.Get())
			return true;

		auto windowName = LocaleProvider::Get(Window::name) + "###" + Window::name;
		if (!ImGui::Begin(windowName.c_str(), &IsOpen.Get())) {
			ImGui::End();
			return true;
		}

		SettingField(&Settings::StateBufferLength, std::function([](const char* name, int& v)
			{ return ImGui::InputInt(name, &v, 1, 10, 4); }));

		if (auto v = Settings::Language().Get();
			ImGui::TreeNode((LocaleProvider::Get(Settings::Name(&Settings::Language)) + ": " + LocaleProvider::Get(v)).c_str())) {

			if (auto i = v == Locale::EN; ImGui::Selectable(LocaleProvider::GetC(Locale::EN), &i))
				Settings::Language() = Locale::EN;
			if (auto i = v == Locale::UA; ImGui::Selectable(LocaleProvider::GetC(Locale::UA), &i))
				Settings::Language() = Locale::UA;

			ImGui::TreePop();
		}

		if (ImGui::TreeNode(LocaleProvider::GetC("step:step"))) {

			SettingField("step:", &Settings::TranslationStep, std::function([](const char* name, float& v) 
				{ return ImGui::InputFloat(name, &v, 1, 10); }));

			SettingField("step:", &Settings::RotationStep, std::function([](const char* name, float& v) 
				{ return ImGui::InputFloat(name, &v, 1, 10); }));

			SettingField("step:", &Settings::ScalingStep, std::function([](const char* name, float& v) 
				{ return ImGui::InputFloat(name, &v, 0.01, 0.1); }));

			SettingField("step:", &Settings::MouseSensivity, std::function([](const char* name, float& v) 
				{ return ImGui::InputFloat(name, &v, 0.01, 0.1); }));
			
			ImGui::TreePop();
		}

		if (ImGui::TreeNode(LocaleProvider::GetC("color:color"))) {

			std::function colorField = [](const char* name, glm::vec4& v)
				{ return ImGui::ColorEdit4(name, (float*)&v, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview); };

			SettingField("color:", &Settings::ColorLeft, colorField);
			SettingField("color:", &Settings::ColorRight, colorField);
			SettingField("color:", &Settings::DimmedColorLeft, colorField);
			SettingField("color:", &Settings::DimmedColorRight, colorField);

			ImGui::TreePop();
		}

		if (ImGui::TreeNode(LocaleProvider::GetC("webcam:webcam"))) {

			std::function colorField = [](const char* name, glm::vec4& v)
			{ return ImGui::ColorEdit4(name, (float*)&v, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview); };

			SettingField("webcam:", &Settings::CameraResolution, std::function([](const char* name, glm::vec2& v)
				{ return ImGui::InputFloat2(name, (float*)&v); }));

			SettingField("webcam:", &Settings::CameraViewAngles, std::function([](const char* name, glm::vec2& v)
				{ return ImGui::InputFloat2(name, (float*)&v); }));

			SettingField("webcam:", &Settings::CameraAngle, std::function([](const char* name, glm::vec2& v)
				{ return ImGui::InputFloat2(name, (float*)&v); }));

			SettingField("webcam:", &Settings::FaceSizeYMillimeters, std::function([](const char* name, float& v)
				{ return ImGui::InputFloat(name, &v); }));

			SettingField("webcam:", &Settings::ScreenCenterToCameraDistanceMillimeters, std::function([](const char* name, glm::vec3& v)
				{ return ImGui::InputFloat(name, (float*)&v); }));

			ImGui::TreePop();
		}

	
		SettingField(&Settings::LogFileName, std::function([](const char* name, std::string& v) 
			{ return ImGui::InputText(name, &v); }));

		SettingField(&Settings::PPI, std::function([](const char* name, float& v) 
			{ return ImGui::InputFloat(name, &v); }));

		SettingField(&Settings::CustomRenderWindowAlpha, std::function([](const char* name, float& v) 
			{ return ImGui::DragFloat(name, &v, 0.01, 0, 1); }));

		SettingField(&Settings::PointRadiusPixel, std::function([](const char* name, int& v) 
			{ return ImGui::DragInt(name, &v, 1, 1, 100); }));

		SettingField(&Settings::LineThickness, std::function([](const char* name, int& v)
			{ return ImGui::DragInt(name, &v, 1, Settings::MinLineThickness(), Settings::MaxLineThickness()); }));


		SettingField(&Settings::CosinePointCount, std::function([](const char* name, int& v) 
			{ 
				auto res = ImGui::InputInt(name, &v);
				{
					const char* explanation = " (k)\nd=x^2/(3+k), 0 <= k <= 1e6";
					ImGui::SameLine(); 
					ImGui::Extensions::HelpMarker((LocaleProvider::Get("cosinePointCountToolTip") + explanation).c_str());
				}
				if (v < 0) v = 0;
				else if (v > 1e6) v = 1e6;
				return res; 
			}),
			true);

		//ImGui::SameLine(); ImGui::Extensions::HelpMarker("Requires restart.\n");

		ImGui::End();
		return true;
	}
	virtual bool OnExit() {
		return true;
	}

};

class LogWindow : Window {
	const Log log = Log::For<LogWindow>();
public:
	std::string Logs;

	virtual bool Init() {
		Window::name = "logWindow";

		return true;
	}
	virtual bool Design() {
		//ImGui::Extensions::LogTextMultiline(&Logs);
		ImGui::InputTextMultiline("", &Logs, glm::vec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
		return true;
	}
	virtual bool OnExit() {
		return true;
	}
};
