#pragma once

#include "GLLoader.hpp"
#include "DomainUtils.hpp"
#include "InfrastructureTypes.hpp"
#include <list>
#include <set>




class ISceneHolder {
protected:
	Scene* scene = nullptr;
	bool CheckScene() {
		if (scene == nullptr) {
			std::cout << "Scene wasn't bind" << std::endl;
			return false;
		}
		return true;
	}
public:
	bool BindScene(Scene* scene) {
		this->scene = scene;
		return true;
	}
};

class CreateCommand : Command, public ISceneHolder {
protected:
	virtual bool Execute() {
		if (!CheckScene())
			return false;

		if (destination == nullptr)
			scene->Insert(func());
		else
			scene->Insert(destination, func());

		return true;
	};
public:
	SceneObject* destination = nullptr;
	std::function<SceneObject* ()> func;

	CreateCommand() {
		isReady = true;
	}
};

class DeleteCommand : Command, public ISceneHolder {
protected:
	virtual bool Execute() {
		if (!CheckScene())
			return false;

		scene->Delete(source, target);
		return true;
	};
public:
	SceneObject* source;
	SceneObject* target;

	DeleteCommand() {
		isReady = true;
	}
};



class MoveCommand : Command {
protected:
	virtual bool Execute() {
		bool res = Scene::MoveTo(static_cast<GroupObject*>(target), targetPos, items, pos);

		items->clear();
		caller->isCommandEmpty = true;

		callback();

		return res;
	};
public:
	class IHolder {
	public:
		bool isCommandEmpty = true;
	};

	void SetReady() {
		isReady = true;
	}

	bool GetReady() {
		return isReady;
	}

	SceneObject* target;
	int targetPos;
	std::set<SceneObject*>* items;
	InsertPosition pos;
	IHolder* caller;
	std::function<void()> callback = [] {};
};
