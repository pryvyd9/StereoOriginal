#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Converters.hpp"
#include "ToolPool.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include "Renderer.hpp"
#include <functional>

using namespace std;


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


	
	

	auto dcmd = new DeleteCommand();

	dcmd->BindScene(scene);
	dcmd->source = &((GroupObject*)scene->objects[1])->Children;
	dcmd->target = (SceneObject**)&obj;
	Command::ExecuteAll();
	//Command::ExecuteAll();

}

void testCreationTool(Scene* scene) {
	CreatingTool<StereoLine> tool;

	tool.BindScene(scene);
	tool.BindSource(&((GroupObject*)scene->objects[1])->Children);
	tool.initFunc = [] (SceneObject * o) {
		o->Name = "CreatedByCreatingTool";
		return true;
	};

	SceneObject** obj;
	tool.Create(&obj);
	
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
	p1->Points.push_back(glm::vec3(0, 0.2, 1));
	p1->Points.push_back(glm::vec3(0, -0.2, 1));
	p1->Points.push_back(glm::vec3(-0.5, -0.2, 1));
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

//bool CustomRenderFunc(Cross& cross, Scene& scene, Renderer& renderPipeline) {
//	auto sizes = new size_t[scene.objects.size()];
//	size_t sizeSum = 0;
//	for (size_t i = 0; i < scene.objects.size(); i++)
//		sizeSum += sizes[i] = LineConverter::GetLineCount(scene.objects[i]);
//
//	// We will put cross' lines there too.
//	sizeSum += cross.lineCount;
//
//	auto convertedObjects = new StereoLine[sizeSum];
//	size_t k = 0;
//
//	for (size_t i = 0; i < scene.objects.size(); k += sizes[i++])
//		if (sizes[i] > 0)
//			LineConverter::Convert(scene.objects[i], convertedObjects + k);
//
//	// Put cross' lines
//	for (size_t i = 0; i < cross.lineCount; i++, k++)
//		convertedObjects[k] = cross.lines[i];
//
//	renderPipeline.Pipeline(&convertedObjects, sizeSum, scene);
//
//	// Free memory after the lines are drawn.
//	auto rel1cmd = new FuncCommand();
//	rel1cmd->func = [sizes, convertedObjects] {
//		delete[] sizes, convertedObjects;
//	};
//
//	return true;
//}


bool CustomRenderFunc(Cross& cross, Scene& scene, Renderer& renderPipeline) {
	std::vector<size_t> sizes(scene.objects.size());
	size_t sizeSum = 0;
	for (size_t i = 0; i < scene.objects.size(); i++)
		sizeSum += sizes[i] = LineConverter::GetLineCount(scene.objects[i]);

	// We will put cross' lines there too.
	sizeSum += cross.lineCount;

	std::vector<StereoLine> convertedObjects(sizeSum);
	size_t k = 0;

	for (size_t i = 0; i < scene.objects.size(); k += sizes[i++])
		if (sizes[i] > 0)
			LineConverter::Convert(scene.objects[i], &convertedObjects[k]);

	// Put cross' lines
	for (size_t i = 0; i < cross.lineCount; i++, k++)
		convertedObjects[k] = cross.lines[i];

	auto d = convertedObjects.data();

	renderPipeline.Pipeline(&d, sizeSum, scene);

	return true;
}


int main(int, char**)
{
	std::map<int, int> map;
	map.emplace(1,2);
	int* i = &map[1];
	map.erase(1);






	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow<StereoCamera> cameraPropertiesWindow;
	SceneObjectPropertiesWindow<Cross> crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;

	CreatingToolWindow creatingToolWindow;
	AttributesWindow attributesWindow;

	Scene scene;
	StereoCamera camera;
	Renderer renderPipeline;
	GUI gui;
	Cross cross;

	if (!LoadScene(&scene))
		return false;

	//testCreation(&scene);
	testCreationTool(&scene);

	inspectorWindow.rootObject = scene.root;
	inspectorWindow.selectedObjectsBuffer = &scene.selectedObjects;

	creatingToolWindow.scene = &scene;

	cameraPropertiesWindow.Object = &camera;
	scene.camera = &camera;

	if (!renderPipeline.Init())
		return false;

	gui.windows = {
		(Window*)& customRenderWindow,
		(Window*)& cameraPropertiesWindow,
		(Window*)& crossPropertiesWindow,
		(Window*)& inspectorWindow,
		(Window*)& creatingToolWindow,
		(Window*)& attributesWindow,
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

	//ExtrusionToolWindow<StereoPolyLineT> extrusionToolWindow;
	//extrusionToolWindow.tool = ToolPool::GetTool<ExtrusionEditingTool<StereoPolyLineT>>();
	//attributesWindow.BindTool((Attributes*)&extrusionToolWindow);

	PointPenToolWindow<StereoPolyLineT> extrusionToolWindow;
	extrusionToolWindow.tool = ToolPool::GetTool<PointPenEditingTool<StereoPolyLineT>>();
	attributesWindow.BindTool((Attributes*)&extrusionToolWindow);

#pragma endregion

	customRenderWindow.customRenderFunc = [&cross, &scene, &renderPipeline] {
		return CustomRenderFunc(cross, scene, renderPipeline);
	};

	if (!gui.MainLoop() || 
		!gui.OnExit())
		return false;

    return true;
}
