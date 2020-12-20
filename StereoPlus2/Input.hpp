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

	glm::vec2 mouseOldPos;
	glm::vec2 mouseNewPos;

	bool isMouseBoundlessMode = false;
	bool isRawMouseMotionSupported = false;

	float mouseSensivity = 1e-2;
	float mouseMaxMagnitude = 1e4;

	ContinuousInput continuousInputOneSecondDelay = ContinuousInput(1);
	ContinuousInput continuousInputNoDelay = ContinuousInput(0);

	void FillAxes() {
		mouseOldPos = mouseNewPos;
		mouseNewPos = ImGui::GetMousePos();

		auto useDiscreteMovement = Settings::UseDiscreteMovement().Get();
		ArrowAxe = glm::vec3(
			-IsPressed(Key::Left, useDiscreteMovement) + IsPressed(Key::Right, useDiscreteMovement),
			IsPressed(Key::Up, useDiscreteMovement) + -IsPressed(Key::Down, useDiscreteMovement),
			0);
		NumpadAxe = glm::vec3(
			-IsPressed(Key::N4, useDiscreteMovement) + IsPressed(Key::N6, useDiscreteMovement),
			-IsPressed(Key::N2, useDiscreteMovement) + IsPressed(Key::N8, useDiscreteMovement),
			-IsPressed(Key::N1, useDiscreteMovement) + IsPressed(Key::N9, useDiscreteMovement));

		auto mouseMoveDirection = MouseMoveDirection();
		MouseAxe = IsPressed(Key::MouseRight)
			? glm::vec3(0, 0, -mouseMoveDirection.y)
			: glm::vec3(mouseMoveDirection.x, -mouseMoveDirection.y, 0);
	}

public:
	GLFWwindow* glWindow;
	ImGuiIO* io;

	glm::vec3 ArrowAxe;
	glm::vec3 NumpadAxe;
	glm::vec3 MouseAxe;


	glm::vec3 movement;
	std::map<size_t, std::function<void()>> handlers;

	bool IsMoved() {
		return movement != glm::vec3();
	}

	bool IsPressed(Key::KeyPair key, bool discreteInput = false) {
		if (discreteInput)
			return key.type == Key::Type::Mouse
				? ImGui::IsMouseClicked(key.code, true)
				: ImGui::IsKeyPressed(key.code, true);

		return key.type == Key::Type::Mouse
			? io->MouseDown[key.code]
			: io->KeysDown[key.code];
	}

	bool IsDown(const Key::KeyPair& key) {
		return key.type == Key::Type::Mouse
			? ImGui::IsMouseClicked(key.code, true)
			: ImGui::IsKeyPressed(key.code, true);
	}
	bool IsDown(const Key::Combination& keys) {
		int i = 0;
		for (; i < keys.keys.size() - 1; i++)
			if (!IsPressed(keys.keys[i]))
				return false;
		
		if (!IsDown(keys.keys[i]))
			return false;

		return true;
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
		return glm::length(mouseNewPos - mouseOldPos) == 0 ? glm::vec2(0) : glm::normalize(mouseNewPos - mouseOldPos);
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
	void SetMouseBoundlessMode(bool enable) {
		if (enable == isMouseBoundlessMode)
			return;

		if (enable) {
			if (isRawMouseMotionSupported)
				glfwSetInputMode(glWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else {
			// We can disable raw mouse motion mode even if it's not supported
			// so we don't bother with checking it.
			glfwSetInputMode(glWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

			glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		isMouseBoundlessMode = enable;
	}

	void ProcessInput() {
		movement = glm::vec3();
		FillAxes();

		continuousInputOneSecondDelay.Process(io->AnyKeyPressed);
		continuousInputNoDelay.Process(io->AnyKeyPressed);

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

	float crossScaleSpeed = 0.01;
	float crossMinSize = 0.001;
	float crossMaxSize = 1;


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
			bool isAltPressed = i->IsPressed(Key::AltLeft) || i->IsPressed(Key::AltRight);

			// Enable or disable Mouose boundless mode 
			// regardless of whether we move the cross or not.
			i->SetMouseBoundlessMode(isAltPressed);

			i->movement += i->MouseAxe * Settings().TransitionStep().Get();
			i->movement += i->ArrowAxe * Settings().TransitionStep().Get();
			i->movement += i->NumpadAxe * Settings().TransitionStep().Get();
			});
	}
	void Cross() {
		// Resize cross.
		AddHandler([i = input, c = cross, &sp = crossScaleSpeed, &min = crossMinSize, &max = crossMaxSize] {
			bool isScaleUp = i->IsPressed(Key::N3);
			bool isScaleDown = i->IsPressed(Key::N7);

			if (isScaleUp == isScaleDown)
				return;

			if (isScaleUp) {
				float newSize = c->size *= 1 + sp;
				if (max < newSize)
					c->size = max;
				else
					c->size = newSize;
			}
			else {
				float newSize = c->size *= 1 - sp;
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
			if (i->IsPressed(Key::ControlLeft) || i->IsPressed(Key::ControlRight)) {
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

