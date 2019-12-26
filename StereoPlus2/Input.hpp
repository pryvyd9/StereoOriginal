#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include <map>

enum KeyType {
	Mouse,
	Keyboard,
};

struct KeyPair {
	KeyType type;
	int code;

};

struct KeyStatus {
	KeyType keyType;
	bool isPressed = false;
	bool isDown = false;
	bool isUp = false;
};

struct Key {
	// Mouse
	const KeyPair MouseLeft = KeyPair{ Mouse, GLFW_MOUSE_BUTTON_LEFT };

	// Arrows
	const KeyPair Left	= KeyPair{ Mouse, GLFW_KEY_LEFT };
	const KeyPair Up	= KeyPair{ Mouse, GLFW_KEY_UP };
	const KeyPair Right	= KeyPair{ Mouse, GLFW_KEY_RIGHT };
	const KeyPair Down	= KeyPair{ Mouse, GLFW_KEY_DOWN };

	// NumPad
	const KeyPair N0 = KeyPair{ Keyboard, GLFW_KEY_KP_0 };
	const KeyPair N1 = KeyPair{ Keyboard, GLFW_KEY_KP_1 };
	const KeyPair N2 = KeyPair{ Keyboard, GLFW_KEY_KP_2 };
	const KeyPair N3 = KeyPair{ Keyboard, GLFW_KEY_KP_3 };
	const KeyPair N4 = KeyPair{ Keyboard, GLFW_KEY_KP_4 };
	const KeyPair N5 = KeyPair{ Keyboard, GLFW_KEY_KP_5 };
	const KeyPair N6 = KeyPair{ Keyboard, GLFW_KEY_KP_6 };
	const KeyPair N7 = KeyPair{ Keyboard, GLFW_KEY_KP_7 };
	const KeyPair N8 = KeyPair{ Keyboard, GLFW_KEY_KP_8 };
	const KeyPair N9 = KeyPair{ Keyboard, GLFW_KEY_KP_9 };

	// Special
	const KeyPair ControlLeft	= KeyPair{ Keyboard, GLFW_KEY_LEFT_CONTROL };
	const KeyPair ControlRight	= KeyPair{ Keyboard, GLFW_KEY_RIGHT_CONTROL };
	const KeyPair AltLeft		= KeyPair{ Keyboard, GLFW_KEY_LEFT_ALT };
	const KeyPair AltRight		= KeyPair{ Keyboard, GLFW_KEY_RIGHT_ALT };
	const KeyPair ShiftLeft		= KeyPair{ Keyboard, GLFW_KEY_LEFT_SHIFT };
	const KeyPair ShiftRight	= KeyPair{ Keyboard, GLFW_KEY_RIGHT_SHIFT };

	// Char
	const KeyPair A = KeyPair{ Keyboard, GLFW_KEY_A };

};

// First frame keys are never Up or Down
// as we need to check previous state in order to infer those.
class Input
{
	glm::vec2 mouseOldPos = glm::vec2(0);
	glm::vec2 mouseNewPos = glm::vec2(0);

	bool isMouseBoundlessMode = false;
	bool isRawMouseMotionSupported = false;

	std::map<GLuint, KeyStatus*> keyStatuses;

	void UpdateStatus(GLuint key, KeyStatus* status) {
		bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;

		status->isDown = isPressed && !status->isPressed;
		status->isUp = !isPressed && status->isPressed;
		status->isPressed = isPressed;
	}

	/*void UpdateStatus(GLuint key, KeyStatus* status) {
		bool isPressed = glfwGetMouse(window, key) == GLFW_PRESS;

		status->isDown = isPressed && !status->isPressed;
		status->isUp = !isPressed && status->isPressed;
		status->isPressed = isPressed;
	}*/


	void EnsureKeyStatusExists(GLuint key) {
		if (keyStatuses.find(key) != keyStatuses.end())
			return;

		KeyStatus * status = new KeyStatus();
		keyStatuses.insert({ key, status });
		UpdateStatus(key, status);
	}


public:
	GLFWwindow* window;

	std::vector<std::function<void()>> handlers;


	bool IsPressed(GLuint key)
	{
		EnsureKeyStatusExists(key);

		return keyStatuses[key]->isPressed;
	}

