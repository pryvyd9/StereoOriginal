#pragma once

#include "GLLoader.hpp"
#include "DomainUtils.hpp"
#include "Commands.hpp"
#include "Input.hpp"
#include <map>
#include <stack>
#include <vector>
#include <queue>
#include "TemplateExtensions.hpp"
#include "ToolConfiguration.hpp"

class Tool {};

template<typename T>
class CreatingTool : Tool, public ISceneHolder {
	static std::stack<SceneObject*>& GetCreatedObjects() {
		static std::stack<SceneObject*> val;
		return val;
	}
public:
	std::function<void(T*)> func;

	ReadonlyProperty<PON> destination;

	bool Create() {
		StateBuffer::Commit();

		auto command = new CreateCommand();
		command->scene.BindAndApply(scene);
		command->destination = destination.Get().Get();
		command->func = [=] {
			T* obj = new T();
			func(obj);
			GetCreatedObjects().push(obj);

			return obj;
		};

		return true;
	}
};

template<typename EditMode>
class EditingTool : Tool {
	static size_t& isBeingModifiedHandler() {
		static size_t v;
		return v;
	}
protected:
	using Mode = EditMode;

	struct Config {
		template<typename T>
		T* Get() { return (T*)this; };
	};

	Config* config = nullptr;

	static bool& isBeingModified() {
		static bool v;
		return v;
	}

	//static bool& isBeingContinuouslyModified() {
	//	static bool v;
	//	return v;
	//}


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

	ReadonlyProperty<KeyBinding*> keyBinding;

	EditingTool() {
		if (isBeingModifiedHandler() != 0)
			return;

		isBeingModifiedHandler() = SceneObject::OnBeforeAnyElementChanged().AddHandler([&] {
			isBeingModified() = true;
			});
	}

