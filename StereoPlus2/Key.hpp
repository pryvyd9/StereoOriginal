#pragma once
#include <imgui/imgui_impl_glfw.h>

namespace Key {
	enum class Type {
		Mouse,
		Keyboard,
	};
	enum class Modifier {
		None,
		Control,
		Shift,
		Alt,
	};

	struct KeyPair {
		Type type;
		int code;

		bool operator!=(const KeyPair& v) {
			return type != v.type || code != v.code;
		}
		bool operator==(const KeyPair& v) {
			return !(operator!=(v));
		}
	};

	struct MouseKey : public KeyPair {
		MouseKey(int code) {
			type = Type::Mouse;
			this->code = code;
		}
	};
	struct KeyboardKey : public KeyPair {
		KeyboardKey(int code) {
			type = Type::Keyboard;
			this->code = code;
		}
	};

	struct Combination {
		std::vector<Modifier> modifiers;
		KeyPair key;
		Combination() {}
		Combination(std::vector<Modifier> modifiers, KeyPair key) : modifiers(modifiers), key(key) {}
		Combination(KeyPair key) : key(key) {}

		bool operator!=(const Combination& v) {
			if (key != v.key)
				return false;

			if (modifiers.size() != v.modifiers.size())
				return false;

			for (auto i = 0; i < modifiers.size(); i++)
				if (modifiers[i] != v.modifiers[i])
					return false;

			return true;
		}
	};

	// Mouse
	const KeyPair MouseLeft = MouseKey(GLFW_MOUSE_BUTTON_LEFT);
	const KeyPair MouseRight = MouseKey(GLFW_MOUSE_BUTTON_RIGHT);
	const KeyPair MouseMiddle = MouseKey(GLFW_MOUSE_BUTTON_MIDDLE);

	// Arrows
	const KeyPair Left = KeyboardKey(GLFW_KEY_LEFT);
	const KeyPair Up = KeyboardKey(GLFW_KEY_UP);
	const KeyPair Right = KeyboardKey(GLFW_KEY_RIGHT);
	const KeyPair Down = KeyboardKey(GLFW_KEY_DOWN);

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
	const KeyPair ControlLeft = KeyboardKey(GLFW_KEY_LEFT_CONTROL);
	const KeyPair ControlRight = KeyboardKey(GLFW_KEY_RIGHT_CONTROL);
	const KeyPair AltLeft = KeyboardKey(GLFW_KEY_LEFT_ALT);
	const KeyPair AltRight = KeyboardKey(GLFW_KEY_RIGHT_ALT);
	const KeyPair ShiftLeft = KeyboardKey(GLFW_KEY_LEFT_SHIFT);
	const KeyPair ShiftRight = KeyboardKey(GLFW_KEY_RIGHT_SHIFT);
	const KeyPair Enter = KeyboardKey(GLFW_KEY_ENTER);
	const KeyPair Escape = KeyboardKey(GLFW_KEY_ESCAPE);

	const KeyPair Delete = KeyboardKey(GLFW_KEY_DELETE);

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

	// Functional
	const KeyPair F1 = KeyboardKey(GLFW_KEY_F1);
	const KeyPair F2 = KeyboardKey(GLFW_KEY_F2);
	const KeyPair F3 = KeyboardKey(GLFW_KEY_F3);
	const KeyPair F4 = KeyboardKey(GLFW_KEY_F4);
	const KeyPair F5 = KeyboardKey(GLFW_KEY_F5);
	const KeyPair F6 = KeyboardKey(GLFW_KEY_F6);
	const KeyPair F7 = KeyboardKey(GLFW_KEY_F7);
	const KeyPair F8 = KeyboardKey(GLFW_KEY_F8);
	const KeyPair F9 = KeyboardKey(GLFW_KEY_F9);
	const KeyPair F10 = KeyboardKey(GLFW_KEY_F10);
	const KeyPair F11 = KeyboardKey(GLFW_KEY_F11);
	const KeyPair F12 = KeyboardKey(GLFW_KEY_F12);

}