	bool IsDown(GLuint key)
	{
		EnsureKeyStatusExists(key);

		return keyStatuses[key]->isDown;
	}

	bool IsUp(GLuint key)
	{
		EnsureKeyStatusExists(key);

		return keyStatuses[key]->isUp;
	}


	glm::vec2 MousePosition() {
		return mouseNewPos;
	}

	glm::vec2 MouseMoveDirection() {
		return glm::length(mouseNewPos - mouseOldPos) == 0 ? glm::vec2(0) : glm::normalize(mouseNewPos - mouseOldPos);
	}

	float MouseSpeed() {
		return glm::length(mouseNewPos - mouseOldPos);
	}

	void SetMouseBoundlessMode(bool enable) {
		if (enable == isMouseBoundlessMode)
			return;

		if (enable)
		{
			if (isRawMouseMotionSupported)
				glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			// We can disable raw mouse motion mode even if it's not supported
			// so we don't bother with checking it.
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		isMouseBoundlessMode = enable;
	}

	void ProcessInput()
	{
		// Update mouse position
		mouseOldPos = mouseNewPos;
		mouseNewPos = ImGui::GetMousePos();

		// Update key statuses
		for (auto node : keyStatuses)
		{
			UpdateStatus(node.first, node.second);
		}

		// Handle OnInput actions
		for (auto handler : handlers)
		{
			handler();
		}
	}

	bool Init() {
		isRawMouseMotionSupported = glfwRawMouseMotionSupported();

		return true;
	}

	~Input() {
		for (auto node : keyStatuses)
		{
			delete node.second;
		}
	}
};

class KeyBinding
{
	void AddHandler(std::function<void()> func) {
		input->handlers.push_back(func);
	}


	bool isAxeModeEnabled;
public:
	Input* input;
	Cross* cross;

	float crossMovementSpeed = 0.01;

	float crossScaleSpeed = 0.01;
	float crossMinSize = 0.001;
	float crossMaxSize = 1;

	void AddHandler(std::function<void(Input*)> func) {
		input->handlers.push_back([=] { func(input); });
	}


	void MoveCross() {
		// Simple mouse control
		//AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
		//	bool isAltPressed = i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT);

		//	// Enable or disable Mouose boundless mode 
		//	// regardless of whether we move the cross or not.
		//	i->SetMouseBoundlessMode(isAltPressed);

		//	if (isAltPressed) {
		//		bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

		//		auto m = i->MouseMoveDirection() * sp * (isHighPrecisionMode ? 0.1f : 1);
		//		c->Position.x += m.x;
		//		c->Position.y -= m.y;
		//		c->Refresh();
		//	}
		//});

#pragma region Advanced mouse+keyboard control

		// Axe mode switch A
		AddHandler([i = input, c = cross, sp = crossMovementSpeed, &axeMode = isAxeModeEnabled] {
			if (i->IsDown(GLFW_KEY_A))
			{
				axeMode = !axeMode;
			}
			});
		
		// Advanced mouse+keyboard control
		AddHandler([i = input, c = cross, sp = crossMovementSpeed, &axeMode = isAxeModeEnabled] {
			if (axeMode)
			{
				// alt x y
				// ctrl x z
				// shift y z

				glm::vec3 axes = glm::vec3(
					(i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT)) + (i->IsPressed(GLFW_KEY_LEFT_CONTROL) || i->IsPressed(GLFW_KEY_RIGHT_CONTROL)),
					(i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT)) + (i->IsPressed(GLFW_KEY_LEFT_SHIFT) || i->IsPressed(GLFW_KEY_RIGHT_SHIFT)),
					(i->IsPressed(GLFW_KEY_LEFT_CONTROL) || i->IsPressed(GLFW_KEY_RIGHT_CONTROL)) + (i->IsPressed(GLFW_KEY_LEFT_SHIFT) || i->IsPressed(GLFW_KEY_RIGHT_SHIFT))
				);

				bool isZero = axes.x == 0 && axes.y == 0 && axes.z == 0;
				// If all three keys are down then it's impossible to move anywhere.
				bool isBlocked = axes.x == 2 && axes.y == 2 && axes.z == 2;

				bool mustReturn = isBlocked || isZero;

				// Enable or disable Mouose boundless mode 
				// regardless of whether we move the cross or not.
				i->SetMouseBoundlessMode(!mustReturn);

				if (mustReturn)
				{
					return;
				}

				bool isAxeLocked = axes.x == 2 || axes.y == 2 || axes.z == 2;
				if (isAxeLocked)
				{
					int lockedAxeIndex = axes.x == 2 ? 0 : axes.y == 2 ? 1 : 2;

					// Cross position.
					float* destination = &c->Position[lockedAxeIndex];

					axes -= 1;

					// Enable or disable Mouose boundless mode 
					// regardless of whether we move the cross or not.
					i->SetMouseBoundlessMode(true);

					auto m = i->MouseMoveDirection() * sp;

					*destination += m.x;

					c->Refresh();

					return;
				}

				int lockedPlane[2];
				{
					int i = 0;
					if (axes.x != 0) lockedPlane[i++] = 0;
					if (axes.y != 0) lockedPlane[i++] = 1;
					if (axes.z != 0) lockedPlane[i++] = 2;
				}

				// Enable or disable Mouse boundless mode 
				// regardless of whether we move the cross or not.
				i->SetMouseBoundlessMode(true);

				auto m = i->MouseMoveDirection() * sp;

				c->Position[lockedPlane[0]] += m.x;
				c->Position[lockedPlane[1]] -= m.y;

				c->Refresh();

				return;
			}

			bool isAltPressed = i->IsPressed(GLFW_KEY_LEFT_ALT) || i->IsPressed(GLFW_KEY_RIGHT_ALT);

			// Enable or disable Mouose boundless mode 
			// regardless of whether we move the cross or not.
			i->SetMouseBoundlessMode(isAltPressed);

			if (isAltPressed) {
				bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

				auto m = i->MouseMoveDirection() * sp * (isHighPrecisionMode ? 0.1f : 1);
				c->Position.x += m.x;
				c->Position.y -= m.y;
				c->Refresh();
			}
			});

#pragma endregion

