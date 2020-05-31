#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include <map>


namespace Key {
	enum KeyType {
		Mouse,
		Keyboard,
	};

	struct KeyPair {
		KeyType type;
		int code;
	};

	struct MouseKey : public KeyPair {
		MouseKey(int code) {
			type = Mouse;
			this->code = code;
		}
	};
	struct KeyboardKey : public KeyPair {
		KeyboardKey(int code) {
			type = Keyboard;
			this->code = code;
		}
	};



	// Mouse
	const KeyPair MouseLeft = MouseKey(GLFW_MOUSE_BUTTON_LEFT);

	// Arrows
	const KeyPair Left	= KeyboardKey(GLFW_KEY_LEFT	);
	const KeyPair Up	= KeyboardKey(GLFW_KEY_UP	);
	const KeyPair Right = KeyboardKey(GLFW_KEY_RIGHT);
	const KeyPair Down	= KeyboardKey(GLFW_KEY_DOWN	);

	// NumPad
	const KeyPair N0 = KeyboardKey(GLFW_KEY_KP_0);
	const KeyPair N1 = KeyboardKey(GLFW_KEY_KP_1);
	const KeyPair N2 = KeyboardKey(GLFW_KEY_KP_2);
	const KeyPair N3 = KeyboardKey(GLFW_KEY_KP_3);
	const KeyPair N4 = KeyboardKey(GLFW_KEY_KP_4);
	const KeyPair N5 = KeyboardKey(GLFW_KEY_KP_5);
	const KeyPair N6 = KeyboardKey(GLFW_KEY_KP_6);
	const KeyPair N7 = KeyboardKey(GLFW_KEY_KP_7);
	const KeyPair N8 = KeyboardKey(GLFW_KEY_KP_8);
	const KeyPair N9 = KeyboardKey(GLFW_KEY_KP_9);
	const KeyPair NEnter = KeyboardKey(GLFW_KEY_KP_ENTER);

	// Special
	const KeyPair ControlLeft	= KeyboardKey(GLFW_KEY_LEFT_CONTROL	);
	const KeyPair ControlRight	= KeyboardKey(GLFW_KEY_RIGHT_CONTROL);
	const KeyPair AltLeft		= KeyboardKey(GLFW_KEY_LEFT_ALT		);
	const KeyPair AltRight		= KeyboardKey(GLFW_KEY_RIGHT_ALT	);
	const KeyPair ShiftLeft		= KeyboardKey(GLFW_KEY_LEFT_SHIFT	);
	const KeyPair ShiftRight	= KeyboardKey(GLFW_KEY_RIGHT_SHIFT	);
	const KeyPair Enter			= KeyboardKey(GLFW_KEY_ENTER		);
	const KeyPair Escape		= KeyboardKey(GLFW_KEY_ESCAPE		);

	// Char
	const KeyPair A = KeyboardKey(GLFW_KEY_A);

}




// First frame keys are never Up or Down
// as we need to check previous state in order to infer those.
class Input
{
	struct KeyPairComparator {
		bool operator() (const Key::KeyPair& lhs, const Key::KeyPair& rhs) const {
			return lhs.code < rhs.code || lhs.code == rhs.code && lhs.type < rhs.type;
		}
	};
	struct KeyStatus {
		Key::KeyType keyType;
		bool isPressed;
		bool isDown;
		bool isUp;
	};


	glm::vec2 mouseOldPos;
	glm::vec2 mouseNewPos;

	bool isMouseBoundlessMode = false;
	bool isRawMouseMotionSupported = false;

	std::map<Key::KeyPair, KeyStatus*, KeyPairComparator> keyStatuses;

