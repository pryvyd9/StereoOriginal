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
#include "SettingsLoader.hpp"
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
	// Declare main components.
	PositionDetector positionDetector;

	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow cameraPropertiesWindow;
	SceneObjectPropertiesWindow crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;
	AttributesWindow attributesWindow;
	ToolWindow toolWindow;
	SettingsWindow settingsWindow;

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

	inspectorWindow.rootObject.BindAndApply(scene.root);
	inspectorWindow.input = &gui.input;

	cameraPropertiesWindow.Object = &camera;
	crossPropertiesWindow.Object = &cross;

	scene.camera = &camera;
	scene.glWindow = renderPipeline.glWindow;
	scene.camera->viewSize = &customRenderWindow.renderSize;
	scene.cross = &cross;

	gui.windows = {
		(Window*)&customRenderWindow,
		(Window*)&cameraPropertiesWindow,
		(Window*)&crossPropertiesWindow,
		(Window*)&inspectorWindow,
		(Window*)&attributesWindow,
		(Window*)&toolWindow,
		(Window*)&settingsWindow,
	};
	gui.glWindow = renderPipeline.glWindow;
	gui.glsl_version = renderPipeline.glsl_version;
	gui.scene = &scene;
	gui.keyBinding.cross = &cross;
	gui.settingsWindow = &settingsWindow;
	if (!gui.Init())
		return false;

	cross.Name = "Cross";
	cross.GUIPositionEditHandler = [&cross, i = &gui.input]() { i->movement += cross.GUIPositionEditDifference; };
	cross.GUIPositionEditHandlerId = gui.keyBinding.AddHandler(cross.GUIPositionEditHandler);
	cross.keyboardBindingHandler = [&cross, i = &gui.input]() { cross.SetLocalPosition(cross.GetLocalPosition() + i->movement); };
	cross.keyboardBindingHandlerId = gui.keyBinding.AddHandler(cross.keyboardBindingHandler);
	gui.keyBinding.AddHandler([&cross, i = &gui.input]() {
		if (i->IsDown(Key::N5)) {
			if (i->IsPressed(Key::ControlLeft) || i->IsPressed(Key::ControlRight))
				cross.SetLocalPosition(glm::vec3());
			else
				cross.SetWorldPosition(glm::vec3());
		}
	});

	SettingsLoader::Load();

	ToolPool::Cross() = &cross;
	ToolPool::Scene() = &scene;
	ToolPool::KeyBinding() = &gui.keyBinding;
	if (!ToolPool::Init())
		return false;

	StateBuffer::BufferSize().BindAndApply(Settings::StateBufferLength());
	StateBuffer::RootObject().BindTwoWay(scene.root);
	StateBuffer::Objects() = &scene.objects;
	gui.keyBinding.AddHandler([i = &gui.input, s = &scene]{
		if (i->IsDown(Key::Delete)) {
			StateBuffer::Commit();
			s->DeleteSelected();
			}
		});
	if (!StateBuffer::Init())
		return false;

	if (!LocaleProvider::Init())
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
		if (Settings::ShouldDetectPosition().Get() && !positionDetector.isPositionProcessingWorking)
			positionDetector.StartPositionDetection();
		else if (!Settings::ShouldDetectPosition().Get() && positionDetector.isPositionProcessingWorking)
			positionDetector.StopPositionDetection();
		 
		return CustomRenderFunc(scene, renderPipeline, positionDetector);
	};
	auto updateCacheForAllObjects = [&scene] {
		for (auto& o : scene.objects)
			o->ForceUpdateCache();
	};
	customRenderWindow.OnResize() += updateCacheForAllObjects;
	camera.OnPropertiesChanged() += updateCacheForAllObjects;




	// Start the main loop and clean the memory when closed.
	if (!gui.MainLoop() |
		!gui.OnExit()) {
		positionDetector.StopPositionDetection();
		return false;
	}

	// Stop Position detection thread.
	positionDetector.StopPositionDetection();
	StateBuffer::Clear();
	SettingsLoader::Save();
    return true;
}
