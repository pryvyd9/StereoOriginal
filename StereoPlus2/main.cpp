#include "GUI.hpp"


int main(int, char**)
{
	GUI gui;

	if (!gui.Init())
		return false;

	if (!gui.MainLoop())
		return false;

	if (!gui.OnExit())
		return false;

    return 0;
}
