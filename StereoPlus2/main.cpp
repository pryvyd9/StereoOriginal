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
	// Internal shortcuts.
	kb.input->AddShortcut(Key::Combination(Key::Escape),
		[&] { tw.Unbind(); });
	kb.input->AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::Z),
		[&] { StateBuffer::Rollback(); });
	kb.input->AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::Y),
		[&] { StateBuffer::Repeat(); });
	kb.input->AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::D),
		[&] { ObjectSelection::RemoveAll(); });

	// Tools
	kb.input->AddShortcut(Key::Combination(Key::T),
		[&] { tw.ApplyTool<TransformToolWindow, TransformTool>(); });
	kb.input->AddShortcut(Key::Combination(Key::P),
		[&] { tw.ApplyTool<PointPenToolWindow, PointPenEditingTool>(); });
	kb.input->AddShortcut(Key::Combination(Key::E),
		[&] { tw.ApplyTool<ExtrusionToolWindow<StereoPolyLineT>, ExtrusionEditingTool<StereoPolyLineT>>(); });

	// Render
	kb.input->AddShortcut(Key::Combination(Key::F5),
		[&] { crw.shouldSaveViewportImage = true; });
	kb.input->AddShortcut(Key::Combination(Key::F6),
		[&] { crw.shouldSaveAdvancedImage = true; });
	
	// State
	kb.input->AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::Q),
		[&] { Settings::UseDiscreteMovement() = !Settings::UseDiscreteMovement().Get(); });
	kb.input->AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::W),
		[&] { Settings::SpaceMode() = Settings::SpaceMode().Get() == SpaceMode::Local ? SpaceMode::World : SpaceMode::Local; });
}


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

	inspectorWindow.rootObject.BindAndApply(scene.root());
	inspectorWindow.input = &gui.input;

	cameraPropertiesWindow.Object = &camera;
	crossPropertiesWindow.Object = &cross;

	scene.camera = &camera;
	scene.glWindow = renderPipeline.glWindow;
	scene.camera->ViewSize.BindAndApply(customRenderWindow.RenderSize);
	scene.cross() = &cross;

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
		if (!i->IsPressed(Key::Modifier::Alt))
			return;

		auto m = i->movement * Settings::TranslationStep().Get();
		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			m = glm::rotate(cross.GetWorldRotation(), m);

		//cross.SetLocalPosition(cross.GetLocalPosition() + i->movement * Settings::TranslationStep().Get()); 
		cross.SetWorldPosition(cross.GetWorldPosition() + m); 
	};
	cross.keyboardBindingHandlerId = gui.keyBinding.AddHandler(cross.keyboardBindingHandler);
	gui.keyBinding.AddHandler([&cross, i = &gui.input]() {
		if (i->IsDown(Key::N5)) {
			if (i->IsPressed(Key::Modifier::Control)) {
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
	StateBuffer::RootObject().BindTwoWay(scene.root());
	StateBuffer::Objects() = &scene.Objects().Get();
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
		for (auto& o : scene.Objects().Get())
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
