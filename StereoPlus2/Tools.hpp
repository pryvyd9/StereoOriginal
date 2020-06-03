#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include "Input.hpp"
#include <map>
#include <stack>
#include <vector>
#include <queue>
#include "TemplateExtensions.hpp"


class Tool {
protected:
	/*struct Change {
		SceneObject* reference, * stateCopy;
	};*/
	//static std::stack<Change>& GetChanges() {
	//	static auto val = std::stack<Change>();
	//	return val;
	//}
public:
	//virtual bool Rollback() {
	//	if (GetChanges().size() <= 0)
	//		return true;
	//	
	//	auto change = GetChanges().top();
	//	change.reference = change.stateCopy;
	//	
	//	return true;
	//}
};

template<typename T>
class CreatingTool : Tool, public ISceneHolder {
	SceneObject** destination;

	static std::stack<SceneObject*>& GetCreatedObjects() {
		static std::stack<SceneObject*> val;
		return val;
	}
public:
	std::function<void(T*)> func;

	bool BindDestination(SceneObject** destination) {
		this->destination = destination;
		return true;
	}

	bool Create() {
		auto command = new CreateCommand();
		if (!command->BindScene(scene))
			return false;

		command->destination = *destination;
		command->func = [=] {
			T* obj = new T();
			func(obj);
			GetCreatedObjects().push(obj);

			return obj;
		};

		return true;
	}

	/*virtual bool Rollback() {
		auto command = new DeleteCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = *destination;
		command->target = GetCreatedObjects().top();
		GetCreatedObjects().pop();

		return true;
	}*/
};


template<typename EditMode>
class EditingTool : Tool {
protected:
	using Mode = EditMode;

	struct Config {
		template<typename T>
		T* Get() { return (T*)this; };
	};

	KeyBinding* keyBinding = nullptr;
	Config* config = nullptr;

	void DeleteConfig() {
		if (config != nullptr)
		{
			delete config;
			config = nullptr;
		}
	}
public:
	enum Type {
		PointPen,
		Extrusion,
		Transform,
	};


	virtual Type GetType() = 0;
	virtual bool BindSceneObjects(std::vector<SceneObject*> obj) = 0;
	virtual bool UnbindSceneObjects() = 0;
	virtual bool BindInput(KeyBinding* keyBinding) {
		return this->keyBinding = keyBinding;
	}
};

#pragma region configs
enum class PointPenEditingToolMode {
	Immediate,
	Step,
};

enum class ExtrusionEditingToolMode {
	Immediate,
	Step,
};

enum class ObjectMode {
	Object,
	Vertex,
};

enum class SpaceMode {
	World,
	Local,
};

enum class TransformToolMode {
	Translate,
	Scale,
	Rotate,
};

class GlobalToolConfiguration {
	static ObjectMode& objectMode() {
		static ObjectMode v;
		return v;
	}
	static SpaceMode& spaceMode() {
		static SpaceMode v;
		return v;
	}

	static Event<ObjectMode>& objectModeChanged() {
		static Event<ObjectMode> v;
		return v;
	}
	static Event<SpaceMode>& spaceModeChanged() {
		static Event<SpaceMode> v;
		return v;
	}
public:
	static const ObjectMode& GetCoordinateMode() {
		return objectMode();
	}
	static const SpaceMode& GetSpaceMode() {
		return spaceMode();
	}
	
	static void SetObjectMode(const ObjectMode& v) {
		auto a = objectMode();
		objectMode() = v;
		if (a != v)
			objectModeChanged().Invoke(v);
	}
	static void SetSpaceMode(const SpaceMode& v) {
		auto a = spaceMode();
		spaceMode() = v;
		if (a != v)
			spaceModeChanged().Invoke(v);
	}

	static IEvent<ObjectMode>& OnObjectModeChanged() {
		return objectModeChanged();
	}
	static IEvent<SpaceMode>& OnSpaceModeChanged() {
		return spaceModeChanged();
	}
};
#pragma endregion

