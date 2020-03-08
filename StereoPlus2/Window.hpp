#pragma once

class Window
{
protected:
	std::string name;
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

class Attributes
{
protected:
	bool isInitialized;
	std::string name;
public:
	bool IsInitialized() const {
		return isInitialized;
	}
	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool OnExit() = 0;
};