	float mouseSensivity = 1e-2;
	float mouseMaxMagnitude = 1e4;

	
	void UpdateStatus(Key::KeyPair key, KeyStatus* status) {
		bool isPressed = 
			key.type == Key::Mouse 
			? glfwGetMouseButton(glWindow, key.code) == GLFW_PRESS
			: glfwGetKey(glWindow, key.code) == GLFW_PRESS;

		status->isDown = isPressed && !status->isPressed;
		status->isUp = !isPressed && status->isPressed;
		status->isPressed = isPressed;
	}

	KeyStatus* TryGetStatusEnsuringItExists(const Key::KeyPair& key) {
		auto status = keyStatuses.find(key);

		if (status != keyStatuses.end())
			return status._Ptr->_Myval.second;

		{
			KeyStatus* status = new KeyStatus();
			keyStatuses.insert({ key, status });
			UpdateStatus(key, status);
			return status;
		}
	}

public:
	GLFWwindow* glWindow;

	glm::vec3 movement;
	std::map<size_t, std::function<void()>> handlers;

	bool IsMoved() {
		return movement != glm::vec3();
	}

	// Is pressed
	bool IsPressed(Key::KeyPair key) {
		return TryGetStatusEnsuringItExists(key)->isPressed;
	}

	// Was pressed down
	bool IsDown(Key::KeyPair key) {
		return TryGetStatusEnsuringItExists(key)->isDown;
	}

	// Was lift up
	bool IsUp(Key::KeyPair key) {
		return TryGetStatusEnsuringItExists(key)->isUp;
	}

	glm::vec2 MousePosition() {
		return mouseNewPos;
	}

