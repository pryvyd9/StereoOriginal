#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "DomainUtils.hpp"
#include <map>
#include <vector>

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
	const KeyPair MouseRight = MouseKey(GLFW_MOUSE_BUTTON_RIGHT);

	// Arrows
	const KeyPair Left	= KeyboardKey(GLFW_KEY_LEFT	);
	const KeyPair Up	= KeyboardKey(GLFW_KEY_UP	);
	const KeyPair Right = KeyboardKey(GLFW_KEY_RIGHT);
	const KeyPair Down	= KeyboardKey(GLFW_KEY_DOWN	);

	// NumPad
	const KeyPair N0		= KeyboardKey(GLFW_KEY_KP_0		);
	const KeyPair N1		= KeyboardKey(GLFW_KEY_KP_1		);
	const KeyPair N2		= KeyboardKey(GLFW_KEY_KP_2		);
	const KeyPair N3		= KeyboardKey(GLFW_KEY_KP_3		);
	const KeyPair N4		= KeyboardKey(GLFW_KEY_KP_4		);
	const KeyPair N5		= KeyboardKey(GLFW_KEY_KP_5		);
	const KeyPair N6		= KeyboardKey(GLFW_KEY_KP_6		);
	const KeyPair N7		= KeyboardKey(GLFW_KEY_KP_7		);
	const KeyPair N8		= KeyboardKey(GLFW_KEY_KP_8		);
	const KeyPair N9		= KeyboardKey(GLFW_KEY_KP_9		);
	const KeyPair NEnter	= KeyboardKey(GLFW_KEY_KP_ENTER	);

	// Special
	const KeyPair ControlLeft	= KeyboardKey(GLFW_KEY_LEFT_CONTROL	);
	const KeyPair ControlRight	= KeyboardKey(GLFW_KEY_RIGHT_CONTROL);
	const KeyPair AltLeft		= KeyboardKey(GLFW_KEY_LEFT_ALT		);
	const KeyPair AltRight		= KeyboardKey(GLFW_KEY_RIGHT_ALT	);
	const KeyPair ShiftLeft		= KeyboardKey(GLFW_KEY_LEFT_SHIFT	);
	const KeyPair ShiftRight	= KeyboardKey(GLFW_KEY_RIGHT_SHIFT	);
	const KeyPair Enter			= KeyboardKey(GLFW_KEY_ENTER		);
	const KeyPair Escape		= KeyboardKey(GLFW_KEY_ESCAPE		);

	const KeyPair Delete		= KeyboardKey(GLFW_KEY_DELETE		);

	// Char
	const KeyPair A = KeyboardKey(GLFW_KEY_A);
	const KeyPair B = KeyboardKey(GLFW_KEY_B);
	const KeyPair C = KeyboardKey(GLFW_KEY_C);
	const KeyPair D = KeyboardKey(GLFW_KEY_D);
	const KeyPair E = KeyboardKey(GLFW_KEY_E);
	const KeyPair F = KeyboardKey(GLFW_KEY_F);
	const KeyPair G = KeyboardKey(GLFW_KEY_G);
	const KeyPair H = KeyboardKey(GLFW_KEY_H);
	const KeyPair I = KeyboardKey(GLFW_KEY_I);
	const KeyPair J = KeyboardKey(GLFW_KEY_J);
	const KeyPair K = KeyboardKey(GLFW_KEY_K);
	const KeyPair L = KeyboardKey(GLFW_KEY_L);
	const KeyPair M = KeyboardKey(GLFW_KEY_M);
	const KeyPair N = KeyboardKey(GLFW_KEY_N);
	const KeyPair O = KeyboardKey(GLFW_KEY_O);
	const KeyPair P = KeyboardKey(GLFW_KEY_P);
	const KeyPair Q = KeyboardKey(GLFW_KEY_Q);
	const KeyPair R = KeyboardKey(GLFW_KEY_R);
	const KeyPair S = KeyboardKey(GLFW_KEY_S);
	const KeyPair T = KeyboardKey(GLFW_KEY_T);
	const KeyPair U = KeyboardKey(GLFW_KEY_U);
	const KeyPair V = KeyboardKey(GLFW_KEY_V);
	const KeyPair W = KeyboardKey(GLFW_KEY_W);
	const KeyPair X = KeyboardKey(GLFW_KEY_X);
	const KeyPair Y = KeyboardKey(GLFW_KEY_Y);
	const KeyPair Z = KeyboardKey(GLFW_KEY_Z);
}




// First frame keys are never Up or Down
// as we need to check previous state in order to infer those.
class Input {
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

	std::map<Key::KeyPair, KeyStatus*, KeyPairComparator> keyStatuses;

	float mouseSensivity = 1e-2;
	float mouseMaxMagnitude = 1e4;

	bool isAnythingPressed = false;

