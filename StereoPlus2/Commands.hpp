#pragma once

#include "GLLoader.hpp"
#include "DomainUtils.hpp"
#include "InfrastructureTypes.hpp"
#include <list>
#include <set>

// Recurrent timer
class TimerCommand : Command {
	bool shouldAbort = false;
	size_t timestampStarted;
	
	// Max value
	size_t periodMilliseconds = (size_t)-1;
protected:
	virtual bool Execute() {
		if (auto now = Time::GetTime(); !shouldAbort && now - timestampStarted >= periodMilliseconds) {
			timestampStarted = now;
			func();
		}

		return true;
	};
public:
	TimerCommand() {
		mustPersist = true;
	}
	void Start(size_t periodMinutes) {
		isReady = true;
		periodMilliseconds = periodMinutes * 60 * 1000;
		timestampStarted = Time::GetTime();
	}
	void Abort() {
		shouldAbort = true;
		mustPersist = false;
	}
	std::function<void()> func;
};

// Singleton for autosaving feature
class AutosaveCommand : TimerCommand {
	static AutosaveCommand*& GetCommand() {
		static AutosaveCommand* cmd = nullptr;
		return cmd;
	};
public:
	void StartNew(size_t periodMinutes) {
		if (GetCommand())
			GetCommand()->Abort();

		GetCommand() = this;
		TimerCommand::Start(periodMinutes);
	}

	void SetFunc(std::function<void()> func) {
		TimerCommand::func = func;
	}

	static void Abort() {
		if (GetCommand())
			GetCommand()->TimerCommand::Abort();
	}

	static std::string GetFileName() {
		std::stringstream ss;
		ss << "scenes/" << "~temp" << Time::GetAppStartTime() << ".so2";
		return ss.str();
	}

	// If there was an error save with a different name
	// in case it's corrupted.
	static std::string GetBackupFileName() {
		std::stringstream ss;
		ss << "scenes/" << "~backup" << Time::GetAppStartTime() << ".so2";
		return ss.str();
	}
};

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