		// Move cross with arrows/arrows+Ctrl
		AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
			glm::vec2 movement = glm::vec2(
				-i->IsPressed(GLFW_KEY_LEFT) + i->IsPressed(GLFW_KEY_RIGHT),
				-i->IsPressed(GLFW_KEY_UP) + i->IsPressed(GLFW_KEY_DOWN));

			if (movement.x != 0 || movement.y != 0)
			{
				bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

				movement *= sp * (isHighPrecisionMode ? 0.1f : 1);

				c->Position.x += movement.x;
				c->Position.y -= movement.y;
				c->Refresh();
			}
			});

		// Move cross with numpad/numpad+Ctrl
		AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
			glm::vec3 movement = glm::vec3(
				-i->IsPressed(GLFW_KEY_KP_4) + i->IsPressed(GLFW_KEY_KP_6),
				-i->IsPressed(GLFW_KEY_KP_2) + i->IsPressed(GLFW_KEY_KP_8),
				-i->IsPressed(GLFW_KEY_KP_1) + i->IsPressed(GLFW_KEY_KP_9));

			if (movement.x != 0 || movement.y != 0 || movement.z != 0)
			{
				bool isHighPrecisionMode = i->IsPressed(GLFW_KEY_LEFT_CONTROL);

				movement *= sp * (isHighPrecisionMode ? 0.1f : 1);

				c->Position += movement;
				c->Refresh();
			}
			});
	}

	void Cross() {
		MoveCross();

		// Resize cross.
		AddHandler([i = input, c = cross, &sp = crossScaleSpeed, &min = crossMinSize, &max = crossMaxSize] {
			bool isScaleUp = i->IsPressed(GLFW_KEY_KP_3);
			bool isScaleDown = i->IsPressed(GLFW_KEY_KP_7);

			if (isScaleUp == isScaleDown)
				return;

			if (isScaleUp)
			{
				float newSize = c->size *= 1 + sp;
				if (max < newSize)
					c->size = max;
				else
					c->size = newSize;
			}
			else
			{
				float newSize = c->size *= 1 - sp;
				if (newSize < min)
					c->size = min;
				else
					c->size = newSize;
			}

			c->Refresh();
			});
	}

	bool Init() {
		Cross();

		return true;
	}
};

