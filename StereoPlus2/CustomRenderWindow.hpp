#pragma once
#include "Window.hpp"
#include <functional>

class CustomRenderWindow : Window
{
public:
	std::function<bool()> customRenderFunc;

	virtual bool Init()
	{
		return true;
	}

	virtual bool Design()
	{
		return true;
	}

	virtual bool OnExit()
	{
		return true;
	}
};