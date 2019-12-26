#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include "Input.hpp"
#include <map>
#include <stack>



class Tool {
protected:
	struct Change {
		SceneObject* reference, * stateCopy;
	};
	static std::stack<Change>& GetChanges() {
		static auto val = std::stack<Change>();
		return val;
	}
public:
	virtual bool Rollback() {
		if (GetChanges().size() <= 0)
			return true;
		
		auto change = GetChanges().top();
		change.reference = change.stateCopy;
		
		return true;
	}
};

template<typename T>
class CreatingTool : Tool, public ISceneHolder {

	struct Pointer {
		int objectCreationId;
		SceneObject* obj;
	};

	T* obj;
	std::vector<SceneObject*>* source;
	int currentCreatedId;

	static int& GetNextFreeId() {
		static int val = 0;
		return val;
	}

	static std::map<int, SceneObject*>& GetCreatedObjects() {
		static auto val = std::map<int, SceneObject*>();
		return val;
	}
public:
	std::function<bool(SceneObject*)> initFunc;

	bool BindSource(std::vector<SceneObject*>* source) {
		this->source = source;
		return true;
	}
	bool Create(SceneObject*** createdObj) {
		currentCreatedId = GetNextFreeId()++;
		GetCreatedObjects()[currentCreatedId] = nullptr;

		// If passed nullptr then we should ignore created object.
		if (createdObj != nullptr)
			*createdObj = &GetCreatedObjects()[currentCreatedId];

		auto command = new CreateCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;

		command->func = [=] {
			T* obj = new T();
			initFunc(obj);
			GetCreatedObjects()[currentCreatedId] = obj;
			return obj;
		};

		return true;
	}
	virtual bool Rollback() {
		auto command = new DeleteCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;
		command->target = &GetCreatedObjects()[currentCreatedId];

		return true;
	}

};


class EditingTool : Tool {
	KeyBinding* keyBinding = nullptr;
public:
	virtual bool SelectTarget(SceneObject* obj) = 0;
	virtual bool ReleaseTarget() = 0;
	
	virtual bool BindInput(KeyBinding* keyBinding) {
		this->keyBinding = keyBinding;
		return true;
	}
};

template<typename T>
class PointPenEditingTool : EditingTool {
	SceneObject* target = nullptr;
	std::vector<glm::vec3> pointsSelected;
	std::vector<glm::vec3*> selectedPoints;


	template<typename T>
	bool SelectPoint() {
		std::cout << "Not Supported PointPenEditingTool template type" << std::endl;
		return false;
	}

	template<>
	bool SelectPoint<StereoPolyLine>() {
		if (selectedPoints.size() > 0)
			selectedPoints.clear();

		selectedPoints.push_back(&((StereoPolyLine*)target)->Points.back());
		return true;
	}

public:
	virtual bool SelectTarget(SceneObject* obj) {
		target = (T*)obj;
		return true;
	}
	virtual bool ReleaseTarget() {
		target = nullptr;
		return true;
	}
	virtual bool BindInput(KeyBinding* keyBinding) {
		//keyBinding->AddHandler([](Input * input) {
		//	if (input->IsPressed(GLFW_MOUSE_BUTTON_))
		//	//if (input->IsPressed(GLFW_MOUSE_BUTTON_LEFT))
		//	{
		//		int i = 0;
		//	}
		//});
		
		return EditingTool::BindInput(keyBinding);
	}



	bool SelectPoint() {
		return SelectPoint<T>();
	}

	bool AddPoint() {
		return true;
	}
	
};





template<typename T>
class DrawingInstrument {
public:
	SceneObject* obj = nullptr;

	bool Start() {

	}

	void ApplyObject(SceneObject* obj) {
		this->obj = obj;
	}

	void ApplyPosition(glm::vec3 pos) {

	}

	void ConfirmPoint() {
		//obj->Points.push_back(pointPos);
	}

	void RemoveLastPoint() {
		//obj->Points.pop_back();
	}

	void Finish() {

	}

	void Abort() {
		/*	obj->Points.clear();

			for (auto p : originalPoints)
				obj->Points.push_back(p);*/
	}
};