template<ObjectType type>
class PointPenEditingTool : public EditingTool<PointPenEditingToolMode>{
#pragma region Types
	template<ObjectType type, Mode mode>
	struct Config : EditingTool::Config {

	};
	template<>
	struct Config<StereoPolyLineT, Mode::Immediate> : EditingTool::Config {
		bool isPointCreated = false;

		// If the cos between vectors is less than E
		// then we merge those vectors.
		double E = 1e-6;

		// If distance between previous point and 
		// cursor is less than this number 
		// then don't create a new point.
		double Precision = 1e-4;
	};
	template<>
	struct Config<StereoPolyLineT, Mode::Step> : EditingTool::Config {
		bool isPointCreated = false;
	};
#pragma endregion

	Log Logger = Log::For<PointPenEditingTool>();
	size_t inutHandlerId;
	size_t spaceModeChangeHandlerId;
	Mode mode;

	Cross* cross = nullptr;
	SceneObject* target = nullptr;

	glm::vec3 crossOriginalPosition;
	SceneObject* crossOriginalParent;


	template<Mode mode>
	Config<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new Config<type, mode>();

		return (Config<type, mode>*) config;
	}

	const glm::vec3 GetPos() {
		if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::Local)
			return cross->GetLocalPosition();

		return target->ToLocalPosition(cross->GetLocalPosition());
	}

#pragma region ProcessInput

	template<ObjectType type, Mode mode>
	void ProcessInput(Input* input) {
		Logger.Warning("Unsupported Editing Tool target Type or Unsupported combination of ObjectType and PointPenEditingToolMode");
	}
	template<>
	void ProcessInput<StereoPolyLineT, Mode::Immediate>(Input* input) {
		if (input->IsDown(Key::Escape)) {
			UnbindSceneObjects();
			return;
		}
		
		auto& points = target->GetVertices();
		auto pointsCount = points.size();

		if (pointsCount > 0 && GetPos() == points.back())
			return;

		if (pointsCount < 3 || !GetConfig<Mode::Immediate>()->isPointCreated) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(cross->GetLocalPosition());
			GetConfig<Mode::Immediate>()->isPointCreated = true;
			return;
		}


		// Drawing optimization
		
		// If cross is located at distance less than Precision then don't create a new point
		// but move the last one to cross position.
		if (glm::length(GetPos() - points[pointsCount - 2]) < GetConfig<Mode::Immediate>()->Precision) {
			target->SetVertice(pointsCount - 1, GetPos());
			return;
		}


		// If the line goes straight then instead of adding 
		// a new point - move the previous point to current cross position.
		auto E = GetConfig<Mode::Immediate>()->E;

		glm::vec3 r1 = GetPos() - points[pointsCount - 1];
		glm::vec3 r2 = points[pointsCount - 3] - points[pointsCount - 2];

		auto p = glm::dot(r1, r2);
		auto l1 = glm::length(r1);
		auto l2 = glm::length(r2);


		auto cos = p / l1 / l2;
				
		if (abs(cos) > 1 - E || isnan(cos)) {
			target->SetVertice(pointsCount - 2,  GetPos());
			target->SetVertice(pointsCount - 1,  GetPos());
		}
		else {
			target->AddVertice(GetPos());
		}

		//std::cout << "PointPen tool Immediate mode points count: " << pointsCount << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Mode::Step>(Input* input) {
		if (input->IsDown(Key::Escape)) {
			UnbindSceneObjects();
			return;
		}
		auto& points = target->GetVertices();
		auto pointsCount = points.size();


		if (!GetConfig<Mode::Step>()->isPointCreated) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(GetPos());
			GetConfig<Mode::Step>()->isPointCreated = true;
			return;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter)) {
			target->AddVertice(GetPos());

			return;
		}

		if (pointsCount > 0 && GetPos() == points.back())
			return;

		target->SetVertice(points.size() - 1, GetPos());
	}

	void ProcessInput(Input* input) {
		if (target == nullptr) {
			UnbindSceneObjects();
			return;
		}
			
		switch (mode)
		{
		case Mode::Immediate:
			return ProcessInput<type, Mode::Immediate>(input);
		case Mode::Step:
			return ProcessInput<type, Mode::Step>(input);
		default:
			Logger.Warning("Not suported mode was given");
			return;
		}
	}

