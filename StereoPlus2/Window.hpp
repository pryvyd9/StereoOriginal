#pragma once

#include "Commands.hpp"
//
//class ISceneHolder {
//protected:
//	Scene* scene;
//public:
//	virtual bool BindScene(Scene* scene) {
//		this->scene = scene;
//	}
//};
class INameHolder {
protected:
	std::string name;
};

class Window : public ISceneHolder, public INameHolder
{
protected:
	bool shouldClose = false;
	std::vector<std::function<void()>> onExit;
public:
	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool OnExit() {
		for (auto f : onExit)
			f();
		
		return true;
	}
	virtual bool BindOnExit(std::function<void()> f) {
		onExit.push_back(f);
		return true;
	}
	// Indicates if the window should be closed.
	bool ShouldClose() const {
		return shouldClose;
	}
};

class Attributes : public ISceneHolder, public INameHolder
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
};