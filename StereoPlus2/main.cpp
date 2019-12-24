#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "GUI.hpp"
#include "Windows.hpp"
#include "Renderer.hpp"
#include <functional>

using namespace std;


class LineConverter {
public:
	static size_t GetLineCount(SceneObject* obj) {
		switch (obj->GetType()) {
		case StereoLineT:
			return 1;
		case StereoPolyLineT:
		{
			auto size = ((StereoPolyLine*)obj)->Points.size();
			return size > 0 ? size - 1 : 0;
		}
		default:
			return 0;
		}
	}

	static void Convert(SceneObject* obj, StereoLine* objs) {
		switch (obj->GetType()) {
		case StereoLineT:
			*objs = *((StereoLine*)obj);
			break;
		case StereoPolyLineT:
			auto polyLine = (StereoPolyLine*)obj;

			for (size_t i = 1; i < polyLine->Points.size(); i++)
			{
				objs[i].Start = polyLine->Points[i - 1];
				objs[i].End = polyLine->Points[i];
				/*objs[i]->ShaderLeft = polyLine->ShaderLeft;
				objs[i]->ShaderRight = polyLine->ShaderRight;
				objs[i]->VAOLeft = polyLine->VAOLeft;
				objs[i]->VAORight = polyLine->VAORight;
				objs[i]->VBOLeft = polyLine->VBOLeft;
				objs[i]->VBORight = polyLine->VBORight;*/
			}
			break;
		}
	}


};

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

	/*tool.Rollback();
	Command::ExecuteAll();
*/
}


bool LoadScene(Scene* scene) {

	scene->objects.push_back(new GroupObject());
	scene->objects.push_back(new GroupObject());
	scene->objects.push_back(new GroupObject());
	scene->objects.push_back(new StereoLine());
	scene->objects.push_back(new StereoLine());
	scene->objects.push_back(new StereoLine());

	scene->objects[0]->Name = "Root";
	((GroupObject*) scene->objects[0])->Children.push_back(scene->objects[1]);
	((GroupObject*) scene->objects[0])->Children.push_back(scene->objects[2]);
	scene->objects[1]->Name = "Group";
	scene->objects[2]->Name = "Group";
	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[3]);
	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[4]);
	((GroupObject*)scene->objects[2])->Children.push_back(scene->objects[5]);
	scene->objects[3]->Name = "Obj1";
	scene->objects[4]->Name = "Obj1";
	scene->objects[5]->Name = "Obj3";

	scene->root = (GroupObject*) scene->objects[0];

	return true;
}

int main(int, char**)
{
	CustomRenderWindow customRenderWindow;
	SceneObjectPropertiesWindow<StereoCamera> cameraPropertiesWindow;
	SceneObjectPropertiesWindow<Cross> crossPropertiesWindow;
	SceneObjectInspectorWindow inspectorWindow;

	EditingToolWindow<StereoPolyLine> polyLineDrawingWindow;
	DrawingInstrument<StereoPolyLine> polyLineDrawingInstrument;

	polyLineDrawingWindow.instrument = &polyLineDrawingInstrument;

	CreatingToolWindow creatingToolWindow;

	Scene scene;

	if (!LoadScene(&scene))
		return false;

	//testCreation(&scene);
	//testCreationTool(&scene);

	inspectorWindow.rootObject = scene.root;
	inspectorWindow.selectedObjectsBuffer = &scene.selectedObjects;

	creatingToolWindow.scene = &scene;

	StereoCamera camera;
	cameraPropertiesWindow.Object = &camera;
	scene.camera = &camera;

	Renderer renderPipeline;
	if (!renderPipeline.Init())
		return false;

	GUI gui;
	gui.windows = {
		(Window*)& customRenderWindow,
		(Window*)& cameraPropertiesWindow,
		(Window*)& crossPropertiesWindow,
		(Window*)& inspectorWindow,
		(Window*)& polyLineDrawingWindow,
		(Window*)& creatingToolWindow,
	};

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
	
	if (!gui.Init())
		return false;

	customRenderWindow.customRenderFunc = [&cross, &scene, &renderPipeline] {
		auto sizes = new size_t[scene.objects.size()];
		size_t sizeSum = 0;
		for (size_t i = 0; i < scene.objects.size(); i++)
		{
			sizeSum += sizes[i] = LineConverter::GetLineCount(scene.objects[i]);
		}

		auto convertedObjects = new StereoLine[sizeSum];
		for (size_t i = 0, k = 0; i < scene.objects.size(); k += sizes[i++])
		{
			if (sizes[i] <= 0)
				continue;

			LineConverter::Convert(scene.objects[i], convertedObjects + k);
		}

		renderPipeline.Pipeline(&convertedObjects, sizeSum, scene);
		renderPipeline.Pipeline(&cross.lines, cross.lineCount, scene);

		// Free memory after the lines are drawn.
		auto rel1cmd = new FuncCommand();
		rel1cmd->func = [sizes, convertedObjects] {
			delete[] sizes, convertedObjects;
		};

		return true;
	};

	if (!gui.MainLoop()
		|| !gui.OnExit())
		return false;

    return true;
}