	ContinuousInput continuousInputOneSecondDelay = ContinuousInput(1);
	ContinuousInput continuousInputNoDelay = ContinuousInput(0);

	void UpdateStatus(const Key::KeyPair& key, KeyStatus* status) {
		bool isPressed = 
			key.type == Key::Mouse 
			? glfwGetMouseButton(glWindow, key.code) == GLFW_PRESS
			: glfwGetKey(glWindow, key.code) == GLFW_PRESS;

		if (isPressed)
			isAnythingPressed = true;

		status->isDown = isPressed && !status->isPressed;
		status->isUp = !isPressed && status->isPressed;
		status->isPressed = isPressed;
	}

	KeyStatus* TryGetStatusEnsuringItExists(const Key::KeyPair& key) {
		auto status = keyStatuses.find(key);
		if (status != keyStatuses.end())
			return status._Ptr->_Myval.second;

		KeyStatus* ns = new KeyStatus();
		keyStatuses.insert({ key, ns });
		UpdateStatus(key, ns);
		return ns;
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

	// Does not include mouse movement.
	bool IsContinuousInputOneSecondDelay() {
		return continuousInputOneSecondDelay.isContinuousInput;
	}
	// Does not include mouse movement.
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
		// Update mouse position
		mouseOldPos = mouseNewPos;
		mouseNewPos = ImGui::GetMousePos();
		movement = glm::vec3();

		// Update key statuses
		for (auto node : keyStatuses)
			UpdateStatus(node.first, node.second);

		continuousInputOneSecondDelay.Process(isAnythingPressed);
		continuousInputNoDelay.Process(isAnythingPressed);

		// Handle OnInput actions
		for (auto [id,handler] : handlers)
			handler();

		continuousInputOneSecondDelay.UpdateOld(isAnythingPressed);
		continuousInputNoDelay.UpdateOld(isAnythingPressed);
		isAnythingPressed = false;
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

class KeyBinding {
	bool isAxeModeEnabled;
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
	void MoveCross() {
		// Axe mode switch A
		AddHandler([i = input, c = cross, &axeMode = isAxeModeEnabled] {
			if (i->IsDown(Key::A))
				axeMode = !axeMode;
			});
		
		// Advanced mouse+keyboard control
		AddHandler([i = input, &axeMode = isAxeModeEnabled] {
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

					auto m = i->MouseMoveDirection() * Settings().CrossSpeed().Get();
					i->movement[lockedAxeIndex] += m.x;

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

				auto m = i->MouseMoveDirection() * Settings().CrossSpeed().Get();

				i->movement[lockedPlane[0]] += m.x;
				i->movement[lockedPlane[1]] -= m.y;

				return;
			}

			bool isAltPressed = i->IsPressed(Key::AltLeft) || i->IsPressed(Key::AltRight);

			// Enable or disable Mouose boundless mode 
			// regardless of whether we move the cross or not.
			i->SetMouseBoundlessMode(isAltPressed);

			if (isAltPressed) {
				auto speed = Settings().CrossSpeed().Get();

				if (auto isHighPrecisionMode = i->IsPressed(Key::ControlLeft); 
					isHighPrecisionMode)
					speed *= 0.1f;

				auto m = i->MouseMoveDirection() * speed;

				if (i->IsPressed(Key::MouseRight))
					i->movement.z -= m.y;
				else {
					i->movement.x += m.x;
					i->movement.y -= m.y;
				}
			}
			});

		// Move cross with arrows/arrows+Ctrl
		AddHandler([i = input] {
			// If nothing is pressed then return.
			if (!i->IsContinuousInputNoDelay())
				return;

			auto m = glm::vec2(
				-i->IsPressed(Key::Left) + i->IsPressed(Key::Right),
				-i->IsPressed(Key::Up) + i->IsPressed(Key::Down));

			if (static auto zero = glm::vec2(); m == zero)
				return;

			bool isHighPrecisionMode = i->IsPressed(Key::ControlLeft);

			m *= Settings().CrossSpeed().Get() * (isHighPrecisionMode ? 0.1f : 1);

			i->movement.x += m.x;
			i->movement.y -= m.y;
			});

		// Move cross with numpad/numpad+Ctrl
		AddHandler([i = input] {
			// If nothing is pressed then return.
			if (!i->IsContinuousInputNoDelay())
				return;

			auto movement = glm::vec3(
				-i->IsPressed(Key::N4) + i->IsPressed(Key::N6),
				-i->IsPressed(Key::N2) + i->IsPressed(Key::N8),
				-i->IsPressed(Key::N1) + i->IsPressed(Key::N9));

			if (static auto zero = glm::vec3(); movement == zero)
				return;

			bool isHighPrecisionMode = i->IsPressed(Key::ControlLeft);

			i->movement += movement * Settings().CrossSpeed().Get() * (isHighPrecisionMode ? 0.1f : 1);
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
		Cross();
		ChangeBuffer();

		return true;
	}
};

