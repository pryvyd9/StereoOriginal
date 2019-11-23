#pragma once
class Window
{
public:
	virtual bool Init() = 0;
	virtual bool Design() = 0;
	virtual bool OnExit() = 0;
};