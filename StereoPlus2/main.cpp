#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "ToolPool.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include "Renderer.hpp"
#include <functional>
#include "FileManager.hpp"
#include <filesystem> // C++17 standard header file name
#include <chrono>
#include "PositionDetection.hpp"
using namespace std;

bool CustomRenderFunc(Scene& scene, Renderer& renderPipeline, PositionDetector& positionDetector) {
	// Modify camera posiiton when Posiiton detection is enabled.
	if (positionDetector.isPositionProcessingWorking)
		scene.camera->SetLocalPosition(
			glm::vec3(
				positionDetector.positionHorizontal / 100.0,
				positionDetector.positionVertical / 100.0,
				-positionDetector.distance / 50.0));

	// Run scene drawing.
	renderPipeline.Pipeline(scene);
	
	return true;
}

int main(int, char**) {
	// Declare main components.
	PositionDetector positionDetector;

	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow cameraPropertiesWindow;
	SceneObjectPropertiesWindow crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;

	AttributesWindow attributesWindow;
	ToolWindow toolWindow;

	Scene scene;
	StereoCamera camera;
	Renderer renderPipeline;
	GUI gui;
	Cross cross;

	// Initialize main components.
	toolWindow.attributesWindow = &attributesWindow;
	toolWindow.scene = &scene;

	inspectorWindow.rootObject = (GroupObject**)&scene.root;
	inspectorWindow.selectedObjectsBuffer = &scene.selectedObjects;

	scene.camera = &camera;
	cameraPropertiesWindow.Object = (SceneObject*)scene.camera;

	if (!renderPipeline.Init())
		return false;

	gui.windows = {
		(Window*)&customRenderWindow,
		(Window*)&cameraPropertiesWindow,
		(Window*)&crossPropertiesWindow,
		(Window*)&inspectorWindow,
		(Window*)&attributesWindow,
		(Window*)&toolWindow,
	};

	gui.glWindow = renderPipeline.glWindow;
	gui.glsl_version = renderPipeline.glsl_version;

	scene.glWindow = gui.glWindow;
	scene.camera->viewSize = &customRenderWindow.renderSize;
	scene.cross = &cross;

	gui.scene = &scene;

	cross.Name = "Cross";
	if (!cross.Init())
		return false;

	crossPropertiesWindow.Object = (SceneObject*)scene.cross;
	gui.keyBinding.cross = &cross;
	if (!gui.Init())
		return false;

	cross.keyboardBindingHandler = [&cross, i = &gui.input]() { cross.SetLocalPosition(cross.GetLocalPosition() + i->movement); };
	cross.keyboardBindingHandlerId = gui.keyBinding.AddHandler(cross.keyboardBindingHandler);

	* ToolPool::GetCross() = &cross;
	* ToolPool::GetScene() = &scene;
	* ToolPool::GetKeyBinding() = &gui.keyBinding;

	if (!ToolPool::Init())
		return false;

	// Position detector doesn't initialize itself
	// so we need to help it.
	positionDetector.onStartProcess = [&positionDetector] {
		positionDetector.Init();
	};

	// Track the state of Position detector to switch it
	// when necessary. 
	// Reads user position and modifies camera position when enabled.
	// Calls drawing methods of Renderer.
	customRenderWindow.customRenderFunc = [&scene, &renderPipeline, shouldUsePositionDetection = &gui.shouldUsePositionDetection, &positionDetector]{
		if (*shouldUsePositionDetection && !positionDetector.isPositionProcessingWorking)
			positionDetector.StartPositionDetection();
		else if (!*shouldUsePositionDetection && positionDetector.isPositionProcessingWorking)
			positionDetector.StopPositionDetection();

		return CustomRenderFunc(scene, renderPipeline, positionDetector);
	};

	// Start the main loop and clean the memory when closed.
	if (!gui.MainLoop() |
		!gui.OnExit()) {
		positionDetector.StopPositionDetection();
		return false;
	}

	// Stop Position detection thread.
	positionDetector.StopPositionDetection();
    return true;
}
