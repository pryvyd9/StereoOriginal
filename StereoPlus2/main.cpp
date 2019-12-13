#include "GLLoader.hpp"
#include "DomainTypes.hpp"
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
	SceneObjectInspectorWindow inspectorWindow;

	Scene scene;

	StereoCamera camera;
	cameraPropertiesWindow.Object = &camera;
	scene.camera = &camera;

	if (!renderPipeline.Init())
		return false;

	gui.window = renderPipeline.window;
	gui.glsl_version = renderPipeline.glsl_version;

	scene.whiteZ = 0;
	scene.whiteZPrecision = 0.1;
	scene.window = gui.window;
	scene.camera->viewSize = &customRenderWindow.renderSize;

	gui.scene = &scene;

	Cross cross;
	cross.Name = "Cross";

	if (!cross.Init())
		return false;

	crossPropertiesWindow.Object = &cross;
	gui.keyBinding.cross = &cross;
	
	gui.windows.push_back((Window*)& customRenderWindow);
	gui.windows.push_back((Window*)& cameraPropertiesWindow);
	gui.windows.push_back((Window*)& crossPropertiesWindow);
	gui.windows.push_back((Window*)& inspectorWindow);

	if (false
		|| !gui.Init()
		|| !customRenderWindow.Init()
		|| !cameraPropertiesWindow.Init()
		|| !crossPropertiesWindow.Init()
		|| !inspectorWindow.Init())
		return false;

	customRenderWindow.customRenderFunc = [&cross, &scene, &gui, &renderPipeline] {
		renderPipeline.Pipeline(cross.lines, cross.lineCount, scene);

		return true;
	};

	if (!gui.MainLoop()
		|| !gui.OnExit())
		return false;

    return true;
}
