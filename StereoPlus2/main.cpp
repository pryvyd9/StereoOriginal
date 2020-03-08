#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Converters.hpp"
#include "ToolPool.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include "Renderer.hpp"
#include <functional>
#include "FileManager.hpp"
#include <filesystem> // C++17 standard header file name
//#include <experimental/filesystem> // Header file for pre-standard implementation

using namespace std;

#include <chrono>


void testCreation(Scene* scene) {
	CreateCommand* cmd = new CreateCommand();

	cmd->BindScene(scene);
	cmd->source = &((GroupObject*)scene->objects[1])->Children;

	StereoLine* obj;
	cmd->func = [&obj]{
		obj = new StereoLine();
		obj->Name = "CreatedObject";
		return obj;
	};


	Command::ExecuteAll();

	

	auto dcmd = new DeleteCommand();

	dcmd->BindScene(scene);
	dcmd->source = &((GroupObject*)scene->objects[1])->Children;
	dcmd->target = obj;
	Command::ExecuteAll();
	//Command::ExecuteAll();

}

void testCreationTool(Scene* scene) {
	CreatingTool<StereoLine> tool;
	SceneObject* obj;

	tool.BindScene(scene);
	tool.BindSource(&((GroupObject*)scene->objects[1])->Children);
	
	tool.func = [obj = &obj] (SceneObject * o) {
		o->Name = "CreatedByCreatingTool";
		*obj = o;
	};

	tool.Create();
	

	Command::ExecuteAll();




}

//bool LoadScene(Scene* scene) {
//	scene->objects.push_back(new GroupObject());
//	scene->objects.push_back(new GroupObject());
//	scene->objects.push_back(new GroupObject());
//	scene->objects.push_back(new StereoLine());
//	scene->objects.push_back(new StereoLine());
//	scene->objects.push_back(new StereoLine());
//
//	scene->objects[0]->Name = "Root";
//	((GroupObject*)scene->objects[0])->Children.push_back(scene->objects[1]);
//	((GroupObject*)scene->objects[0])->Children.push_back(scene->objects[2]);
//	scene->objects[1]->Name = "Group";
//	scene->objects[2]->Name = "Group";
//	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[3]);
//	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[4]);
//	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[5]);
//	scene->objects[3]->Name = "Obj1";
//	scene->objects[4]->Name = "Obj1";
//	scene->objects[5]->Name = "Obj3";
//
//	scene->root = (GroupObject*)scene->objects[0];
//
//	return true;
//}


bool LoadScene(Scene* scene) {

	auto root = new GroupObject();
	auto g1 = new GroupObject();
	auto g2 = new GroupObject();
	auto s1 = new StereoLine();
	auto s2 = new StereoLine();
	auto s3 = new StereoLine();
	auto p1 = new StereoPolyLine();


	root->Name = "Root";
	root->Children.push_back(g1);
	root->Children.push_back(g2);
	g1->Name = "Group1";
	g1->Children.push_back(p1);
	//p1->Points.push_back(glm::vec3(0, 0.2, 1));
	//p1->Points.push_back(glm::vec3(0, -0.2, 1));
	//p1->Points.push_back(glm::vec3(-0.5, -0.2, 1));
	p1->Points.push_back(glm::vec3(0, 0, 0));
	p1->Points.push_back(glm::vec3(0.1, 0.1, 0));
	p1->Points.push_back(glm::vec3(0.2, 0, 0));
	p1->Points.push_back(glm::vec3(0.1, -0.1, 0));
	p1->Points.push_back(glm::vec3(0, 0, 0));
	p1->Points.push_back(glm::vec3(-0.1, 0.1, 0));
	p1->Points.push_back(glm::vec3(-0.2, 0, 0));
	p1->Points.push_back(glm::vec3(-0.1, -0.1, 0));
	p1->Points.push_back(glm::vec3(0, 0, 0));



	p1->Name = "PolyLine1";

	g2->Name = "Group2";
	g2->Children.push_back(s1);
	g2->Children.push_back(s2);
	g2->Children.push_back(s3);
	s1->Name = "Obj1";
	s2->Name = "Obj2";
	s3->Name = "Obj3";

	s1->Start = glm::vec3(0);
	s1->End = glm::vec3(1,1,-2);

	scene->objects.push_back(root);
	scene->objects.push_back(g1);
	scene->objects.push_back(g2);
	scene->objects.push_back(s1);
	scene->objects.push_back(s2);
	scene->objects.push_back(s3);
	scene->objects.push_back(p1);
	scene->root = root;

	return true;
}

