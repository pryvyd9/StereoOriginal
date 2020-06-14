#pragma once

#include "InfrastructureTypes.hpp"
#include "DomainTypes.hpp"

class INameHolder {
protected:
	std::string name;
};

class Window : public INameHolder
{
protected:
	bool shouldClose = false;
	Event<> onExit;
public:
	IEvent<>& OnExit() {
		return onExit;
	}

	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool Exit() {
		onExit.Invoke();
		return true;
	}
	// Indicates if the window should be closed.
	bool ShouldClose() const {
		return shouldClose;
	}
};

class Attributes : public INameHolder
{
protected:
	bool isInitialized;
public:
	bool IsInitialized() const {
		return isInitialized;
	}
	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool OnExit() = 0;
	virtual void UnbindTargets() = 0;
	virtual SceneObject* GetTarget() = 0;
	virtual void BindTarget(SceneObject* o) {}
};