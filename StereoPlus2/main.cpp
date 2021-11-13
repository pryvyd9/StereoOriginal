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
		Changes::Rollback);
	Input::AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::Y),
		Changes::Repeat);
	Input::AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::D),
		ObjectSelection::RemoveAll);	
	Input::AddShortcut(Key::Combination({ Key::Modifier::Control }, Key::A),
		[] { ObjectSelection::Set(Scene::Objects().Get()); });


	// Tools
	Input::AddShortcut(Key::Combination(Key::T),
		ToolWindow::ApplyTool<TransformToolWindow, TransformTool>);
	Input::AddShortcut(Key::Combination(Key::P),
		ToolWindow::ApplyTool<PenToolWindow, PenTool>);
	Input::AddShortcut(Key::Combination(Key::S),
		ToolWindow::ApplyTool<CosinePenToolWindow, CosinePenTool>);
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

	if (!LocaleProvider::Init())
		return false;

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

	Scene scene([] { return LocaleProvider::Get("object:root"); });
	Camera camera;
	Cross cross;

	// Initialize main components.
	inspectorWindow.rootObject <<= scene.root();
	inspectorWindow.input = &gui.input;

	cameraPropertiesWindow.Object = &camera;
	crossPropertiesWindow.Object = &cross;
	ReadOnlyState::ViewSize() <<= customRenderWindow.RenderSize;

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
	gui.settingsWindow = &settingsWindow;
	gui.renderViewport = [&customRenderWindow] { customRenderWindow.shouldSaveViewportImage = true; };
	gui.renderAdvanced = [&customRenderWindow] { customRenderWindow.shouldSaveAdvancedImage = true; };
	if (!gui.Init())
		return false;

	cross.Name = "Cross";
	cross.GUIPositionEditHandler = [] { Input::movement() += Scene::cross()->GUIPositionEditDifference; };
	cross.GUIPositionEditHandlerId = Input::AddHandler(cross.GUIPositionEditHandler);
	cross.keyboardBindingHandler = [] {
		auto relativeRotation = Input::GetRelativeRotation(glm::vec3());
		auto relativeMovement = Input::GetRelativeMovement(Scene::cross()->GUIPositionEditDifference);

		if (relativeRotation != glm::vec3())
			Transform::Rotate(Scene::cross()->GetWorldPosition(), relativeRotation, &Scene::cross().Get());
		if (relativeMovement != glm::vec3())
			Transform::Translate(relativeMovement, &Scene::cross().Get());
	};
	cross.keyboardBindingHandlerId = Input::AddHandler(cross.keyboardBindingHandler);

	if (!ToolPool::Init())
		return false;


	Changes::RootObject() <<= scene.root();
	Changes::Objects() <<= scene.Objects();
	Input::AddHandler([s = &scene]{
		if (Input::IsDown(Key::Delete, true)) {
			Changes::Commit();
			s->DeleteSelected();
			}
		});
	if (!Changes::Init())
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
		Scene::cross()->ForceUpdateCache();

		for (auto& o : Scene::Objects().Get())
			o->ForceUpdateCache();
	};
	customRenderWindow.OnResize() += updateCacheForAllObjects;
	camera.OnPropertiesChanged() += updateCacheForAllObjects;
	Settings::PointRadiusPixel().OnChanged() += [updateCacheForAllObjects](int) { updateCacheForAllObjects(); };

	ConfigureShortcuts(customRenderWindow);

	// Start the main loop and clean the memory when closed.
	if (!gui.MainLoop() |
		!gui.OnExit() |
		!renderPipeline.OnExit()) {
		positionDetector.StopPositionDetection();
		return false;
	}

	// Stop Position detection thread.
	positionDetector.StopPositionDetection();
	Changes::Clear();
	SettingsLoader::Save();
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	return main();
}
