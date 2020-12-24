#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "DomainUtils.hpp"
#include <map>
#include <vector>
#include "Key.hpp"




// First frame keys are never Up or Down
// as we need to check previous state in order to infer those.
class Input {
	struct KeyPairComparator {
		bool operator() (const Key::KeyPair& lhs, const Key::KeyPair& rhs) const {
			return lhs.code < rhs.code || lhs.code == rhs.code && lhs.type < rhs.type;
		}
	};
	struct CombinationNode {
		std::map<Key::Modifier, CombinationNode> children;
		
		// End keys and callbacks
		std::map<Key::KeyPair, std::function<void()>, KeyPairComparator> callbacks;
	};


	// Continuous input is a state when there is
	// input with delay in between.
	struct ContinuousInput {
		bool isContinuousInput = false;
		// seconds
		float continuousInputAwaitTime;
		size_t lastPressedTime;

		bool isAnythingPressedLast = false;

		ContinuousInput(float continuousInputAwaitTime) : continuousInputAwaitTime(continuousInputAwaitTime) {}

		void Process(bool isAnythingPressed) {
			if (isAnythingPressed) {
				isContinuousInput = true;
				lastPressedTime = Time::GetTime();
			}
			else if (isContinuousInput && (Time::GetTime() - lastPressedTime) > continuousInputAwaitTime * 1e3)
				isContinuousInput = false;
		}

		void UpdateOld(bool isAnythingPressed) {
			isAnythingPressedLast = isAnythingPressed;
		}
	};

	const Log Logger = Log::For<Input>();

	glm::vec2 mouseOldPos;
	glm::vec2 mouseNewPos;

	bool isMouseBoundlessMode = false;
	bool isRawMouseMotionSupported = false;

	float mouseSensivity = 1e-2;
	float mouseMaxMagnitude = 1e4;

	ContinuousInput continuousInputOneSecondDelay = ContinuousInput(1);
	ContinuousInput continuousInputNoDelay = ContinuousInput(0);

	CombinationNode combinations;

	void FillAxes() {
		mouseOldPos = mouseNewPos;
		mouseNewPos = ImGui::GetMousePos();
		
		auto useDiscreteMovement = Settings::UseDiscreteMovement().Get();
		ArrowAxe = glm::vec3(
			-IsPressed(Key::Left, useDiscreteMovement) + IsPressed(Key::Right, useDiscreteMovement),
			IsPressed(Key::Up, useDiscreteMovement) + -IsPressed(Key::Down, useDiscreteMovement),
			0);
		NumpadAxe = IsPressed(Key::Modifier::Shift)
			? glm::vec3()
			: glm::vec3(
				-IsPressed(Key::N4, useDiscreteMovement) + IsPressed(Key::N6, useDiscreteMovement),
				-IsPressed(Key::N2, useDiscreteMovement) + IsPressed(Key::N8, useDiscreteMovement),
				-IsPressed(Key::N9, useDiscreteMovement) + IsPressed(Key::N1, useDiscreteMovement));

		if (IsCustomRenderImageActive().Get()) {
			auto mouseMoveDirection = MouseMoveDirection();
			MouseAxe = IsPressed(Key::MouseRight)
				? glm::vec3(0, 0, -mouseMoveDirection.y)
				: glm::vec3(mouseMoveDirection.x, -mouseMoveDirection.y, 0);

		}
		else MouseAxe = glm::vec3();
	}


	static bool InsertCombination(
		CombinationNode& node,
		const std::vector<Key::Modifier>::const_iterator& modifiersCurrent,
		const std::vector<Key::Modifier>::const_iterator& modifiersEnd,
		const Key::KeyPair& endKey,
		const std::function<void()>& callback) {
		if (modifiersCurrent == modifiersEnd) {
			if (node.children.find(*modifiersCurrent._Ptr) != node.children.end())
				return false;

			node.callbacks[endKey] = callback;
			return true;
		}

		if (auto c = node.children.find(*modifiersCurrent._Ptr);
			c != node.children.end())
			return InsertCombination(c._Ptr->_Myval.second, modifiersCurrent + 1, modifiersEnd, endKey, callback);

		node.children[*modifiersCurrent._Ptr] = CombinationNode();
		return InsertCombination(node.children[*modifiersCurrent._Ptr], modifiersCurrent + 1, modifiersEnd, endKey, callback);
	}

