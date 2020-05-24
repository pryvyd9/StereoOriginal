#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include <list>
#include <set>


class Command {
	static std::list<Command*>& GetQueue() {
		static auto queue = std::list<Command*>();
		return queue;
	}
protected:
	bool isReady = false;
	virtual bool Execute() = 0;
public:
	Command() {
		GetQueue().push_back(this);
	}
	static bool ExecuteAll() {
		std::list<Command*> deleteQueue;
		for (auto command : GetQueue())
			if (command->isReady) {
				if (!command->Execute())
					return false;

				deleteQueue.push_back(command);
			}

		for (auto command : deleteQueue) {
			GetQueue().remove(command);
			delete command;
		}

		return true;
	}
};

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
	SceneObject* destination;
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

class FuncCommand : Command {
protected:
	virtual bool Execute() {
		func();

		return true;
	};
public:
	FuncCommand() {
		isReady = true;
	}

	std::function<void()> func;
};

class MoveCommand : Command {
protected:
	virtual bool Execute() {
		bool res = Scene::MoveTo(static_cast<GroupObject*>(target), targetPos, items, pos);

		items->clear();
		caller->isCommandEmpty = true;

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
};
