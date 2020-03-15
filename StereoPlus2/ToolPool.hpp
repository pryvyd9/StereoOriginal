#pragma once
#include "DomainTypes.hpp"
#include "Tools.hpp"
#include "Input.hpp"
#include <sstream>

class ToolPool {

	template<typename T>
	static int GetId() {
		static int i = 0;
		return i++;
	}

	template<typename T>
	static bool Init(T* tool) {
		std::cout << "Tool type not supported and could not be initialized" << std::endl;
		return false;
	}

	template<>
	static bool Init<PointPenEditingTool<StereoPolyLineT>>(PointPenEditingTool<StereoPolyLineT>* tool) {
		return 
			tool->BindInput(*GetKeyBinding()) &&
			tool->BindCross(*GetCross());
	}

	template<>
	static bool Init<ExtrusionEditingTool<StereoPolyLineT>>(ExtrusionEditingTool<StereoPolyLineT>* tool) {

		if (!tool->BindInput(*GetKeyBinding()) ||
			!tool->BindCross(*GetCross()) ||
			!tool->BindScene(*GetScene()) ||
			!tool->BindSource(&(*GetScene())->root->Children))
			return false;

		return true;
	}
	template<>
	static bool Init<TransformTool>(TransformTool* tool) {
		return
			tool->BindInput(*GetKeyBinding()) &&
			tool->BindCross(*GetCross());
	}

	/*template<>
	static bool Init<TransformTool<LineMeshT>>(TransformTool<LineMeshT>* tool) {
		return
			tool->BindInput(*GetKeyBinding()) &&
			tool->BindCross(*GetCross());
	}*/



public:
	static KeyBinding** GetKeyBinding() {
		static KeyBinding* val = nullptr;
		return &val;
	}
	static Scene** GetScene() {
		static Scene* val = nullptr;
		return &val;
	}
	static Cross** GetCross() {
		static Cross* val = nullptr;
		return &val;
	}


	template<typename T>
	static T* GetTool() {
		static T tool;
		static bool isInitialized = false;

		if (isInitialized || (isInitialized = Init(&tool)))
			return &tool;

		std::cout << "Tool could not be initialized because some fields were null" << std::endl;
		return nullptr;
	}

	static bool Init() {
		if (*GetKeyBinding() && *GetScene() && *GetCross())
			return true;

		std::cout << "ToolPool could not be initialized because some fields were null" << std::endl;
		return false;
	}
};