	// Prevents combinations with letters or printable symbols
	// being executed while the keyboard is captured by text input.
	// Applied to Shift-modified and non-modified keys.
	static bool IsAcceptableCombination(const int key, const bool ignoreCapturedKeys, const int level, const Key::Modifier& modifier) {
		if (!ignoreCapturedKeys || key < GLFW_KEY_SPACE || key > GLFW_KEY_WORLD_2 && key < GLFW_KEY_KP_0 || key > GLFW_KEY_KP_EQUAL)
			return true;

		if (level == 0 || level == 1 && modifier == Key::Modifier::Shift)
			return false;
		
		return true;
	}

	// Executes most complex combination if found.
	static bool ExecuteFirstMatchingCombination(const CombinationNode& node, const bool ignoreCapturedKeys, const int level = 0, const Key::Modifier modifier = Key::Modifier::None) {
		for (auto& [m, c] : node.children)
			if (IsPressed(m) && ExecuteFirstMatchingCombination(c, false, level + 1, m))
				return true;

		for (auto& [k, c] : node.callbacks)
			if (IsAcceptableCombination(k.code, ignoreCapturedKeys, level, modifier) && IsPressed(k, true)) {
				c();
				return true;
			}

		return false;
	}

public:
	StaticProperty(GLFWwindow*, GLFWindow);
	ImGuiIO* io;

	glm::vec3 ArrowAxe;
	glm::vec3 NumpadAxe;
	glm::vec3 MouseAxe;

	glm::vec3 movement;
	std::map<size_t, std::function<void()>> handlers;

	StaticProperty(bool, IsCustomRenderImageActive)

	bool IsMoved() {
		return movement != glm::vec3();
	}

	static bool IsPressed(Key::KeyPair key, bool discreteInput = false) {
		auto* io = &ImGui::GetIO();

		if (discreteInput)
			return key.type == Key::Type::Mouse
				? ImGui::IsMouseClicked(key.code, true)
				: ImGui::IsKeyPressed(key.code, true);

		return key.type == Key::Type::Mouse
			? io->MouseDown[key.code]
			: io->KeysDown[key.code];
	}
	static bool IsPressed(Key::Modifier key) {
		auto* io = &ImGui::GetIO();

		switch (key)
		{
		case Key::Modifier::Control: return io->KeyCtrl;
		case Key::Modifier::Shift: return io->KeyShift;
		case Key::Modifier::Alt: return io->KeyAlt;
		}
	}

	static bool IsDown(const Key::KeyPair& key) {
		return key.type == Key::Type::Mouse
			? ImGui::IsMouseClicked(key.code, true)
			: ImGui::IsKeyPressed(key.code, true);
	}

	bool IsUp(Key::KeyPair key) {
		return key.type == Key::Type::Mouse
			? ImGui::IsMouseReleased(key.code)
			: ImGui::IsKeyReleased(key.code);
	}

	glm::vec2 MousePosition() {
		return mouseNewPos;
	}
	glm::vec2 MouseMoveDirection() {
		return glm::length(mouseNewPos - mouseOldPos) == 0 ? glm::vec2(0) : mouseNewPos - mouseOldPos;
	}

	// Does not include mouse movement nor scrolling.
	bool IsContinuousInputOneSecondDelay() {
		return continuousInputOneSecondDelay.isContinuousInput;
	}
	bool IsContinuousInputNoDelay() {
		return continuousInputNoDelay.isContinuousInput;
	}

