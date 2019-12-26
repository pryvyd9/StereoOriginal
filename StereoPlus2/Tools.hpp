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

template<ObjectType type>
class PointPenEditingTool : EditingTool {
	Cross* cross = nullptr;
	SceneObject* target = nullptr;

	//std::vector<glm::vec3> pointsSelected;
	std::vector<glm::vec3*> existingPointsSelected;

	/*template<typename T>
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
	}*/

	template<ObjectType type>
	void ProcessInput(Input* input) {
		std::cout << "Unsupported Editing Tool target Type" << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT>(Input* input) {
		if (input->IsPressed(Key::Enter))
		{
			auto selectedPointCount = existingPointsSelected.size();

			if (existingPointsSelected.size() == 0)
			{
				SelectPoint<StereoPolyLine>(input);
			}

			AddPoint<StereoPolyLine>(input);

			int i = 0;

			return;
		}

		if (existingPointsSelected.size() > 1)
		{
			*existingPointsSelected.back() = cross->Position;
		}
		
	}

	template<typename T>
	void SelectPoint(Input* input) {
		std::cout << "Unsupported Editing Tool target Type" << std::endl;
	}
	template<>
	void SelectPoint<StereoPolyLine>(Input* input) {
		auto target = (StereoPolyLine*)this->target;

		auto crossViewSurfacePos = glm::vec2(cross->Position.x, cross->Position.y);

		auto closestPoint = &target->Points[0];
		auto viewSurfacePos = *(glm::vec2*)closestPoint;
		double minDistance = glm::distance(crossViewSurfacePos, viewSurfacePos);

		for (size_t i = 1; i < target->Points.size(); i++)
		{
			viewSurfacePos = *(glm::vec2*)&target->Points[i];
			auto currentDistance = glm::distance(crossViewSurfacePos, viewSurfacePos);
			if (minDistance > currentDistance)
				closestPoint = &target->Points[0];
		}

		existingPointsSelected.push_back(closestPoint);
	}

	template<typename T>
	void AddPoint(Input* input) {
		std::cout << "Unsupported Editing Tool target Type" << std::endl;
	}
	template<>
	void AddPoint<StereoPolyLine>(Input* input) {
		auto target = (StereoPolyLine*)this->target;
		auto h = &target->Points.back();

		target->Points.push_back(cross->Position);

		existingPointsSelected.push_back(&target->Points.back());
	}

public:
	virtual bool SelectTarget(SceneObject* obj) {
		if (obj->GetType() != type)
		{
			std::cout << "Invalid Object passed to PointPenEditingTool" << std::endl;
			return true;
		}
		target = obj;
		return true;
	}
	virtual bool ReleaseTarget() {
		target = nullptr;
		return true;
	}
	virtual bool BindInput(KeyBinding* keyBinding) {
		keyBinding->AddHandler([this](Input * input) { this->ProcessInput<type>(input); });

		return EditingTool::BindInput(keyBinding);
	}
	bool BindCross(Cross* cross) {
		this->cross = cross;
		return true;
	}
	/*bool BindSceneObjects(std::vector<SceneObject*>* sceneObjects) {
		this->sceneObjects = sceneObjects;
		return true;
	}*/

	/*bool SelectPoint() {
		return SelectPoint<T>();
	}*/

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

