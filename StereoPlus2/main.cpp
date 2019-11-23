#include "GUI.hpp"
#include "CustomRenderWindow.hpp"

int main(int, char**)
{
	CustomRenderWindow customRenderWindow;
	
	GUI gui;
	gui.windows.push_back((Window*)&customRenderWindow);

	if (!gui.Init())
		return false;

	if (!gui.MainLoop())
		return false;

	if (!gui.OnExit())
		return false;


    return 0;
}
