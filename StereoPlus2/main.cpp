#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "SceneConfig.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include "Renderer.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include <functional>

using namespace std;






int main(int, char**)
{
	Renderer renderPipeline;

	GUI gui;

	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow<StereoCamera> cameraPropertiesWindow;
	SceneObjectPropertiesWindow<Cross> crossPropertiesWindow;

	//Scene scene;

	if (!renderPipeline.Init())
		return false;

	gui.window = renderPipeline.window;
	gui.glsl_version = renderPipeline.glsl_version;

	SceneConfiguration config;
	config.whiteZ = 0;
	config.whiteZPrecision = 0.1;
	config.window = gui.window;
	config.camera.viewSize = &customRenderWindow.renderSize;

	cameraPropertiesWindow.Object = &config.camera;
	gui.sceneConfig = &config;

	Cross cross;
	cross.Name = "Cross";

	if (!cross.Init())
		return false;

	crossPropertiesWindow.Object = &cross;
	gui.keyBinding.cross = &cross;
	
	gui.windows.push_back((Window*)& customRenderWindow);
	gui.windows.push_back((Window*)& cameraPropertiesWindow);
	gui.windows.push_back((Window*)& crossPropertiesWindow);

	if (false
		|| !gui.Init()
		|| !customRenderWindow.Init()
		|| !cameraPropertiesWindow.Init()
		|| !crossPropertiesWindow.Init())
		return false;

	customRenderWindow.customRenderFunc = [&cross, &config, &gui, &renderPipeline] {
		renderPipeline.Pipeline(cross.lines, cross.lineCount, config);

		return true;
	};

	if (!gui.MainLoop()
		|| !gui.OnExit())
		return false;

    return true;
}