	virtual Type GetType() = 0;
	virtual bool BindSceneObjects(std::vector<PON> obj) = 0;
	virtual bool UnbindSceneObjects() = 0;
};

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
	size_t stateChangedHandlerId;
	size_t anyObjectChangedHandlerId;

	Mode mode;

	PON target;

	glm::vec3 crossOriginalPosition;
	SceneObject* crossOriginalParent;
	bool wasCommitDone = false;


	template<Mode mode>
	Config<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new Config<type, mode>();

		return (Config<type, mode>*) config;
	}

	const glm::vec3 GetPos() {
		if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::Local)
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
		if (!input->IsContinuousInputOneSecondDelay()) {
			isBeingModified() = false;
			wasCommitDone = false;
		}

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


		if (!GetConfig<Mode::Step>()->isPointCreated || points.empty()) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(GetPos());
			GetConfig<Mode::Step>()->isPointCreated = true;
			return;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter)) {
			isBeingModified() = false;
			wasCommitDone = false;
			target->AddVertice(GetPos());

			return;
		}

		if (pointsCount > 0 && GetPos() == points.back())
			return;

		target->SetVertice(points.size() - 1, GetPos());
	}

	void ProcessInput(Input* input) {
		if (!target.HasValue()) {
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
	ReadonlyProperty<Cross*> cross;
	std::vector<std::function<void()>> onTargetReleased;

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (target.HasValue() && !UnbindSceneObjects())
			return false;

		if (!keyBinding.Get()) {
			Logger.Warning("KeyBinding wasn't assigned");
			return false;
		}

		if (objs[0]->GetType() != type) {
			Logger.Warning("Invalid Object passed to PointPenEditingTool");
			return true;
		}
		target = objs[0];

		crossOriginalPosition = cross->GetLocalPosition();
		crossOriginalParent = const_cast<SceneObject*>(cross->GetParent());

		if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::Local) {
			cross->SetParent(target.Get(), false, true, true, false);
			if (target->GetVertices().size() > 0)
				cross->SetLocalPosition(target->GetVertices().back());
			else
				cross->SetLocalPosition(glm::vec3());
		}
		else {
			if (target->GetVertices().size() > 0)
				cross->SetWorldPosition(target->ToWorldPosition(target->GetVertices().back()));
			else
				cross->SetWorldPosition(target->GetWorldPosition());
		}

		inutHandlerId = keyBinding->AddHandler([&](Input * input) { ProcessInput(input); });
		spaceModeChangeHandlerId = GlobalToolConfiguration::SpaceMode().OnChanged().AddHandler([&](const SpaceMode& v) {
			if (v == SpaceMode::Local) {
				cross->SetParent(target.Get(), false, true, true, false);
				if (target->GetVertices().size() > 0)
					cross->SetLocalPosition(target->GetVertices().back());
				else
					cross->SetLocalPosition(glm::vec3());
			}
			else {
				cross->SetParent(crossOriginalParent, false, true, true, false);
				if (target->GetVertices().size() > 0)
					cross->SetWorldPosition(target->ToWorldPosition(target->GetVertices().back()));
				else
					cross->SetWorldPosition(target->GetWorldPosition());
			}
			});
		stateChangedHandlerId = StateBuffer::OnStateChange().AddHandler([&] {
			cross->SetParent(target.Get(), false, true, true, false);
			if (!target.HasValue() || target->GetVertices().empty())
				cross->SetLocalPosition(glm::vec3());
			else
				cross->SetLocalPosition(target->GetVertices().back());
			});
		anyObjectChangedHandlerId = SceneObject::OnBeforeAnyElementChanged().AddHandler([&] {
			if (wasCommitDone)
				return;

			StateBuffer::Commit();
			wasCommitDone = true;
			Logger.Information("commit");
			});

		return true;
	}
	virtual bool UnbindSceneObjects() {
		if (!target.HasValue())
			return true;

		target->RemoveVertice();

		for (auto func : onTargetReleased)
			func();

		cross->SetParent(crossOriginalParent, false, true, true, false);
		cross->SetLocalPosition(crossOriginalPosition);
		keyBinding->RemoveHandler(inutHandlerId);
		GlobalToolConfiguration::SpaceMode().OnChanged().RemoveHandler(spaceModeChangeHandlerId);
		StateBuffer::OnStateChange().RemoveHandler(stateChangedHandlerId);
		SceneObject::OnBeforeAnyElementChanged().RemoveHandler(anyObjectChangedHandlerId);

		DeleteConfig();

		return true;
	}
	virtual Type GetType() {
		return Type::PointPen;
	}
	SceneObject* GetTarget() {
		return target.HasValue()
			? target.Get()
			: nullptr;
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
	size_t stateChangedHandlerId;
	size_t anyObjectChangedHandlerId;

	Mode mode;

	PON mesh = nullptr;
	PON pen = nullptr;

	bool wasCommitDone = false;

	glm::vec3 crossStartPosition;
	glm::vec3 crossOldPosition;
	glm::vec3 crossOriginalPosition;
	SceneObject* crossOriginalParent;

	template<Mode mode>
	Config<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new Config<type, mode>();

		return (Config<type, mode>*) config;
	}

	const glm::vec3 GetPos() {
		if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::Local)
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
		if (!input->IsContinuousInputOneSecondDelay()) {
			isBeingModified() = false;
			wasCommitDone = false;
		}

		if (!mesh.HasValue() || crossOldPosition == GetPos())
			return;

		if (input->IsDown(Key::Escape)) {
			UnbindSceneObjects();
			return;
		}

		auto& meshPoints = mesh->GetVertices();
		auto& penPoints = pen->GetVertices();
		auto transformVector = GetPos() - crossStartPosition;
		auto mesh = (Mesh*)this->mesh.Get();

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

		crossOldPosition = GetPos();
		//std::cout << "PointPen tool Immediate mode points count: " << meshPoints->size() << std::endl;
	}

	template<>
	void ProcessInput<StereoPolyLineT, Mode::Step>(Input* input) {
		if (!mesh.HasValue())
			return;
		
		if (input->IsDown(Key::Escape)) {
			UnbindSceneObjects();
			return;
		}

		auto& meshPoints = mesh->GetVertices();
		auto& penPoints = pen->GetVertices();
		auto mesh = (Mesh*)this->mesh.Get();

		auto transformVector = GetPos() - crossStartPosition;

		if (!GetConfig<Mode::Step>()->isPointCreated || meshPoints.empty()) {
			mesh->AddVertice(penPoints[0] + transformVector);

			for (size_t i = 1; i < penPoints.size(); i++) {
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(i - 1, i);
			}

			GetConfig<Mode::Step>()->isPointCreated = true;

			return;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter)) {
			isBeingModified() = false;
			wasCommitDone = false;

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
		if (!pen.HasValue())
			return;

		//this->pen = nullptr;
	}
	template<>
	void UnbindSceneObjects<StereoPolyLineT, Mode::Step>() {
		if (!pen.HasValue())
			return;

		auto& penPoints = pen->GetVertices();
		auto mesh = (Mesh*)this->mesh.Get();

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

		//this->pen = nullptr;
	}

	void ResetTool() {
		cross->SetParent(crossOriginalParent);
		cross->SetLocalPosition(crossOriginalPosition);

		GlobalToolConfiguration::SpaceMode().OnChanged().RemoveHandler(spaceModeChangeHandlerId);
		keyBinding->RemoveHandler(inputHandlerId);
		StateBuffer::OnStateChange().RemoveHandler(stateChangedHandlerId);
		SceneObject::OnBeforeAnyElementChanged().RemoveHandler(anyObjectChangedHandlerId);

		DeleteConfig();
	}

	template<typename T>
	static int GetId() {
		static int i = 0;
		return i++;
	}


