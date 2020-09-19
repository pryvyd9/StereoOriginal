#include "GLLoader.hpp"
#include "DomainUtils.hpp"
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
				positionDetector.positionHorizontal / 500.0,
				positionDetector.positionVertical / 500.0,
				-positionDetector.distance / 50.0));

	// Run scene drawing.
	renderPipeline.Pipeline(scene);
	
	return true;
}

int main(int, char**) {
	Property<int> a, b;

	a.BindTwoWay(b);
	a = 1;

	a = 2;
	a = 3;

	// Declare main components.
	PositionDetector positionDetector;

	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow cameraPropertiesWindow;
	SceneObjectPropertiesWindow crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;

	AttributesWindow attributesWindow;
	ToolWindow toolWindow;

	Renderer renderPipeline;
	GUI gui;

	if (!renderPipeline.Init())
		return false;

	Scene scene;
	StereoCamera camera;
	Cross cross;

	// Initialize main components.
	toolWindow.attributesWindow = &attributesWindow;
	toolWindow.scene = &scene;

	inspectorWindow.rootObject = (GroupObject**)&scene.root.Get();

	scene.camera = &camera;
	cameraPropertiesWindow.Object = (SceneObject*)scene.camera;

	

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

	inspectorWindow.input = &gui.input;

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

	StateBuffer::BufferSize() = 30;
	StateBuffer::RootObject().BindTwoWay(scene.root);
	StateBuffer::RootObject() = scene.root.Get();
	StateBuffer::Objects() = &scene.objects;
	if (!StateBuffer::Init())
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
	customRenderWindow.customRenderFunc = [&scene, &renderPipeline, &positionDetector]{
		if (GlobalToolConfiguration::ShouldDetectPosition().Get() && !positionDetector.isPositionProcessingWorking)
			positionDetector.StartPositionDetection();
		else if (!GlobalToolConfiguration::ShouldDetectPosition().Get() && positionDetector.isPositionProcessingWorking)
			positionDetector.StopPositionDetection();
		 
		return CustomRenderFunc(scene, renderPipeline, positionDetector);
	};

	gui.keyBinding.AddHandler([i = &gui.input] {
		if (i->IsPressed(Key::ControlLeft) || i->IsPressed(Key::ControlRight)) {
			if (i->IsDown(Key::Z))
				StateBuffer::Rollback();
			else if (i->IsDown(Key::Y))
				StateBuffer::Repeat();
		}
		});

	gui.keyBinding.AddHandler([i = &gui.input, s = &scene]{
		if (i->IsDown(Key::Delete)) {
			StateBuffer::Commit();
			s->DeleteSelected();
			}
		});


	// Start the main loop and clean the memory when closed.
	if (!gui.MainLoop() |
		!gui.OnExit()) {
		positionDetector.StopPositionDetection();
		return false;
	}

	// Stop Position detection thread.
	positionDetector.StopPositionDetection();
	StateBuffer::Clear();
    return true;
}