#pragma endregion

public:
	std::vector<std::function<void()>> onTargetReleased;



	virtual bool BindSceneObjects(std::vector<SceneObject*> objs) {
		if (target != nullptr && !UnbindSceneObjects())
			return false;

		if (keyBinding == nullptr)
		{
			Logger.Warning("KeyBinding wasn't assigned");
			return false;
		}

		if (objs[0]->GetType() != type)
		{
			Logger.Warning("Invalid Object passed to PointPenEditingTool");
			return true;
		}
		target = objs[0];

		crossOriginalPosition = cross->GetLocalPosition();
		crossOriginalParent = const_cast<SceneObject*>(cross->GetParent());

		if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::Local) {
			cross->SetParent(target);
			cross->SetLocalPosition(target->GetVertices().back());
		}
		else {
			if (target->GetVertices().size() > 0)
				cross->SetWorldPosition(target->ToWorldPosition(target->GetVertices().back()));
			else
				cross->SetWorldPosition(target->GetWorldPosition());
		}

		inutHandlerId = keyBinding->AddHandler([&](Input * input) { ProcessInput(input); });
		spaceModeChangeHandlerId = GlobalToolConfiguration::OnSpaceModeChanged().AddHandler([&](const SpaceMode& v) {
			if (v == SpaceMode::Local) {
				cross->SetParent(target);
				if (target->GetVertices().size() > 0)
					cross->SetLocalPosition(target->GetVertices().back());
				else
					cross->SetLocalPosition(glm::vec3());
			}
			else {
				cross->SetParent(crossOriginalParent);
				if (target->GetVertices().size() > 0)
					cross->SetWorldPosition(target->ToWorldPosition(target->GetVertices().back()));
				else
					cross->SetWorldPosition(target->GetWorldPosition());
			}
			});

		return true;
	}
	virtual bool UnbindSceneObjects() {
		UnbindSceneObjects<type>();
		
		for (auto func : onTargetReleased)
			func();

		cross->SetParent(crossOriginalParent);
		cross->SetLocalPosition(crossOriginalPosition);
		keyBinding->RemoveHandler(inutHandlerId);
		GlobalToolConfiguration::OnSpaceModeChanged().RemoveHandler(spaceModeChangeHandlerId);

		DeleteConfig();

		return true;
	}
	virtual Type GetType() {
		return Type::PointPen;
	}

	SceneObject** GetTarget() {
		return &target;
	}

	template<ObjectType type>
	void UnbindSceneObjects() {
		switch (mode)
		{
		case Mode::Immediate:
			return UnbindSceneObjects<type, Mode::Immediate>();
		case Mode::Step:
			return UnbindSceneObjects<type, Mode::Step>();
		default:
			Logger.Warning("Not suported mode was given");
			return;
		}
	}

	template<ObjectType type, Mode mode>
	void UnbindSceneObjects() {
		
	}
	template<>
	void UnbindSceneObjects<StereoPolyLineT, Mode::Immediate>() {
		if (this->target == nullptr)
			return;

		auto target = (StereoPolyLine*)this->target;

		if (target->GetVertices().size() > 0)
			target->RemoveVertice();

		this->target = nullptr;

		//existingPointsSelected.clear();
	}
	template<>
	void UnbindSceneObjects<StereoPolyLineT, Mode::Step>() {
		if (this->target == nullptr)
			return;

		auto target = (StereoPolyLine*)this->target;

		if (target->GetVertices().size() > 0)
			target->RemoveVertice();

		this->target = nullptr;

		//existingPointsSelected.clear();
	}

	bool BindCross(Cross* cross) {
		this->cross = cross;
		return true;
	}

	void SetMode(Mode mode) {
		DeleteConfig();
		this->mode = mode;
	}
};





