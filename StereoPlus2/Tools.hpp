#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include <map>


class Tool {
public:
	virtual bool Rollback() = 0;
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

		if (createdObj != nullptr)
		{
			*createdObj = &GetCreatedObjects()[currentCreatedId];
		}

		auto command = new CreateCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;

		int id = currentCreatedId;
		auto initFunc = this->initFunc;

		command->func = [id, initFunc] {
			T* obj = new T();
			initFunc(obj);
			GetCreatedObjects()[id] = obj;
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

public:

	virtual bool SelectTarget(SceneObject* obj) = 0;
	virtual bool ReleaseTarget() = 0;
	virtual bool Rollback() = 0;
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

//template<>
//class DrawingInstrument<StereoPolygonalChain>{
//	std::vector<glm::vec3> originalPoints;
//
//	StereoPolygonalChain* obj;
//
//	glm::vec3 pointPos;
//
//public:
//
//	void Start() {
//
//	}
//
//	void ApplyObject(StereoPolygonalChain* obj) {
//		originalPoints.clear();
//
//		for (auto p : obj->Points)
//			originalPoints.push_back(p);
//
//		this->obj = obj;
//	}
//
//	void ApplyPosition(glm::vec3 pos) {
//		pointPos = pos;
//		obj->Points.data[obj->Points.size() - 1] = pos;
//	}
//
//	void ConfirmPoint() {
//		obj->Points.push_back(pointPos);
//	}
//
//	void RemoveLastPoint() {
//		obj->Points.pop_back();
//	}
//
//	void Finish() {
//
//	}
//
//	void Abort() {
//		obj->Points.clear();
//
//		for (auto p : originalPoints)
//			obj->Points.push_back(p);
//	}
//};

//class StereoPolygonalChain
