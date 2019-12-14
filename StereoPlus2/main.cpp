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


#include <set>

void test() {

	std::set<int> vec;

	vec.emplace(1);
	vec.emplace(2);
	vec.emplace(3);
	vec.emplace(4);
	vec.emplace(4);
	vec.emplace(4);

	//auto b = vec.begin() + 2;

	////auto er1 = (std::vector<int>*)vec.begin()._Verify_offset;

	//auto h = *(b)._Ptr;
	//vec.insert(vec.begin() +1, 5);
	//vec.erase(b);
}

bool LoadScene(Scene* scene) {

	scene->objects.push_back(new GroupObject());
	scene->objects.push_back(new GroupObject());
	scene->objects.push_back(new GroupObject());
	scene->objects.push_back(new StereoLine());
	scene->objects.push_back(new StereoLine());

	scene->objects[0]->Name = "Group0";
	((GroupObject*) scene->objects[0])->Children.push_back(scene->objects[1]);
	((GroupObject*) scene->objects[0])->Children.push_back(scene->objects[2]);
	scene->objects[1]->Name = "Group1";
	scene->objects[2]->Name = "Group2";
	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[3]);
	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[4]);
	scene->objects[3]->Name = "Obj1";
	scene->objects[4]->Name = "Obj2";

	scene->root = (GroupObject*) scene->objects[0];

	return true;
}

int main(int, char**)
{
	test();
	Renderer renderPipeline;

	GUI gui;

	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow<StereoCamera> cameraPropertiesWindow;
	SceneObjectPropertiesWindow<Cross> crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;

	Scene scene;

	if (!LoadScene(&scene))
		return false;

	inspectorWindow.rootObject = scene.root;
	inspectorWindow.selectedObjectsBuffer = &scene.selectedObjects;

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
