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
	class ContinuousInput {
		bool iscontinuousInputLast = false;
		bool isAnythingPressedLast = false;
		size_t lastPressedTime;
	public:
		bool isContinuousInput = false;
		// seconds
		float continuousInputAwaitTime;


		ContinuousInput(float continuousInputAwaitTime) : continuousInputAwaitTime(continuousInputAwaitTime) {}

		void Process(bool isAnythingPressed) {
			iscontinuousInputLast = isContinuousInput;

			if (isAnythingPressed) {
				isContinuousInput = true;
				lastPressedTime = Time::GetTime();
			}
			else if (isContinuousInput && (Time::GetTime() - lastPressedTime) > continuousInputAwaitTime * 1e3)
				isContinuousInput = false;
		}

		bool HasStopped() {
			return !isContinuousInput && iscontinuousInputLast;
		}
	};

	StaticField(glm::vec2, mouseOldPos)
	StaticField(glm::vec2, mouseNewPos)

	//StaticField(bool, isMouseBoundlessMode)
	StaticField(bool, isRawMouseMotionSupported)

	StaticFieldDefault(float, mouseSensivity, 1e-2)
	StaticFieldDefault(float, mouseMaxMagnitude, 1e4)

	StaticFieldDefault(ContinuousInput, continuousInputOneSecondDelay, ContinuousInput(1))
	StaticFieldDefault(ContinuousInput, continuousInputNoDelay, ContinuousInput(0))

	static std::map<size_t, std::function<void()>>& handlers() {
		static std::map<size_t, std::function<void()>> v;
		return v;
	}

	static CombinationNode& combinationTree() {
		static CombinationNode v;
		return v;
	}

	static void FillAxes() {
		mouseOldPos() = mouseNewPos();
		mouseNewPos() = ImGui::GetMousePos();
		
		auto useDiscreteMovement = Settings::UseDiscreteMovement().Get();
		ArrowAxe() = glm::vec3(
			-IsPressed(Key::Left, useDiscreteMovement) + IsPressed(Key::Right, useDiscreteMovement),
			IsPressed(Key::Up, useDiscreteMovement) + -IsPressed(Key::Down, useDiscreteMovement),
			0);
		NumpadAxe() = IsPressed(Key::Modifier::Shift)
			? glm::vec3()
			: glm::vec3(
				-IsPressed(Key::N4, useDiscreteMovement) + IsPressed(Key::N6, useDiscreteMovement),
				-IsPressed(Key::N2, useDiscreteMovement) + IsPressed(Key::N8, useDiscreteMovement),
				-IsPressed(Key::N9, useDiscreteMovement) + IsPressed(Key::N1, useDiscreteMovement));

		if (IsCustomRenderImageActive().Get()) {
			auto mouseMoveDirection = MouseMoveDirection();
			MouseAxe() = IsPressed(Key::MouseRight)
				? glm::vec3(0, 0, mouseMoveDirection.y)
				: glm::vec3(mouseMoveDirection.x, -mouseMoveDirection.y, 0);

		}
		else MouseAxe() = glm::vec3();
	}

	static bool InsertCombination(
		CombinationNode& node,
		const std::vector<Key::Modifier>::const_iterator* modifiersCurrent,
		const std::vector<Key::Modifier>::const_iterator* modifiersEnd,
		const Key::KeyPair& endKey,
		const std::function<void()>& callback) {
		if (modifiersCurrent == modifiersEnd || *modifiersCurrent == *modifiersEnd) {
			if (node.callbacks.find(endKey) != node.callbacks.end())
				return false;

			node.callbacks[endKey] = callback;
			return true;
		}

		auto modifiersNext = (*modifiersCurrent) + 1;

		if (auto c = node.children.find(*modifiersCurrent->_Ptr);
			c != node.children.end())
			return InsertCombination(c._Ptr->_Myval.second, &modifiersNext, modifiersEnd, endKey, callback);

		node.children[*modifiersCurrent->_Ptr] = CombinationNode();
		return InsertCombination(node.children[*modifiersCurrent->_Ptr], &modifiersNext, modifiersEnd, endKey, callback);
	}


	//static bool RemoveCombination(
	//	CombinationNode& node,
	//	const std::vector<Key::Modifier>::const_iterator* modifiersCurrent,
	//	const std::vector<Key::Modifier>::const_iterator* modifiersEnd,
	//	const Key::KeyPair& endKey,
	//	const std::function<void()>& callback) {
	//	if (modifiersCurrent == modifiersEnd || *modifiersCurrent == *modifiersEnd) {
	//		if (auto i = node.callbacks.find(endKey); i != node.callbacks.end()) {
	//			node.callbacks.erase(i);
	//			return true;
	//		}

	//		return false;
	//	}

	//	auto modifiersNext = (*modifiersCurrent) + 1;

	//	if (auto c = node.children.find(*modifiersCurrent->_Ptr);
	//		c != node.children.end())
	//		return RemoveCombination(c._Ptr->_Myval.second, &modifiersNext, modifiersEnd, endKey, callback);

	//	node.children[*modifiersCurrent->_Ptr] = CombinationNode();
	//	return RemoveCombination(node.children[*modifiersCurrent->_Ptr], &modifiersNext, modifiersEnd, endKey, callback);
	//}

	// Prevents combinations with letters or printable symbols
	// being executed while the keyboard is captured by text input.
	// Applied to Shift-modified and non-modified keys, Escape, Enter and other keys used while interacting with text.
	static bool IsAcceptableCombination(const int key, const bool ignoreCapturedKeys, const int level, const Key::Modifier& modifier) {
		if (!ignoreCapturedKeys || key < GLFW_KEY_SPACE || key > GLFW_KEY_NUM_LOCK && key < GLFW_KEY_KP_0 || key > GLFW_KEY_KP_EQUAL)
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
	StaticProperty(ImGuiIO*, io)

	StaticField(glm::vec3, ArrowAxe)
	StaticField(glm::vec3, NumpadAxe)
	StaticField(glm::vec3, MouseAxe)

	StaticField(glm::vec3, movement)

	StaticProperty(bool, IsCustomRenderImageActive)

	static bool IsMoved() {
		return movement() != glm::vec3();
	}

	static bool IsPressed(Key::KeyPair key, bool discreteInput = false) {
		auto* io = &ImGui::GetIO();

		if (discreteInput)
			return key.type == Key::Type::Mouse
				? ImGui::IsMouseClicked(key.code, true)
				: ImGui::IsKeyPressed(key.code, true);

		return key.type == Key::Type::Mouse
			? ImGui::IsMouseDown(key.code)
			: ImGui::IsKeyDown(key.code);
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

	// Checks if key changed state to Pressed.
	// ignoreCaptured ignores the key and returns false if keyboard is captured by text input.
	static bool IsDown(const Key::KeyPair& key, bool ignoreCaptured = false) {
		if (ignoreCaptured && ImGui::GetIO().WantCaptureKeyboard && !IsAcceptableCombination(key.code, true, 0, Key::Modifier::None))
			return false;

		return key.type == Key::Type::Mouse
			? ImGui::IsMouseClicked(key.code, false)
			: ImGui::IsKeyPressed(key.code, false);
	}

	static bool IsUp(Key::KeyPair key) {
		return key.type == Key::Type::Mouse
			? ImGui::IsMouseReleased(key.code)
			: ImGui::IsKeyReleased(key.code);
	}

	static glm::vec2 MousePosition() {
		return mouseNewPos();
	}
	static glm::vec2 MouseMoveDirection() {
		return glm::length(mouseNewPos() - mouseOldPos()) == 0 ? glm::vec2(0) : mouseNewPos() - mouseOldPos();
	}

	// Does not include mouse movement nor scrolling.
	static bool IsContinuousInputOneSecondDelay() {
		return continuousInputOneSecondDelay().isContinuousInput;
	}
	static bool IsContinuousInputNoDelay() {
		return continuousInputNoDelay().isContinuousInput;
	}
	static bool HasContinuousInputOneSecondDelayStopped() {
		return continuousInputOneSecondDelay().HasStopped();
	}
	static bool HasContinuousInputNoDelayStopped() {
		return continuousInputNoDelay().HasStopped();
	}

	static bool MouseMoved() {
		return MouseSpeed() > mouseSensivity() && MouseSpeed() < mouseMaxMagnitude();
	}
	static float MouseSpeed() {
		return glm::length(mouseNewPos() - mouseOldPos());
	}
	static void SetMouseBoundlessMode(bool enable) {
		if (enable) {
			if (glfwRawMouseMotionSupported())
				glfwSetInputMode(&GLFWindow().Get(), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

			glfwSetInputMode(&GLFWindow().Get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else {
			// We can disable raw mouse motion mode even if it's not supported
			// so we don't bother with checking it.
			glfwSetInputMode(&GLFWindow().Get(), GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

			glfwSetInputMode(&GLFWindow().Get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	static size_t AddHandler(std::function<void()> func) {
		// 0 means it isn't assigned.
		static size_t id = 1;

		(new FuncCommand())->func = [id = id, func = func] {
			handlers()[id] = func;
		};

		return id++;
	}
	static void RemoveHandler(size_t& id) {
		auto cmd = new FuncCommand();
		cmd->func = [id] {
			handlers().erase(id);
		};
		id = 0;
	}

	static void AddShortcut(const Key::Combination& combination, const std::function<void()>& callback) {
		if (auto b = combination.modifiers.cbegin(), e = combination.modifiers.cend();
			InsertCombination(combinationTree(), &b, &e, combination.key, callback))
			return;
		Log::For<Input>().Error("Shortcut is already taken.");
	}


	static void ProcessInput() {
		FillAxes();

		continuousInputOneSecondDelay().Process(io()->AnyKeyPressed);
		continuousInputNoDelay().Process(io()->AnyKeyPressed);

		// Make sure printable characters don't trigger combinations
		// while keyboard is captured by text input
		if (io()->AnyKeyPressed)
			ExecuteFirstMatchingCombination(combinationTree(), io()->WantCaptureKeyboard);

		// Handle OnInput actions
		for (auto& [id,handler] : handlers())
			handler();
	}

	static bool Init() {
		isRawMouseMotionSupported() = glfwRawMouseMotionSupported();

		return true;
	}

	static const glm::vec3 GetRelativeRotation(const glm::vec3& defaultAngle) {
		static glm::vec3 zero = glm::vec3();

		if (!Input::IsPressed(Key::Modifier::Control) || Input::movement() == zero)
			return defaultAngle;

		// If all 3 axes are modified then don't apply such rotation.
		// Quaternion can't rotate around 3 axes.
		if (Input::movement().x && Input::movement().y && Input::movement().z)
			return defaultAngle;

		auto mouseThresholdMin = 0.8;
		auto mouseThresholdMax = 1.25;
		auto mouseAxe = Input::MouseAxe();
		auto maxAxe = 0;

		// Find non-zero axes
		short t[2] = { 0,0 };
		short n = 0;
		for (short i = 0; i < 3; i++)
			if (mouseAxe[i] != 0)
				t[n] = i;

		// Nullify weak axes (those with small values)
		auto ratio = abs(mouseAxe[t[0]]) / abs(mouseAxe[t[1]]);
		if (ratio < mouseThresholdMin)
			mouseAxe[t[0]] = 0;
		else if (ratio > mouseThresholdMax)
			mouseAxe[t[1]] = 0;
		// If 2 axes are used simultaneously it breaks the quaternion somehow.
		else
			mouseAxe = zero;

		mouseAxe *= Settings::MouseSensivity().Get() * Input::MouseSpeed();

		auto na = (Input::ArrowAxe() + Input::NumpadAxe() + mouseAxe) * Settings::RotationStep().Get();
		return na;
	}
	static const glm::vec3 GetRelativeMovement(const glm::vec3& defaultMovement) {
		static glm::vec3 zero = glm::vec3();

		if (!Input::IsPressed(Key::Modifier::Alt) || Input::movement() == zero)
			return defaultMovement;

		return Input::movement() * Settings::TranslationStep().Get();
	}
	static const float GetNewScale(const float& currentScale) {
		static glm::vec3 zero = glm::vec3();

		if (!Input::IsPressed(Key::Modifier::Shift) || Input::movement() == zero)
			return currentScale;

		return currentScale + Input::movement().x * Settings::ScalingStep().Get();
	}

};

class KeyBinding {
	static void ResetFocus() {
		Input::AddHandler([] {
			if (Input::IsDown(Key::Escape))
				ImGui::FocusWindow(NULL);
			});
	}
	static void BindInputToMovement() {
		Input::AddHandler([] {
			// Enable or disable Mouse boundless mode 
			// and reset movement
			bool mouseBoundlessMode = Input::IsPressed(Key::Modifier::Alt) || Input::IsPressed(Key::Modifier::Shift) || Input::IsPressed(Key::Modifier::Control);
			Input::SetMouseBoundlessMode(mouseBoundlessMode && Input::IsCustomRenderImageActive().Get());

			Input::movement() = glm::vec3();

			Input::movement() += Input::MouseAxe() * Settings::MouseSensivity().Get();
			Input::movement() += Input::ArrowAxe();
			Input::movement() += Input::NumpadAxe();
			});
		Input::AddHandler([] {
			if (Input::IsPressed(Key::Modifier::Alt)) {
				if (Input::IsDown(Key::N5, true) && !ObjectSelection::Selected().empty()) {
					glm::vec3 v(0);
					for (auto& o : ObjectSelection::Selected())
						v += o.Get()->GetWorldPosition();
					v /= ObjectSelection::Selected().size();
					Scene::cross()->SetWorldPosition(v);
				}
				else if (Input::IsDown(Key::N0, true))
					Scene::cross()->SetWorldPosition(glm::vec3());
			}
			else if (Input::IsPressed(Key::Modifier::Control)) {
				if (Input::IsDown(Key::N5, true) && !ObjectSelection::Selected().empty())
					Scene::cross()->SetWorldRotation(ObjectSelection::Selected().begin()._Ptr->_Myval->GetWorldRotation());
				else if (Input::IsDown(Key::N0, true))
					Scene::cross()->SetWorldRotation(glm::quat(1, 0, 0, 0));
			}
			});
	}
	static void Cross() {
		// Resize cross.
		Input::AddHandler([&sp = crossScaleSpeed(), &min = crossMinSize(), &max = crossMaxSize()] {
			bool isScaleUp = Input::IsPressed(Key::N3, true);
			bool isScaleDown = Input::IsPressed(Key::N7, true);

			if (isScaleUp == isScaleDown)
				return;

			if (isScaleUp) {
				float newSize = Scene::cross()->size += sp;
				if (max < newSize)
					Scene::cross()->size = max;
				else
					Scene::cross()->size = newSize;
			}
			else {
				float newSize = Scene::cross()->size -= sp;
				if (newSize < min)
					Scene::cross()->size = min;
				else
					Scene::cross()->size = newSize;
			}

			Scene::cross()->ForceUpdateCache();
			});
	}

public:
	StaticFieldDefault(float, crossScaleSpeed, 1)
	StaticFieldDefault(float, crossMinSize, .1f)
	StaticFieldDefault(float, crossMaxSize, 100)

	static bool Init() {
		ResetFocus();
		BindInputToMovement();
		Cross();

		return true;
	}
};

