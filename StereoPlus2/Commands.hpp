#pragma once

#include "GLLoader.hpp"
#include "DomainUtils.hpp"
#include "InfrastructureTypes.hpp"
#include <list>
#include <set>




class CreateCommand : Command {
protected:
	virtual bool Execute() {
		SceneObject* o;
		if (destination == nullptr)
			Scene::Insert(o = init());
		else
			Scene::Insert(destination, o = init());

		onCreated(o);
		return true;
	};
public:
	SceneObject* destination = nullptr;
	std::function<SceneObject*()> init;
	std::function<void(SceneObject*)> onCreated = [] (SceneObject*) {};

	CreateCommand() {
		isReady = true;
	}
};

class DeleteCommand : Command {
protected:
	virtual bool Execute() {
		Scene::Delete(source, target);
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
	std::set<PON>* items;
	InsertPosition pos;
	IHolder* caller;
	std::function<void()> callback = [] {};
};