template<ObjectType type>
class ExtrusionEditingTool : public EditingTool<ExtrusionEditingToolMode>, public CreatingTool<Mesh> {
#pragma region Types
	template<ObjectType type, Mode mode>
	struct Config : EditingTool::Config {

	};
	template<>
	struct Config<StereoPolyLineT, Mode::Immediate> : EditingTool::Config {
		//bool shouldCreateMesh = false;

		// If the cos between vectors is less than E
		// then we merge those vectors.
		double E = 1e-6;

		std::vector<glm::vec3> directingPoints;

	};
	template<>
	struct Config<StereoPolyLineT, Mode::Step> : EditingTool::Config {
		bool isPointCreated = false;
	};
#pragma endregion
	Log Logger = Log::For<ExtrusionEditingTool>();

	size_t inputHandlerId;
	size_t spaceModeChangeHandlerId;

	Mode mode;

	Cross* cross = nullptr;

	Mesh* mesh = nullptr;
	SceneObject* pen = nullptr;

	glm::vec3 crossStartPosition;
	glm::vec3 crossOriginalPosition;
	SceneObject* crossOriginalParent;

	template<Mode mode>
	Config<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new Config<type, mode>();

		return (Config<type, mode>*) config;
	}

	const glm::vec3 GetPos() {
		if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::Local)
			return cross->GetLocalPosition();

		return mesh->ToLocalPosition(cross->GetLocalPosition());
	}


#pragma region ProcessInput
	template<ObjectType type, Mode mode>
	void ProcessInput(Input* input) {
		std::cout << "Unsupported Editing Tool target Type or Unsupported combination of ObjectType and PointPenEditingToolMode" << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Mode::Immediate>(Input* input) {
		if (mesh == nullptr)
			return;

		if (input->IsDown(Key::Escape)) {
			UnbindSceneObjects();
			return;
		}

		auto& meshPoints = mesh->GetVertices();
		auto& penPoints = pen->GetVertices();
		auto transformVector = GetPos() - crossStartPosition;


		if (GetConfig<Mode::Immediate>()->directingPoints.size() < 1) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			GetConfig<Mode::Immediate>()->directingPoints.push_back(GetPos());

			mesh->AddVertice(penPoints[0] + transformVector);

			for (size_t i = 1; i < penPoints.size(); i++) {
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(i - 1, i);
			}

			return;
		}
		else if (GetConfig<Mode::Immediate>()->directingPoints.size() < 2) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			GetConfig<Mode::Immediate>()->directingPoints.push_back(GetPos());

			auto meshPointsSize = meshPoints.size();
			mesh->AddVertice(penPoints[0] + transformVector);
			mesh->Connect(meshPointsSize - penPoints.size(), meshPointsSize);

			for (size_t i = 1; i < penPoints.size(); i++) {
				meshPointsSize = meshPoints.size();
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(meshPointsSize - penPoints.size(), meshPointsSize);
				mesh->Connect(meshPointsSize - 1, meshPointsSize);
			}

			return;
		}

		auto E = GetConfig<Mode::Immediate>()->E;

		auto directingPoints = &GetConfig<Mode::Immediate>()->directingPoints;

		glm::vec3 r1 = GetPos() - (*directingPoints)[1];
		glm::vec3 r2 = (*directingPoints)[0] - (*directingPoints)[1];


		auto p = glm::dot(r1, r2);
		auto l1 = glm::length(r1);
		auto l2 = glm::length(r2);

		auto cos = p / l1 / l2;

		if (abs(cos) > 1 - E || isnan(cos)) {
			for (size_t i = 0; i < penPoints.size(); i++)
				mesh->SetVertice(meshPoints.size() - penPoints.size() + i, penPoints[i] + transformVector);

			(*directingPoints)[1] = GetPos();
		}
		else {
			auto meshPointsSize = meshPoints.size();
			mesh->AddVertice(penPoints[0] + transformVector);
			mesh->Connect(meshPointsSize - penPoints.size(), meshPointsSize);

			for (size_t i = 1; i < penPoints.size(); i++) {
				meshPointsSize = meshPoints.size();
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(meshPointsSize - penPoints.size(), meshPointsSize);
				mesh->Connect(meshPointsSize - 1, meshPointsSize);
			}

			directingPoints->erase(directingPoints->begin());
			directingPoints->push_back(GetPos());
		}

		//std::cout << "PointPen tool Immediate mode points count: " << meshPoints->size() << std::endl;
	}

	template<>
	void ProcessInput<StereoPolyLineT, Mode::Step>(Input* input) {
		if (mesh == nullptr)
			return;
		
		if (input->IsDown(Key::Escape)) {
			UnbindSceneObjects();
			return;
		}

		auto& meshPoints = mesh->GetVertices();
		auto& penPoints = pen->GetVertices();

		auto transformVector = GetPos() - crossStartPosition;

		if (!GetConfig<Mode::Step>()->isPointCreated) {
			mesh->AddVertice(penPoints[0] + transformVector);

			for (size_t i = 1; i < penPoints.size(); i++) {
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(i - 1, i);
			}

			GetConfig<Mode::Step>()->isPointCreated = true;

			return;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter)) {
			auto s = meshPoints.size();
			mesh->AddVertice(penPoints[0] + transformVector);
			mesh->Connect(s - penPoints.size(), s);

			for (size_t i = 1; i < penPoints.size(); i++) {
				s = meshPoints.size();
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(s - penPoints.size(), s);
				mesh->Connect(s - 1, s);
			}
			return;
		}

		for (size_t i = 0; i < penPoints.size(); i++)
			mesh->SetVertice(meshPoints.size() - penPoints.size() + i, penPoints[i] + transformVector);
	}

	void ProcessInput(Input* input) {
		switch (mode)
		{
		case Mode::Immediate:
			return ProcessInput<type, Mode::Immediate>(input);
		case Mode::Step:
			return ProcessInput<type, Mode::Step>(input);
		default:
			std::cout << "Not suported mode was given" << std::endl;
			return;
		}
	}
