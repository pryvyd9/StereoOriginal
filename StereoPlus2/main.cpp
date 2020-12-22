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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb/stb_image_write.h"



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

void ConfigureShortcuts(ToolWindow& tw, KeyBinding& kb, CustomRenderWindow& crw) {
	Settings::TransformToolShortcut() = Key::Combination({ Key::T });
	Settings::PenToolShortcut() = Key::Combination({ Key::P });
	Settings::ExtrusionToolShortcut() = Key::Combination({ Key::E });
	Settings::RenderViewportToFile() = Key::Combination({ Key::F5 });
	Settings::RenderAdvancedToFile() = Key::Combination({ Key::F6 });
	Settings::SwitchUseDiscreteMovement() = Key::Combination({ Key::ControlLeft, Key::Q });

	kb.AddHandler([&] (Input* i) {
		if (ImGui::IsAnyItemFocused())
			return;

		// Tools
		if (i->IsDown(Key::Escape))
			tw.Unbind();

		if (i->IsDown(Settings::TransformToolShortcut().Get()))
			tw.ApplyTool<TransformToolWindow, TransformTool>();
		if (i->IsDown(Settings::PenToolShortcut().Get()))
			tw.ApplyTool<PointPenToolWindow<StereoPolyLineT>, PointPenEditingTool<StereoPolyLineT>>();
		if (i->IsDown(Settings::ExtrusionToolShortcut().Get()))
			tw.ApplyTool<ExtrusionToolWindow<StereoPolyLineT>, ExtrusionEditingTool<StereoPolyLineT>>();

		// Rendering
		if (i->IsDown(Settings::RenderViewportToFile().Get()))
			crw.shouldSaveViewportImage = true;
		if (i->IsDown(Settings::RenderAdvancedToFile().Get()))
			crw.shouldSaveAdvancedImage = true;
		if (i->IsDown(Settings::SwitchUseDiscreteMovement().Get()))
			Settings::UseDiscreteMovement() = !Settings::UseDiscreteMovement().Get();
		});
}


//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
int main() {
	Settings::LogFileName().OnChanged() += [](const std::string& v) { Log::LogFileName() = v; };
	SettingsLoader::Load();

	//LogWindow logWindow;
	//Log::AdditionalLogOutput() = [&](const std::string& v) { logWindow.Logs += v; };

	// Declare main components.
	PositionDetector positionDetector;


	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow cameraPropertiesWindow;
	SceneObjectPropertiesWindow crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;
	AttributesWindow attributesWindow;
	ToolWindow toolWindow;

	SettingsWindow settingsWindow;
	settingsWindow.IsOpen = true;

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
	scene.camera->ViewSize.BindAndApply(customRenderWindow.RenderSize);
	scene.cross = &cross;

	gui.windows = {
		(Window*)&customRenderWindow,
		(Window*)&cameraPropertiesWindow,
		(Window*)&crossPropertiesWindow,
		(Window*)&inspectorWindow,
		(Window*)&attributesWindow,
		(Window*)&toolWindow,
		(Window*)&settingsWindow,
		//(Window*)&logWindow,
	};
	gui.glWindow = renderPipeline.glWindow;
	gui.glsl_version = renderPipeline.glsl_version;
	gui.scene = &scene;
	gui.keyBinding.cross = &cross;
	gui.settingsWindow = &settingsWindow;
	gui.renderViewport = [&customRenderWindow] { customRenderWindow.shouldSaveViewportImage = true; };
	gui.renderAdvanced = [&customRenderWindow] { customRenderWindow.shouldSaveAdvancedImage = true; };
	if (!gui.Init())
		return false;

	cross.Name = "Cross";
	cross.GUIPositionEditHandler = [&cross, i = &gui.input]() { i->movement += cross.GUIPositionEditDifference; };
	cross.GUIPositionEditHandlerId = gui.keyBinding.AddHandler(cross.GUIPositionEditHandler);
	cross.keyboardBindingHandler = [&cross, i = &gui.input]() { 
		if (!i->IsPressed(Key::AltLeft) && !i->IsPressed(Key::AltRight))
			return;

		cross.SetLocalPosition(cross.GetLocalPosition() + i->movement); 
	};
	cross.keyboardBindingHandlerId = gui.keyBinding.AddHandler(cross.keyboardBindingHandler);
	gui.keyBinding.AddHandler([&cross, i = &gui.input]() {
		if (i->IsDown(Key::N5)) {
			if (i->IsPressed(Key::ControlLeft) || i->IsPressed(Key::ControlRight)) {
				glm::vec3 v(0);
				for (auto& o : ObjectSelection::Selected())
					v += o.Get()->GetWorldPosition();
				v /= ObjectSelection::Selected().size();
				cross.SetWorldPosition(v);
			}
			else
				cross.SetWorldPosition(glm::vec3());
		}
	});



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

	ConfigureShortcuts(toolWindow, gui.keyBinding, customRenderWindow);

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main();
}
