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
	// Position detection
	if (positionDetector.isPositionProcessingWorking)
		scene.camera->SetLocalPosition(
			glm::vec3(
				positionDetector.positionHorizontal / 500.0,
				positionDetector.positionVertical / 500.0,
				-positionDetector.distance / 10.0));

	renderPipeline.Pipeline(scene);
	
	//Log::For<void>().Information(Time::GetDeltaTime());

	return true;
}

int main(int, char**)
{
	float aa = 3.1415926 / 2;
	auto q1 = glm::angleAxis(aa, glm::vec3(1, 0, 0));
	auto q2 = glm::angleAxis(aa, glm::vec3(0, 1, 0));
	auto q3 = glm::cross(q1, q2);
	auto q4 = glm::cross(glm::angleAxis(aa, glm::rotate(q1, glm::vec3(0, 1, 0))), q1);

	auto r1 = glm::rotate(q1, glm::vec3(0, 0, 1));
	auto r2 = glm::rotate(q2, glm::vec3(0, 0, 1));
	auto r3 = glm::rotate(q3, glm::vec3(0, 0, 1));
	auto r4 = glm::rotate(q4, glm::vec3(0, 0, 1));

	PositionDetector positionDetector;

	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow<StereoCamera> cameraPropertiesWindow;
	SceneObjectPropertiesWindow<Cross> crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;

	AttributesWindow attributesWindow;
	ToolWindow toolWindow;

	Scene scene;
	StereoCamera camera;
	Renderer renderPipeline;
	GUI gui;
	Cross cross;

	toolWindow.attributesWindow = &attributesWindow;
	toolWindow.scene = &scene;

	inspectorWindow.rootObject = (GroupObject**)&scene.root;
	inspectorWindow.selectedObjectsBuffer = &scene.selectedObjects;

	cameraPropertiesWindow.Object = &camera;
	scene.camera = &camera;

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

	scene.whiteZ = 0;
	scene.whiteZPrecision = 0.1;
	scene.glWindow = gui.glWindow;
	scene.camera->viewSize = &customRenderWindow.renderSize;
	scene.cross = &cross;

	gui.scene = &scene;

	cross.Name = "Cross";
	if (!cross.Init())
		return false;

	crossPropertiesWindow.Object = &cross;
	gui.keyBinding.cross = &cross;
	if (!gui.Init())
		return false;

	* ToolPool::GetCross() = &cross;
	* ToolPool::GetScene() = &scene;
	* ToolPool::GetKeyBinding() = &gui.keyBinding;

	if (!ToolPool::Init())
		return false;

	positionDetector.onStartProcess = [&positionDetector] {
		positionDetector.Init();
	};

	customRenderWindow.customRenderFunc = [&scene, &renderPipeline, shouldUsePositionDetection = &gui.shouldUsePositionDetection, &positionDetector]{
		if (*shouldUsePositionDetection && !positionDetector.isPositionProcessingWorking)
			positionDetector.StartPositionDetection();
		else if (!*shouldUsePositionDetection && positionDetector.isPositionProcessingWorking)
			positionDetector.StopPositionDetection();

		return CustomRenderFunc(scene, renderPipeline, positionDetector);
	};

	if (!gui.MainLoop() |
		!gui.OnExit()) {
		positionDetector.StopPositionDetection();
		return false;
	}

	positionDetector.StopPositionDetection();
    return true;
}