public:
	ReadonlyProperty<Cross*> cross;

	ExtrusionEditingTool() {
		func = [&, mesh = &mesh](Mesh* o) {
			std::stringstream ss;
			ss << o->GetDefaultName() << GetId<ExtrusionEditingTool<StereoPolyLineT>>();
			o->Name = ss.str();
			*mesh = o;

			o->SetWorldPosition(cross->GetWorldPosition());
			o->SetWorldRotation(pen->GetWorldRotation());

			if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::Local) {
				cross->SetParent(o, false, true, true, false);
				cross->SetLocalPosition(glm::vec3());
			}

			crossStartPosition = this->GetPos();
		};
	}

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (pen.HasValue() && !UnbindSceneObjects())
			return false;

		if (!keyBinding.Get()) {
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
		stateChangedHandlerId = StateBuffer::OnStateChange().AddHandler([&] {
			cross->SetParent(mesh.Get(), false, true, true, false);
			cross->SetLocalPosition(glm::vec3());
			});
		anyObjectChangedHandlerId = SceneObject::OnBeforeAnyElementChanged().AddHandler([&] {
			if (wasCommitDone)
				return;

			StateBuffer::Commit();
			wasCommitDone = true;
			//Logger.Information("commit");
			});

		

		inputHandlerId = keyBinding->AddHandler([&](Input * input) { ProcessInput(input); });
		spaceModeChangeHandlerId = GlobalToolConfiguration::SpaceMode().OnChanged().AddHandler([&](const SpaceMode& v) {
			if (!mesh.HasValue())
				return;

			auto pos = cross->GetWorldPosition();
			
			if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::Local)
				cross->SetParent(mesh.Get(), false, true, true, false);
			else
				cross->SetParent(crossOriginalParent, false, true, true, false);

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

	SceneObject* GetTarget() {
		return pen.HasValue()
			? pen.Get()
			: nullptr;
	}
	virtual Type GetType() {
		return Type::Extrusion;
	}

	void SetMode(Mode mode) {
		this->mode = mode;
	}

	bool Create() {
		if (!pen.HasValue()) {
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
	size_t stateChangedHandlerId;
	size_t anyObjectChangedHandlerId;

	Mode mode;
	ObjectType type;

	Scene::CategorizedPON targets;

	float minScale = 0.01;
	float oldScale = 1;
	glm::vec3 oldAngle;
	glm::vec3 transformOldPos;

	int oldAxeId;

	bool wasCommitDone = false;
	
	void ProcessInput(const ObjectType& type, const Mode& mode, Input* input) {
		static glm::vec3 zero = glm::vec3();
		//if (SceneObjectSelection::Selected().empty())
		//	return;

		if (!input->IsContinuousInputOneSecondDelay()) {
			isBeingModified() = false;
			wasCommitDone = false;
		}

		// We can move cross without having any objects bind.
		if (!input->IsContinuousInputNoDelay()) {
			transformPos = transformOldPos = zero;
			angle = oldAngle = zero;
			scale = oldScale = 1;
		}

		glm::vec3 relativeMovement;
		if (input->movement == zero)
			relativeMovement = transformPos - transformOldPos;
		else
			relativeMovement = input->movement;

		// Check if we need to process tool.
		if (targets.parentObjects.empty() || !targets.parentObjects.front().HasValue()) {
			MoveCross(relativeMovement);
			return;
		}
		if (input->IsDown(Key::Escape)) {
			MoveCross(relativeMovement);
			UnbindSceneObjects();
			return;
		}

		switch (mode) {
		case Mode::Translate:
			if (relativeMovement == zero)
				return;
			
			Translate(relativeMovement, targets.parentObjects);
			break;
		case Mode::Scale:
			MoveCross(relativeMovement);
			if (scale == oldScale || abs(scale) < minScale)
				return;

			Scale(cross->GetWorldPosition(), oldScale, scale, targets.parentObjects);
			oldScale = scale;
			break;
		case Mode::Rotate:
			MoveCross(relativeMovement);
			if (angle == oldAngle)
				return;

			Rotate(cross->GetWorldPosition(), targets.parentObjects);
			nullifyUntouchedAngles();
			oldAngle = angle;
			break;
		default:
			Logger.Warning("Unsupported Editing Tool target Type or Unsupported combination of ObjectType and Transformation");
			break;
		}

		transformOldPos = transformPos;
		cross->ForceUpdateCache();
	}
	void Scale(const glm::vec3& center, const float& oldScale, const float& scale, std::list<PON>& targets) {
		for (auto target : targets) {
			int v = 0;
			target->CallRecursive([&](SceneObject* o) {
				o->SetWorldPosition((o->GetWorldPosition() - center) / oldScale * scale + center);
				for (size_t i = 0; i < o->GetVertices().size(); i++)
					o->SetVertice(i, o->GetVertices()[i] / oldScale * scale);
				v++;
				});
		}
	}
	void Translate(const glm::vec3& transformVector, std::list<PON>& targets) {
		// Need to calculate average rotation.
		// https://stackoverflow.com/questions/12374087/average-of-multiple-quaternions/27410865#27410865
		if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::Local && haveSingleParent(targets)) {
			auto r = glm::rotate(targets.front()->GetWorldRotation(), transformVector);
			
			MoveCross(r);
			for (auto o : targets)
				o->SetWorldPosition(o->GetWorldPosition() + r);

			return;
		}

		MoveCross(transformVector);
		for (auto o : targets)
			o->SetWorldPosition(o->GetWorldPosition() + transformVector);
	}
	void Rotate(const glm::vec3& center, std::list<PON>& targets) {
		auto i = getChangedAxe();
		if (i < 0) return;

		if (oldAxeId != i && GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::World)
			cross->SetWorldRotation(cross->unitQuat());

		oldAxeId = i;

		auto trimmedDeltaAngle = getTrimmedAngle((angle - oldAngle)[i]) * 3.1415926f * 2 / 360;

		auto axe = glm::vec3();
		axe[i] = 1;

		auto r = glm::angleAxis(trimmedDeltaAngle, axe);

		auto crossOldRotation = cross->GetLocalRotation();
		cross->SetLocalRotation(cross->GetLocalRotation() * r);

		if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::World)
			for (auto& target : targets) {
				target->SetWorldPosition(glm::rotate(r, target->GetWorldPosition() - center) + center);
				target->SetWorldRotation(r * target->GetWorldRotation());
			}
		else {
			auto rIsolated = cross->GetLocalRotation() * glm::inverse(crossOldRotation);
			for (auto& target : targets) {
				// Rotate relative to cross.
				target->SetWorldPosition(glm::rotate(rIsolated, target->GetWorldPosition() - center) + center);
				target->SetLocalRotation(target->GetLocalRotation() * r);
			}
		}
	}

	void MoveCross(const glm::vec3& movement) {
		cross->SetLocalPosition(cross->GetLocalPosition() + movement);
	}


	bool haveSingleParent(std::list<PON>& targets) {
		auto firstParent = targets.front()->GetParent();
		
		// First object check is redundant.
		for (auto& o : targets)
			if (o->GetParent() != firstParent)
				return false;
		
		return true;
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


	static glm::vec3 Avg(std::vector<glm::vec3> vs) {
		glm::vec3 sum = glm::vec3();
		for (auto o : vs)
			sum += o;
		return glm::vec3(sum.x / vs.size(), sum.y / vs.size(), sum.z / vs.size());
	}
	static std::vector<glm::vec3> GetWorldPositions(std::list<PON>& vs) {
		std::vector<glm::vec3> l;
		for (auto o : vs)
			l.push_back(o->GetWorldPosition());
		return l;
	}

	//static std::vector<glm::quat> GetLocalRotations(std::list<PON>& vs) {
	//	std::vector<glm::quat> l;
	//	for (auto o : vs)
	//		l.push_back(o->GetLocalRotation());
	//	return l;
	//}
	//static glm::quat Avg(std::vector<glm::quat> vs) {
	//	glm::mat<4, vs.size(), float> m;
	//}
public:
	ReadonlyProperty<Cross*> cross;

	float scale = 1;
	glm::vec3 angle;
	glm::vec3 transformPos;
	bool isRelativeMode;

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (!UnbindSceneObjects())
			return false;

		if (keyBinding.Get() == nullptr) {
			Logger.Error("KeyBinding wasn't assigned");
			return false;
		}

		targets.childObjects.clear();
		targets.orphanedObjects.clear();
		targets.parentObjects.clear();
		Scene::CategorizeObjects(StateBuffer::RootObject().Get().Get(), objs, targets);

		cross->SetLocalPosition(Avg(GetWorldPositions(targets.parentObjects)));
		if (GlobalToolConfiguration::SpaceMode().Get() == SpaceMode::World)
			cross->SetWorldRotation(cross->unitQuat());
		else if (!targets.parentObjects.empty() && targets.parentObjects.front().HasValue())
			cross->SetLocalRotation(targets.parentObjects.front()->GetWorldRotation());

		keyBinding->RemoveHandler(cross->keyboardBindingHandlerId);
		inputHandlerId = keyBinding->AddHandler([this](Input* input) { this->ProcessInput(type, mode, input); });
		stateChangedHandlerId = StateBuffer::OnStateChange().AddHandler([&] { 
			cross->SetLocalPosition(Avg(GetWorldPositions(targets.parentObjects)));
			});
		anyObjectChangedHandlerId = SceneObject::OnBeforeAnyElementChanged().AddHandler([&] {
			if (wasCommitDone)
				return;

			StateBuffer::Commit();
			wasCommitDone = true;
			//Logger.Information("commit");
			});
		spaceModeChangeHandlerId = GlobalToolConfiguration::SpaceMode().OnChanged().AddHandler([&](const SpaceMode& v) {
			transformOldPos = transformPos = oldAngle = angle = glm::vec3();
			oldAngle = angle = glm::vec3();

			if (v == SpaceMode::Local) {
				if (!targets.parentObjects.empty() && targets.parentObjects.front().HasValue())
					cross->SetLocalRotation(targets.parentObjects.front()->GetWorldRotation());
				//cross->SetLocalRotation(cross->unitQuat());
			}
			else
				cross->SetWorldRotation(cross->unitQuat());
			});

		return true;
	}
	virtual bool UnbindSceneObjects() {
		if (targets.parentObjects.empty())
			return true;

		scale = 1;
		oldScale = 1;
		angle = glm::vec3();
		oldAngle = glm::vec3();
		transformPos = glm::vec3();

		keyBinding->RemoveHandler(inputHandlerId);
		cross->keyboardBindingHandlerId = keyBinding->AddHandler(cross->keyboardBindingHandler);
		GlobalToolConfiguration::SpaceMode().OnChanged().RemoveHandler(spaceModeChangeHandlerId);
		StateBuffer::OnStateChange().RemoveHandler(stateChangedHandlerId);
		SceneObject::OnBeforeAnyElementChanged().RemoveHandler(anyObjectChangedHandlerId);
		DeleteConfig();

		cross->SetLocalRotation(cross->unitQuat());
		cross->SetLocalPosition(glm::vec3());

		return true;
	}
	SceneObject* GetTarget() {
		if (targets.parentObjects.empty() || !targets.parentObjects.front().HasValue())
			return nullptr;
		else 
			return targets.parentObjects.front().Get();
	}
	virtual Type GetType() {
		return Type::Transform;
	}
	void SetMode(Mode mode) {
		this->mode = mode;
	}
	const Mode& GetMode() {
		return mode;
	}
};