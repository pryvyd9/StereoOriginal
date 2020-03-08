#pragma once

class Window
{
protected:
	std::string name;
	bool shouldClose = false;
public:
	bool ShouldClose() {
		return shouldClose;
	}
	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool OnExit() = 0;
};

class Attributes
{
protected:
	bool isInitialized;
	std::string name;
public:
	bool IsInitialized() {
		return isInitialized;
	}
	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool OnExit() = 0;
};