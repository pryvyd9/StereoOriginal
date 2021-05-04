#pragma once
#include "DomainTypes.hpp"
#include "Tools.hpp"
#include "Input.hpp"
#include <sstream>

class ToolPool {
	static void Init(PenTool* tool) {
		tool->cross <<= Cross();
		tool->keyBinding <<= KeyBinding();
	}

	static void Init(SinePenTool* tool) {
		tool->cross <<= Cross();
		tool->keyBinding <<= KeyBinding();
	}

	static void Init(ExtrusionEditingTool<PolyLineT>* tool) {
		tool->destination <<= Scene()->root();
		tool->cross <<= Cross();
		tool->keyBinding <<= KeyBinding();
	}

	static void Init(TransformTool* tool) {
		tool->cross <<= Cross();
		tool->keyBinding <<= KeyBinding();
	}

public:
	StaticProperty(::Scene*, Scene);
	StaticProperty(::Cross*, Cross);
	StaticProperty(::KeyBinding*, KeyBinding);

	template<typename T>
	static T* GetTool() {
		static T tool;
		static bool isInitialized = false;

		if (!isInitialized) {
			Init(&tool);
			isInitialized = true;
		}
		
		return &tool;
	}

	static bool Init() {
		if (KeyBinding().IsAssigned() && Scene().IsAssigned() && Cross().IsAssigned())
			return true;

		std::cout << "ToolPool could not be initialized because some fields were null" << std::endl;
		return false;
	}
};
