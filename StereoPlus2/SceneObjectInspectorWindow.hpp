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

class SceneObjectInspectorWindow : Window, MoveCommand::IHolder {
	const Log log = Log::For<SceneObjectInspectorWindow>();
	const glm::vec4 selectedColor = glm::vec4(0, 0.2, 0.4, 1);
	const glm::vec4 selectedHoveredColor = glm::vec4(0, 0.4, 1, 1);
	const glm::vec4 selectedActiveColor = glm::vec4(0, 0, 0.8, 1);
	const glm::vec4 unselectedColor = glm::vec4(0, 0, 0, 0);

	bool hasMovementOccured;

	size_t shiftDownEventHandler;
	bool isShiftPressed;

	MoveCommand* moveCommand;

	static int& GetID() {
		static int val = 0;
		return val;
	}

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

	void Select(SceneObject* t, bool isSelected = false, bool ignoreCtrl = false) {
		auto isCtrlPressed = ignoreCtrl
			? false
			: input->IsPressed(Key::Modifier::Control);
		auto isShiftPressed = input->IsPressed(Key::Modifier::Shift);

		std::function<void(SceneObject*)> func = isSelected && isCtrlPressed
			? ObjectSelection::Remove
			: ObjectSelection::Add;
		bool isRecursive = isShiftPressed;
		bool mustRemoveAllBeforeSelect = !isCtrlPressed;


		if (mustRemoveAllBeforeSelect)
			ObjectSelection::RemoveAll();

		if (isRecursive)
			t->CallRecursive([func](SceneObject* o) { func(o); });
		else
			func(t);
	}

	bool TrySelect(SceneObject* t, bool isSelected, bool isFullySelectable = false) {
		static SceneObject* clickedItem;

		if (hasMovementOccured)
			clickedItem = nullptr;
		else if (ImGui::IsItemClicked())
			clickedItem = t;

		if (!input->IsUp(Key::MouseLeft) || !ImGui::IsItemHovered() || clickedItem != t)
			return false;

		if (!isFullySelectable && GetSelectPosition() != Rest)
			return false;

		Select(t, isSelected);

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

	bool TreeNode(SceneObject* t, bool& isSelected, int flags = 0) {
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
		ImGuiDragDropFlags target_flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
		bool isSelected;
		bool open = TreeNode(t, isSelected, target_flags);

		!TryDragDropTarget(t, 0, Center) && !TryDragDropSource(t, isSelected) && TrySelect(t, isSelected);

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

		bool isSelected;
		bool open = TreeNode(t, isSelected);

		ImGuiDragDropFlags src_flags = 0;
		src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;     // Keep the source displayed as hovered
		src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers; // Because our dragging is local, we disable the feature of opening foreign treenodes/tabs while dragging
		//src_flags |= ImGuiDragDropFlags_SourceNoPreviewTooltip; // Hide the tooltip

		!TryDragDropTarget(t, pos, Any) && !TryDragDropSource(t, isSelected, src_flags) && TrySelect(t, isSelected);

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

		input->AddShortcut(Key::Combination(Key::ShiftLeft), [] {});

		shiftDownEventHandler = input->AddHandler([&] (Input* i){
			isShiftPressed = i->IsPressed(Key::Modifier::Shift);
			});
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
		input->RemoveHandler(shiftDownEventHandler);
		return true;
	}
};
