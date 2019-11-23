#pragma once
#include "Window.hpp"

class CustomRenderWindow : Window
{
public:
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