	glm::vec2 MouseMoveDirection() {
		return glm::length(mouseNewPos - mouseOldPos) == 0 ? glm::vec2(0) : glm::normalize(mouseNewPos - mouseOldPos);
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

		if (enable)
		{
			if (isRawMouseMotionSupported)
				glfwSetInputMode(glWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			// We can disable raw mouse motion mode even if it's not supported
			// so we don't bother with checking it.
			glfwSetInputMode(glWindow, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

			glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		isMouseBoundlessMode = enable;
	}

	void ProcessInput() {
		// Update mouse position
		mouseOldPos = mouseNewPos;
		mouseNewPos = ImGui::GetMousePos();
		movement = glm::vec3();

		// Update key statuses
		for (auto node : keyStatuses)
			UpdateStatus(node.first, node.second);

		// Handle OnInput actions
		for (auto handler : handlers)
			handler.second();
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


	bool isAxeModeEnabled;
public:
	Input* input;
	Cross* cross;

	float crossMovementSpeed = 0.01;

	float crossScaleSpeed = 0.01;
	float crossMinSize = 0.001;
	float crossMaxSize = 1;

	size_t AddHandler(std::function<void()> func) {
		static size_t id = 0;

		auto cmd = new FuncCommand();
		cmd->func = [id = id, input = input, func = func] {
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

	void MoveCross() {
		// Axe mode switch A
		AddHandler([i = input, c = cross, sp = crossMovementSpeed, &axeMode = isAxeModeEnabled] {
			if (i->IsDown(Key::A))
				axeMode = !axeMode;
			});
		
		// Advanced mouse+keyboard control
		AddHandler([i = input, c = cross, sp = crossMovementSpeed, &axeMode = isAxeModeEnabled] {
			if (axeMode) {
				// alt x y
				// ctrl x z
				// shift y z
				
				glm::vec3 axes = glm::vec3(
					(i->IsPressed(Key::AltLeft) || i->IsPressed(Key::AltRight)) + (i->IsPressed(Key::ControlLeft) || i->IsPressed(Key::ControlRight)),
					(i->IsPressed(Key::AltLeft) || i->IsPressed(Key::AltRight)) + (i->IsPressed(Key::ShiftLeft) || i->IsPressed(Key::ShiftRight)),
					(i->IsPressed(Key::ControlLeft) || i->IsPressed(Key::ControlRight)) + (i->IsPressed(Key::ShiftLeft) || i->IsPressed(Key::ShiftRight))
				);

				bool isZero = axes.x == 0 && axes.y == 0 && axes.z == 0;
				// If all three keys are down then it's impossible to move anywhere.
				bool isBlocked = axes.x == 2 && axes.y == 2 && axes.z == 2;

				bool mustReturn = isBlocked || isZero;

				// Enable or disable Mouose boundless mode 
				// regardless of whether we move the cross or not.
				i->SetMouseBoundlessMode(!mustReturn);

				if (mustReturn)
					return;

				bool isAxeLocked = axes.x == 2 || axes.y == 2 || axes.z == 2;
				if (isAxeLocked) {
					int lockedAxeIndex = axes.x == 2 ? 0 : axes.y == 2 ? 1 : 2;
					axes -= 1;

					// Enable or disable Mouose boundless mode 
					// regardless of whether we move the cross or not.
					i->SetMouseBoundlessMode(true);

					auto m = i->MouseMoveDirection() * sp;

					// Cross position.
					//*const_cast<float*>(&c->GetLocalPosition()[lockedAxeIndex]) += m.x;
					i->movement[lockedAxeIndex] += m.x;

					c->ForceUpdateCache();

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

				//auto position = const_cast<glm::vec3*>(&c->GetLocalPosition());
				//(*position)[lockedPlane[0]] += m.x;
				//(*position)[lockedPlane[1]] -= m.y;
				i->movement[lockedPlane[0]] += m.x;
				i->movement[lockedPlane[1]] -= m.y;


				c->ForceUpdateCache();

				return;
			}

			bool isAltPressed = i->IsPressed(Key::AltLeft) || i->IsPressed(Key::AltRight);

			// Enable or disable Mouose boundless mode 
			// regardless of whether we move the cross or not.
			i->SetMouseBoundlessMode(isAltPressed);

			if (isAltPressed) {
				bool isHighPrecisionMode = i->IsPressed(Key::ControlLeft);

				auto m = i->MouseMoveDirection() * sp * (isHighPrecisionMode ? 0.1f : 1);

				//auto position = const_cast<glm::vec3*>(&c->GetLocalPosition());
				//position->x += m.x;
				//position->y -= m.y;
				i->movement.x += m.x;
				i->movement.y -= m.y;

				c->ForceUpdateCache();
			}
			});

		// Move cross with arrows/arrows+Ctrl
		AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
			glm::vec2 m = glm::vec2(
				-i->IsPressed(Key::Left) + i->IsPressed(Key::Right),
				-i->IsPressed(Key::Up) + i->IsPressed(Key::Down));

			if (m.x != 0 || m.y != 0) {
				bool isHighPrecisionMode = i->IsPressed(Key::ControlLeft);

				m *= sp * (isHighPrecisionMode ? 0.1f : 1);

				//auto position = const_cast<glm::vec3*>(&c->GetLocalPosition());
				//position->x += movement.x;
				//position->y -= movement.y;
				i->movement.x += m.x;
				i->movement.y -= m.y;

				c->ForceUpdateCache();
			}
			});

		// Move cross with numpad/numpad+Ctrl
		AddHandler([i = input, c = cross, sp = crossMovementSpeed] {
			glm::vec3 movement = glm::vec3(
				-i->IsPressed(Key::N4) + i->IsPressed(Key::N6),
				-i->IsPressed(Key::N2) + i->IsPressed(Key::N8),
				-i->IsPressed(Key::N1) + i->IsPressed(Key::N9));

			if (movement.x != 0 || movement.y != 0 || movement.z != 0) {
				bool isHighPrecisionMode = i->IsPressed(Key::ControlLeft);

				movement *= sp * (isHighPrecisionMode ? 0.1f : 1);

				//*const_cast<glm::vec3*>(&c->GetLocalPosition()) += movement;
				i->movement += movement;

				c->ForceUpdateCache();
			}
			});
	}
	void Cross() {
		MoveCross();

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

	bool Init() {
		Cross();

		return true;
	}
};

