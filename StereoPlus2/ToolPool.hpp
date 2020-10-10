#pragma once
#include "DomainTypes.hpp"
#include "Tools.hpp"
#include "Input.hpp"
#include <sstream>

class ToolPool {
	static void Init(PointPenEditingTool<StereoPolyLineT>* tool) {
		tool->cross.BindAndApply(Cross());
		tool->keyBinding.BindAndApply(KeyBinding());
	}

	static void Init(ExtrusionEditingTool<StereoPolyLineT>* tool) {
		tool->destination.BindAndApply(Scene().Get()->root);
		tool->scene.BindAndApply(Scene());
		tool->cross.BindAndApply(Cross());
		tool->keyBinding.BindAndApply(KeyBinding());
	}

	static void Init(TransformTool* tool) {
		tool->cross.BindAndApply(Cross());
		tool->keyBinding.BindAndApply(KeyBinding());
	}

public:
	static ReadonlyProperty<::Scene*>& Scene() {
		static ReadonlyProperty<::Scene*> v;
		return v;
	}
	static ReadonlyProperty<::Cross*>& Cross() {
		static ReadonlyProperty<::Cross*> v;
		return v;
	}
	static ReadonlyProperty<::KeyBinding*>& KeyBinding() {
		static ReadonlyProperty<::KeyBinding*> v;
		return v;
	}

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
		if (KeyBinding().Get() && Scene().Get() && Cross().Get())
			return true;

		std::cout << "ToolPool could not be initialized because some fields were null" << std::endl;
		return false;
	}
};