bool CustomRenderFunc(Cross& cross, Scene& scene, Renderer& renderPipeline) {
	//std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();



	std::vector<size_t> sizes(scene.objects.size());
	size_t sizeSum = 0;
	for (size_t i = 0; i < scene.objects.size(); i++)
		sizeSum += sizes[i] = LineConverter::GetLineCount(scene.objects[i]);

	//auto t0 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

	// We will put cross' lines there too.
	sizeSum += cross.lineCount;

	std::vector<StereoLine> convertedObjects(sizeSum);
	size_t k = 0;

	for (size_t i = 0; i < scene.objects.size(); k += sizes[i++])
		if (sizes[i] > 0)
			LineConverter::Convert(scene.objects[i], &convertedObjects[k]);
	//auto t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

	//std::cout << "FPS: " << t << std::endl;


	// Put cross' lines
	for (size_t i = 0; i < cross.lineCount; i++, k++)
		convertedObjects[k] = cross.lines[i];

	//auto t2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

	auto d = convertedObjects.data();

	renderPipeline.Pipeline(&d, sizeSum, scene);

	//auto t3 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count();

	return true;
}

namespace fs = std::filesystem;

void DisplayDirectoryTree(const fs::path& pathToScan, int level = 0) {
	for (const auto& entry : fs::directory_iterator(pathToScan)) {
		const auto filenameStr = entry.path().filename().string();
		if (entry.is_directory()) {
			std::cout << std::string(" ", level * 3) << "" << filenameStr << '\n';
			//DisplayDirectoryTree(entry, level + 1);
		}
		else if (entry.is_regular_file()) {
			std::cout << std::string(" ", level * 3) << "" << filenameStr
				<< "\n";
		}
		else
			std::cout << std::string(" ", level * 3) << "" << " [?]" << filenameStr << '\n';
	}
}

//void FileSystemTest() {
//	auto pathToShow = "F:/";
//
//	//DisplayDirectoryTree(pathToShow);
//
//	//for (const auto& entry : fs::directory_iterator(pathToShow)) {
//	//	const auto filenameStr = entry.path().filename().string();
//	//	if (is_directory(entry)) {
//	//		std::cout << "dir:  " << filenameStr << '\n';
//	//	}
//	//	else if (is_regular_file(entry)) {
//	//		std::cout << "file: " << filenameStr << '\n';
//	//	}
//	//	else
//	//		std::cout << "??    " << filenameStr << '\n';
//	//}
//}

int main(int, char**)
{
	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow<StereoCamera> cameraPropertiesWindow;
	SceneObjectPropertiesWindow<Cross> crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;
	OpenFileWindow openFileWindow;

	CreatingToolWindow creatingToolWindow;
	AttributesWindow attributesWindow;
	ToolWindow toolWindow;

	Scene scene;
	StereoCamera camera;
	Renderer renderPipeline;
	GUI gui;
	Cross cross;


	//FileSystemTest();

	//if (!LoadScene(&scene))
	//	return false;

	//FileManager::SaveBinary("scene1", &scene);
	//FileManager::LoadBinary("scene1", &scene);
	FileManager::LoadJson("scene1.json", &scene);
	//FileManager::SaveJson("scene1.json", &scene);

	//testCreation(&scene);
	//testCreationTool(&scene);

	openFileWindow.BindScene(&scene);
	toolWindow.attributesWindow = &attributesWindow;

	inspectorWindow.rootObject = scene.root;
	inspectorWindow.selectedObjectsBuffer = &scene.selectedObjects;

	creatingToolWindow.scene = &scene;

	cameraPropertiesWindow.Object = &camera;
	scene.camera = &camera;

	if (!renderPipeline.Init())
		return false;

	gui.windows = {
		(Window*)&customRenderWindow,
		(Window*)&cameraPropertiesWindow,
		(Window*)&crossPropertiesWindow,
		(Window*)&inspectorWindow,
		(Window*)&creatingToolWindow,
		(Window*)&attributesWindow,
		(Window*)&toolWindow,
		(Window*)&openFileWindow,
	};

	gui.glWindow = renderPipeline.glWindow;
	gui.glsl_version = renderPipeline.glsl_version;

	scene.whiteZ = 0;
	scene.whiteZPrecision = 0.1;
	scene.glWindow = gui.glWindow;
	scene.camera->viewSize = &customRenderWindow.renderSize;

	gui.scene = &scene;

	cross.Name = "Cross";
	if (!cross.Init())
		return false;

	crossPropertiesWindow.Object = &cross;
	gui.keyBinding.cross = &cross;
	if (!gui.Init())
		return false;

#pragma region Init Tools

	* ToolPool::GetCross() = &cross;
	* ToolPool::GetScene() = &scene;
	* ToolPool::GetKeyBinding() = &gui.keyBinding;

	if (!ToolPool::Init())
		return false;

#pragma endregion

	customRenderWindow.customRenderFunc = [&cross, &scene, &renderPipeline] {
		return CustomRenderFunc(cross, scene, renderPipeline);
	};

	if (!gui.MainLoop() || 
		!gui.OnExit())
		return false;

    return true;
}