	bool MouseMoved() {
		return MouseSpeed() > mouseSensivity && MouseSpeed() < mouseMaxMagnitude;
	}
	float MouseSpeed() {
		return glm::length(mouseNewPos - mouseOldPos);
	}
	static void SetMouseBoundlessMode(bool enable) {
		if (enable) {
			if (glfwRawMouseMotionSupported())
				glfwSetInputMode(GLFWindow().Get(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetInputMode(GLFWindow().Get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else {
			// We can disable raw mouse motion mode even if it's not supported
			// so we don't bother with checking it.
			glfwSetInputMode(GLFWindow().Get(), GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

			glfwSetInputMode(GLFWindow().Get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	void AddShortcut(const Key::Combination& combination, const std::function<void()>& callback) {
		if (InsertCombination(combinations, combination.modifiers.begin(), combination.modifiers.end(), combination.key, callback))
			return;
		Logger.Error("Shortcut is already taken.");
	}


	void ProcessInput() {
		movement = glm::vec3();
		FillAxes();

		continuousInputOneSecondDelay.Process(io->AnyKeyPressed);
		continuousInputNoDelay.Process(io->AnyKeyPressed);

		// Make sure printable characters don't trigger combinations
		// while keyboard is captured by text input
		if (io->AnyKeyPressed)
			ExecuteFirstMatchingCombination(combinations, io->WantCaptureKeyboard);

		// Handle OnInput actions
		for (auto [id,handler] : handlers)
			handler();

		continuousInputOneSecondDelay.UpdateOld(io->AnyKeyPressed);
		continuousInputNoDelay.UpdateOld(io->AnyKeyPressed);
	}

	bool Init() {
		isRawMouseMotionSupported = glfwRawMouseMotionSupported();

		return true;
	}
};

class KeyBinding {
public:
	Input* input;
	Cross* cross;

	float crossScaleSpeed = 1;
	float crossMinSize = 0.1;
	float crossMaxSize = 100;


	size_t AddHandler(std::function<void()> func) {
		static size_t id = 0;

		(new FuncCommand())->func = [id = id, input = input, func = func] {
			input->handlers[id] = func;
		};

		return id++;
	}
	size_t AddHandler(std::function<void(Input*)> func) {
		return AddHandler([i = input, func] { func(i); });
	}
	void RemoveHandler(size_t id) {
		auto cmd = new FuncCommand();
		cmd->func = [id = id, input = input] {
			input->handlers.erase(id);
		};
	}

	void ResetFocus() {
		AddHandler([i = input] {
			if (i->IsDown(Key::Escape))
				ImGui::FocusWindow(NULL);
			});
	}
	void BindInputToMovement() {
		AddHandler([i = input] {
			// Enable or disable Mouse boundless mode 
			bool mouseBoundlessMode = i->IsPressed(Key::Modifier::Alt) || i->IsPressed(Key::Modifier::Shift) || i->IsPressed(Key::Modifier::Control);
			Input::SetMouseBoundlessMode(mouseBoundlessMode && Input::IsCustomRenderImageActive().Get());

			i->movement += i->MouseAxe * Settings::MouseSensivity().Get();
			i->movement += i->ArrowAxe;
			i->movement += i->NumpadAxe;
			});
	}
	void Cross() {
		// Resize cross.
		AddHandler([i = input, c = cross, &sp = crossScaleSpeed, &min = crossMinSize, &max = crossMaxSize] {
			bool isScaleUp = i->IsPressed(Key::N3, true);
			bool isScaleDown = i->IsPressed(Key::N7, true);

			if (isScaleUp == isScaleDown)
				return;

			if (isScaleUp) {
				//float newSize = c->size *= 1 + sp;
				float newSize = c->size += sp;
				if (max < newSize)
					c->size = max;
				else
					c->size = newSize;
			}
			else {
				//float newSize = c->size *= 1 - sp;
				float newSize = c->size -= sp;
				if (newSize < min)
					c->size = min;
				else
					c->size = newSize;
			}

			c->ForceUpdateCache();
			});
	}
	void ChangeBuffer() {
		AddHandler([i = input] {
			if (i->IsPressed(Key::Modifier::Control)) {
				if (i->IsDown(Key::Z))
					StateBuffer::Rollback();
				else if (i->IsDown(Key::Y))
					StateBuffer::Repeat();
			}
			});
	}
	bool Init() {
		ResetFocus();
		BindInputToMovement();
		Cross();
		ChangeBuffer();

		return true;
	}
};