#pragma endregion
	template<ObjectType type, Mode mode>
	void UnbindSceneObjects() {

	}
	template<>
	void UnbindSceneObjects<StereoPolyLineT, Mode::Immediate>() {
		if (this->pen == nullptr)
			return;

		this->pen = nullptr;
	}
	template<>
	void UnbindSceneObjects<StereoPolyLineT, Mode::Step>() {
		if (this->pen == nullptr)
			return;

		auto& penPoints = pen->GetVertices();

		if (GetConfig<Mode::Step>()->isPointCreated && mesh->GetVertices().size() > 0) {
			if (mesh->GetVertices().size() > penPoints.size())
			{
				for (size_t i = 1; i < penPoints.size(); i++)
				{
					mesh->RemoveVertice();
					mesh->Disconnect(mesh->GetVertices().size() - penPoints.size(), mesh->GetVertices().size());
					mesh->Disconnect(mesh->GetVertices().size() - 1, mesh->GetVertices().size());
				}

				mesh->RemoveVertice();
				mesh->Disconnect(mesh->GetVertices().size() - penPoints.size(), mesh->GetVertices().size());
			}
			else
			{
				for (size_t i = 1; i < penPoints.size(); i++)
				{
					mesh->RemoveVertice();
					mesh->Disconnect(mesh->GetVertices().size() - 1, mesh->GetVertices().size());
				}

				mesh->RemoveVertice();
			}
		}

		this->pen = nullptr;
	}

	void ResetTool() {
		cross->SetParent(crossOriginalParent);
		cross->SetLocalPosition(crossOriginalPosition);

		GlobalToolConfiguration::OnSpaceModeChanged().RemoveHandler(spaceModeChangeHandlerId);
		keyBinding->RemoveHandler(inputHandlerId);

		DeleteConfig();
	}

	template<typename T>
	static int GetId() {
		static int i = 0;
		return i++;
	}


