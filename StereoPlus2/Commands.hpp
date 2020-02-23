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
			if (command->isReady)
			{
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

		scene->Insert(source, func());
		return true;
	};
public:
	std::vector<SceneObject*>* source;
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
	std::vector<SceneObject*>* source;
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



enum MoveCommandPosition
{
	Top = 0x01,
	Bottom = 0x10,
	Center = 0x100,
	Any = Top | Bottom | Center,
};

class MoveCommand : Command {
protected:
	virtual bool Execute() {
		bool res = MoveTo(*target, targetPos, items, pos);

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
	static bool MoveTo(std::vector<SceneObject*>& target, int targetPos, std::set<ObjectPointer, ObjectPointerComparator>* items, MoveCommandPosition pos) {

		// Move single object
		if (items->size() > 1)
		{
			std::cout << "Moving of multiple objects is not implemented" << std::endl;
			return false;
		}

		// Find if item is present in target;

		auto pointer = items->begin()._Ptr->_Myval;
		auto item = (*pointer.source)[pointer.pos];

		if (target.size() == 0)
		{
			target.push_back(item);
			pointer.source->erase(pointer.source->begin() + pointer.pos);
			return true;
		}

		if ((pos & Bottom) == Bottom)
		{
			targetPos++;
		}

		if (pointer.source == &target && targetPos < pointer.pos)
		{
			target.erase(target.begin() + pointer.pos);
			target.insert(target.begin() + targetPos, (const size_t)1, item);

			return true;
		}

		target.insert(target.begin() + targetPos, (const size_t)1, item);
		pointer.source->erase(pointer.source->begin() + pointer.pos);

		return true;
	}

	std::vector<SceneObject*>* target;
	int targetPos;
	std::set<ObjectPointer, ObjectPointerComparator>* items;
	MoveCommandPosition pos;
	IHolder* caller;
};
