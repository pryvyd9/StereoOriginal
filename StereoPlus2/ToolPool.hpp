#pragma once
#include "DomainTypes.hpp"
#include "Tools.hpp"
#include "Input.hpp"
#include <sstream>

class ToolPool {
	static void Init(PenTool* tool) {
		tool->cross <<= Scene::cross();
	}

	static void Init(CosinePenTool* tool) {
		tool->cross <<= Scene::cross();
	}

	static void Init(PointPenTool* tool) {
		tool->cross <<= Scene::cross();
	}

	static void Init(ExtrusionEditingTool<PolyLineT>* tool) {
		tool->destination <<= Scene::root();
		tool->cross <<= Scene::cross();
	}

	static void Init(TransformTool* tool) {
		tool->cross <<= Scene::cross();
	}

public:
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
		if (Scene::root().IsAssigned() && Scene::cross().IsAssigned())
			return true;

		std::cout << "ToolPool could not be initialized because some fields were null" << std::endl;
		return false;
	}
};