public:

	ExtrusionEditingTool() {
		func = [&, mesh = &mesh](Mesh* o) {
			std::stringstream ss;
			ss << o->GetDefaultName() << GetId<ExtrusionEditingTool<StereoPolyLineT>>();
			o->Name = ss.str();
			*mesh = o;

			o->SetWorldPosition(cross->GetWorldPosition());
			o->SetWorldRotation(pen->GetWorldRotation());

			if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::Local) {
				cross->SetParent(o);
				cross->SetLocalPosition(glm::vec3());
			}

			crossStartPosition = this->GetPos();
		};
	}

	virtual bool BindSceneObjects(std::vector<SceneObject*> objs) {
		if (pen != nullptr && !UnbindSceneObjects())
			return false;

		if (keyBinding == nullptr) {
			Logger.Error("KeyBinding wasn't assigned");
			return false;
		}

		if (objs[0]->GetType() != type) {
			Logger.Warning("Invalid Object passed to ExtrusionEditingTool");
			return true;
		}
		pen = objs[0];

		crossOriginalPosition = cross->GetLocalPosition();
		crossOriginalParent = const_cast<SceneObject*>(cross->GetParent());

		

		inputHandlerId = keyBinding->AddHandler([&](Input * input) { ProcessInput(input); });
		spaceModeChangeHandlerId = GlobalToolConfiguration::OnSpaceModeChanged().AddHandler([&](const SpaceMode& v) {
			if (!mesh)
				return;

			auto pos = cross->GetWorldPosition();
			
			if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::Local) {
				cross->SetParent(mesh);
				//cross->SetLocalPosition(mesh->ToLocalPosition(cross->GetLocalPosition()));
			}
			else {
				cross->SetParent(crossOriginalParent);
				//cross->SetLocalPosition(mesh->ToWorldPosition(cross->GetLocalPosition()));
			}

			cross->SetWorldPosition(pos);
			});
		return true;
	}
	virtual bool UnbindSceneObjects() {
		switch (mode)
		{
		case  Mode::Immediate:
			UnbindSceneObjects<type, Mode::Immediate>();
			break;
		case  Mode::Step:
			UnbindSceneObjects<type, Mode::Step>();
			break;
		default:
			std::cout << "Not suported mode was given" << std::endl;
			return false;
		}
		
		ResetTool();

		return true;
	}

	SceneObject** GetTarget() {
		return &pen;
	}
	virtual Type GetType() {
		return Type::Extrusion;
	}

	bool BindCross(Cross* cross) {
		return this->cross = cross;
	}

	void SetMode(Mode mode) {
		this->mode = mode;
	}

	bool Create() {
		if (pen == nullptr) {
			std::cout << "Pen was not assigned" << std::endl;
			return false;
		}

		DeleteConfig();

		return CreatingTool<Mesh>::Create();
	};
};


class TransformTool : public EditingTool<TransformToolMode> {
	const Log Logger = Log::For<TransformTool>();

	size_t inputHandlerId;
	size_t spaceModeChangeHandlerId;
	Mode mode;
	ObjectType type;

	Cross* cross = nullptr;
	SceneObject* target = nullptr;

	SceneObject* crossOriginalParent;

#pragma region ToolState
	float oldScale;
	glm::vec3 oldAngle;
	glm::vec3 transformOldPos;
	glm::fquat originalLocalRotation;

	bool areVerticesModified = false;
	bool areLinesModified = false;
	bool isPositionModified = false;
	bool isRotationModified = false;

