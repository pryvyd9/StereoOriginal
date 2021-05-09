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
		scene.camera->PositionModifier =
		glm::vec3(
			positionDetector.positionHorizontal.load(),
			positionDetector.positionVertical.load(),
			positionDetector.distance.load());

	// Run scene drawing.
	renderPipeline.Pipeline(scene);
	
	return true;
}

void ConfigureShortcuts(CustomRenderWindow& crw) {
	// Internal shortcuts.
	Input::AddShortcut(Key::Combination(Key::Escape),
		ToolWindow::ApplyDefaultTool().Get());
	Input::AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::Z),
		StateBuffer::Rollback);
	Input::AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::Y),
		StateBuffer::Repeat);
	Input::AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::D),
		ObjectSelection::RemoveAll);

	// Tools
	Input::AddShortcut(Key::Combination(Key::T),
		ToolWindow::ApplyTool<TransformToolWindow, TransformTool>);
	Input::AddShortcut(Key::Combination(Key::P),
		ToolWindow::ApplyTool<PenToolWindow, PenTool>);
	Input::AddShortcut(Key::Combination(Key::S),
		ToolWindow::ApplyTool<SinePenToolWindow, SinePenTool>);
	Input::AddShortcut(Key::Combination(Key::E),
		ToolWindow::ApplyTool<ExtrusionToolWindow<PolyLineT>, ExtrusionEditingTool<PolyLineT>>);

	// Render
	Input::AddShortcut(Key::Combination(Key::F5),
		[&] { crw.shouldSaveViewportImage = true; });
	Input::AddShortcut(Key::Combination(Key::F6),
		[&] { crw.shouldSaveAdvancedImage = true; });
	
	// State
	Input::AddShortcut(Key::Combination(Key::D),
		[] { Settings::UseDiscreteMovement() = !Settings::UseDiscreteMovement().Get(); });
	Input::AddShortcut(Key::Combination(Key::W),
		[] { Settings::SpaceMode() = Settings::SpaceMode().Get() == SpaceMode::Local ? SpaceMode::World : SpaceMode::Local; });
	Input::AddShortcut(Key::Combination(Key::C),
		[] { Settings::TargetMode() = Settings::TargetMode().Get() == TargetMode::Object ? TargetMode::Pivot : TargetMode::Object; });
}

int main() {
	Settings::LogFileName().OnChanged() += [](const std::string& v) { Log::LogFileName() = v; };
	SettingsLoader::Load();

	//LogWindow logWindow;
	//Log::AdditionalLogOutput() = [&](const std::string& v) { logWindow.Logs += v; };
	Log::Sink() = Log::ConsoleSink;

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
	Camera camera;
	Cross cross;

	// Initialize main components.
	inspectorWindow.rootObject <<= scene.root();
	inspectorWindow.input = &gui.input;

	cameraPropertiesWindow.Object = &camera;
	crossPropertiesWindow.Object = &cross;

	ToolWindow::ApplyDefaultTool() = ToolWindow::ApplyTool<TransformToolWindow, TransformTool>;
	ToolWindow::AttributesWindow() = &attributesWindow;
	ToolWindow::ApplyDefaultTool()();
	
	scene.camera = &camera;
	scene.glWindow = renderPipeline.glWindow;
	scene.camera->ViewSize <<= customRenderWindow.RenderSize;
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
		if (!Input::IsPressed(Key::Modifier::Alt) && cross.GUIPositionEditDifference == glm::vec3())
			return;

		auto m = i->movement * Settings::TranslationStep().Get();
		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			m = glm::rotate(cross.GetWorldRotation(), m);

		cross.SetWorldPosition(cross.GetWorldPosition() + m);
	};
	cross.keyboardBindingHandlerId = gui.keyBinding.AddHandler(cross.keyboardBindingHandler);
	gui.keyBinding.AddHandler([&cross]() {
		if (Input::IsPressed(Key::Modifier::Alt)) {
			if (Input::IsDown(Key::N5, true) && !ObjectSelection::Selected().empty()) {
				glm::vec3 v(0);
				for (auto& o : ObjectSelection::Selected())
					v += o.Get()->GetWorldPosition();
				v /= ObjectSelection::Selected().size();
				cross.SetWorldPosition(v);
			}
			else if (Input::IsDown(Key::N0, true))
				cross.SetWorldPosition(glm::vec3());
		}
		else if (Input::IsPressed(Key::Modifier::Control)) {
			if (Input::IsDown(Key::N5, true) && !ObjectSelection::Selected().empty())
				cross.SetWorldRotation(ObjectSelection::Selected().begin()._Ptr->_Myval->GetWorldRotation());
			else if (Input::IsDown(Key::N0, true))
				cross.SetWorldRotation(glm::quat(1,0,0,0));
		}
	});


	ToolPool::KeyBinding() = &gui.keyBinding;
	if (!ToolPool::Init())
		return false;


	StateBuffer::BufferSize() <<= Settings::StateBufferLength();
	StateBuffer::RootObject() <<= scene.root();
	StateBuffer::Objects() <<= scene.Objects();
	gui.keyBinding.AddHandler([s = &scene]{
		if (Input::IsDown(Key::Delete, true)) {
			StateBuffer::Commit();
			s->DeleteSelected();
			}
		});
	if (!StateBuffer::Init())
		return false;

	if (!LocaleProvider::Init())
		return false;

	scene.OnDeleteAll() += [] {
		ObjectSelection::RemoveAll();
	};

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

	ConfigureShortcuts(customRenderWindow);

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