	std::vector<std::vector<glm::vec3>> originalVerticesFolded;
	std::vector<glm::vec3> originalLocalPositionsFolded;
	std::vector<glm::vec3> originalWorldPositionsFolded;
	std::vector<std::vector<std::array<size_t, 2>>> originalLinesFolded;

#pragma endregion
	void ProcessInput(const ObjectType& type, const Mode& mode, Input* input) {
		if (target == nullptr)
			return;
		if (input->IsDown(Key::Escape)) {
			Cancel();
			return;
		}
		if (input->IsDown(Key::Enter)) {
			UnbindSceneObjects();
			return;
		}

		transformPos += input->movement;

		switch (mode) {
		case Mode::Translate:
			if (transformPos == transformOldPos)
				return;
			
			Translate(transformPos, target);
			transformOldPos = transformPos;
			return;
		case Mode::Scale:
			if (scale == oldScale && transformPos == transformOldPos)
				return;

			Scale(transformPos, scale, target);
			oldScale = scale;
			transformOldPos = transformPos;
			return;
		case Mode::Rotate:

			if (angle == oldAngle && transformPos == transformOldPos)
				return;

			Rotate(target);

			nullifyUntouchedAngles();

			oldAngle = angle;
			transformOldPos = transformPos;
			return;
		}

		Logger.Warning("Unsupported Editing Tool target Type or Unsupported combination of ObjectType and Transformation");
	}
	void Scale(glm::vec3 center, float scale, SceneObject* target) {
		isPositionModified = true;
		areVerticesModified = true;
		int v = 0;
		CallRecursive(target, [&](SceneObject* o) {
			if (o != target)
				o->SetLocalPosition(originalLocalPositionsFolded[v] * scale);
			for (size_t i = 0; i < o->GetVertices().size(); i++)
				o->SetVertice(i, originalVerticesFolded[v][i] * scale);
			v++;
			});
	}
	void Translate(glm::vec3 transformVector, SceneObject* target) {
		isPositionModified = true;
		
		if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::Local) {
			auto r = originalLocalPositionsFolded[0] + glm::rotate(target->GetWorldRotation(), transformVector);
			target->SetLocalPosition(r);
			return;
		}

		target->SetWorldPosition(originalLocalPositionsFolded[0] + transformVector);
	}
	void Rotate(SceneObject* target) {
		isRotationModified = true;

		auto i = getChangedAxe();
		if (i < 0) return;
		
		auto trimmedDeltaAngle = getTrimmedAngle((angle - oldAngle)[i]) * 3.1415926f * 2 / 360;

		auto axe = glm::vec3();
		axe[i] = 1;
		
		auto r = glm::angleAxis(trimmedDeltaAngle, axe);

		if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::World)
			target->SetWorldRotation(r * target->GetWorldRotation());
		else
			target->SetLocalRotation(target->GetLocalRotation() * r);
	}
	
	float getTrimmedAngle(float a) {
		while (a - 360 > 0)
			a -= 360;
		while (a + 360 < 0)
			a += 360;
		return a;
	}
	void nullifyUntouchedAngles() {
		auto da = angle - oldAngle;
		for (size_t i = 0; i < 3; i++)
			if (da[i] == 0)
				angle[i] = oldAngle [i] = 0;
	}
	int getChangedAxe() {
		auto da = angle - oldAngle;
		for (size_t i = 0; i < 3; i++)
			if (da[i] != 0)
				return i;

		return -1;
	}

	template<typename K, typename V>
	static bool exists(const std::multimap<K, V>& map, const K& key, const V& val) {
		auto h = map.equal_range(key);

		for (auto i = h.first; i != h.second; i++)
			if (i->second == val)
				return true;

		return false;
	}

	void CallRecursive(SceneObject* o, std::function<void(SceneObject*)> f) {
		f(o);
		for (auto c : o->children)
			CallRecursive(c, f);
	}
public:

	const std::multimap<ObjectType, Mode> supportedConfigs{
		{ Group, Mode::Translate },
		{ Group, Mode::Scale },
		{ Group, Mode::Rotate },
		{ StereoPolyLineT, Mode::Translate },
		{ StereoPolyLineT, Mode::Scale },
		{ StereoPolyLineT, Mode::Rotate },
		{ MeshT, Mode::Translate },
		{ MeshT, Mode::Scale },
		{ MeshT, Mode::Rotate },
	};

	float scale = 1;
	glm::vec3 angle;
	glm::vec3 transformPos;

	virtual bool BindSceneObjects(std::vector<SceneObject*> objs) {
		if (!UnbindSceneObjects())
			return false;

		if (keyBinding == nullptr) {
			Logger.Error("KeyBinding wasn't assigned");
			return false;
		}

		type = objs[0]->GetType();

		if (supportedConfigs.find(type) == supportedConfigs.end()) {
			Logger.Warning("Invalid Object passed to TransformTool");
			return true;
		}
		if (!exists(supportedConfigs, type, mode)) {
			Logger.Warning("Unsupported Mode for ObjectType");
			return true;
		}

		target = objs[0];
		

		

		originalLocalRotation = target->GetLocalRotation();
		crossOriginalParent = const_cast<SceneObject*>(cross->GetParent());
		cross->SetParent(target);
		if (GlobalToolConfiguration::GetSpaceMode() == SpaceMode::World)
			cross->SetWorldRotation(cross->unitQuat());

		originalVerticesFolded.clear();
		originalLinesFolded.clear();
		originalLocalPositionsFolded.clear();
		originalWorldPositionsFolded.clear();

		CallRecursive(target, [&](SceneObject* o) {
			originalVerticesFolded.push_back(o->GetVertices());
			originalLocalPositionsFolded.push_back(o->GetLocalPosition());
			originalWorldPositionsFolded.push_back(o->GetWorldPosition());
			if (o->GetType() == MeshT)
				originalLinesFolded.push_back(static_cast<Mesh*>(o)->GetLinearConnections());
			});

		keyBinding->RemoveHandler(cross->keyboardBindingHandlerId);
		inputHandlerId = keyBinding->AddHandler([this](Input* input) { this->ProcessInput(type, mode, input); });
		spaceModeChangeHandlerId = GlobalToolConfiguration::OnSpaceModeChanged().AddHandler([&](const SpaceMode& v) {
			transformOldPos = transformPos = oldAngle = angle = glm::vec3();
			originalLocalRotation = target->GetLocalRotation();
			originalLocalPositionsFolded[0] = target->GetLocalPosition();

			if (v == SpaceMode::Local)
				cross->SetLocalRotation(cross->unitQuat());
			else
				cross->SetWorldRotation(cross->unitQuat());
			});

		return true;
	}
	virtual bool UnbindSceneObjects() {
		if (this->target == nullptr)
			return true;

		isPositionModified = false;
		areVerticesModified = false;
		areLinesModified = false;
		isRotationModified = false;

		this->target = nullptr;

		scale = 1;
		oldScale = 1;
		angle = glm::vec3();
		oldAngle = glm::vec3();
		transformPos = glm::vec3();

		keyBinding->RemoveHandler(inputHandlerId);
		cross->keyboardBindingHandlerId = keyBinding->AddHandler(cross->keyboardBindingHandler);
		GlobalToolConfiguration::OnSpaceModeChanged().RemoveHandler(spaceModeChangeHandlerId);
		DeleteConfig();

		cross->SetParent(crossOriginalParent);
		cross->SetLocalRotation(cross->unitQuat());
		cross->SetLocalPosition(glm::vec3());

		return true;
	}
	void Cancel() {
		cross->SetParent(crossOriginalParent);

		int sceneObjectI = 0, meshI = 0;
		
		if (isRotationModified)
			target->SetLocalRotation(originalLocalRotation);

		CallRecursive(target, [=, &sceneObjectI, &meshI](SceneObject* o) {
			if (isPositionModified)
				o->SetLocalPosition(originalLocalPositionsFolded[sceneObjectI]);
			if (areVerticesModified)
				o->SetVertices(originalVerticesFolded[sceneObjectI]);
			if (o->GetType() == MeshT) {
				if (areLinesModified)
					static_cast<Mesh*>(o)->SetConnections(originalLinesFolded[meshI]);
				
				meshI++;
			}
			sceneObjectI++;
		});
	
		UnbindSceneObjects();
	}
	SceneObject** GetTarget() {
		return &target;
	}
	virtual Type GetType() {
		return Type::Transform;
	}
	bool BindCross(Cross* cross) {
		return this->cross = cross;
	}
	void SetMode(Mode mode) {
		UnbindSceneObjects();
		this->mode = mode;
	}
	const Mode& GetMode() {
		return mode;
	}
};