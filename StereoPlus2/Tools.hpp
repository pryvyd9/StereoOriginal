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
#include "Settings.hpp"
#include "Math.hpp"

class Tool {
protected:
	static size_t& isBeingModifiedHandler() {
		static size_t v;
		return v;
	}
	static bool& isAnyActive() {
		static bool v;
		return v;
	}
	static size_t& selectionChangedhandlerId() {
		static size_t v;
		return v;
	}
	static Tool*& activeTool() {
		static Tool* v = nullptr;
		return v;
	}
};

template<typename T>
class CreatingTool : Tool {
public:
	std::function<void(T*)> init;
	std::function<void(SceneObject*)> onCreated = [](SceneObject*) {};

	Property<PON> destination;

	T* Create() {
		T* obj = new T();

		auto command = new CreateCommand();
		command->destination = destination.Get().Get();
		command->init = [=] {
			init(obj);
			return obj;
		};
		command->onCreated = onCreated;

		return obj;
	}
};

class CloneTool : Tool {
	
public:
	PON destination;
	PON target;

	std::function<void(SceneObject*)> init;
	std::function<void(SceneObject*)> onCreated = [](SceneObject*) {};

	SceneObject* Create() {
		// Clone the object before it's modified
		// and initialize later.
		auto clone = target->Clone();

		auto command = new CreateCommand();
		if (destination.HasValue())
			command->destination = destination.Get();
		else
			command->destination = Scene::root().Get().Get();
		command->init = [clone = clone, init = init] {
			init(clone);
			return clone;
		};
		command->onCreated = onCreated;

		return clone;
	}
};

class EditingTool : Tool {
protected:
	static bool& isBeingModified() {
		static bool v;
		return v;
	}
	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) {}
	virtual void OnToolActivated(const ObjectSelection::Selection& v) { OnSelectionChanged(v); }
public:
	EditingTool() {
		if (isBeingModifiedHandler() != 0)
			return;

		isBeingModifiedHandler() = SceneObject::OnBeforeAnyElementChanged().AddHandler([&] {
			isBeingModified() = true;
			});
	}

	virtual bool BindSceneObjects(std::vector<PON> obj) { return true; };
	virtual bool UnbindSceneObjects() { return true; };

	virtual void Activate() {
		if (isAnyActive())
			Deactivate();

		selectionChangedhandlerId() = ObjectSelection::OnChanged() += [&](const ObjectSelection::Selection& v) {
			OnSelectionChanged(v);
		};

		isAnyActive() = true;

		OnToolActivated(ObjectSelection::Selected());
	}
	static void Deactivate() {
		if (!isAnyActive())
			return;

		ObjectSelection::OnChanged() -= selectionChangedhandlerId();
		selectionChangedhandlerId() = 0;
		isAnyActive() = false;
	}

	virtual void UnbindTool() = 0;
};

template<typename EditMode>
class EditingToolConfigured : public EditingTool {
protected:
	using Mode = EditMode;

	struct Config {
		template<typename T>
		T* Get() { return (T*)this; };
	};

	Config* config = nullptr;

	void DeleteConfig() {
		if (config != nullptr)
		{
			delete config;
			config = nullptr;
		}
	}
};

class PenTool : public EditingTool {
	using Mode = PointPenEditingToolMode;

	const Log Logger = Log::For<PenTool>();

	size_t inputHandlerId;
	size_t stateChangedHandlerId;

	// Create new object by pressing Enter.
	size_t createNewObjectHandlerId;
	bool lockCreateNewObjectHandlerId;

	Mode mode;
	PON target;

	bool createdAdditionalPoints;
	bool createdNewObject;
	
	// If the cos between vectors is less than E
	// then we merge those vectors.
	float E = 1e-6;

	// If distance between previous point and 
	// cursor is less than this number 
	// then don't create a new point.
	double Precision = 1e-4;

	void Immediate() {
		createdNewObject = false;

		if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			Changes::Commit();
			SceneObject::ResetIsAnyElementChanged();

			UnbindSceneObjects();
			TryCreateNewObject();
			return;
		}
		else if (Input::HasContinuousMovementInputNoDelayStopped() && SceneObject::IsAnyElementChanged()) {
			Changes::Commit();
			SceneObject::ResetIsAnyElementChanged();
		}

		auto& points = target->GetVertices();
		auto pointsCount = points.size();

		// Create 2 points to be able to measure distance between them.
		if (pointsCount < 2) {
			target->AddVertice(cross->GetLocalPosition());
			return;
		}

		// If cross is located at distance less than Precision then don't do anything.
		if (glm::length(cross->GetWorldPosition() - points[pointsCount - 2]) < Precision)
			return;

		// Create the third point to be able to measure the angle between the last 3 points.
		if (pointsCount < 3) {
			target->AddVertice(cross->GetWorldPosition());
			return;
		}

		// If the line goes straight then instead of adding 
		// a new point - move the previous point to current cross position.
		if (IsStraightLineMaintained(
			points[pointsCount - 3], 
			points[pointsCount - 2], 
			points[pointsCount - 1], 
			cross->GetWorldPosition(),
			E)) {
			target->SetVertice(pointsCount - 2, cross->GetWorldPosition());
			target->SetVertice(pointsCount - 1, cross->GetWorldPosition());
		}
		else
			target->AddVertice(cross->GetWorldPosition());
	}
	void Step() {
		if (!createdAdditionalPoints || target->GetVertices().empty()) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(cross->GetWorldPosition());
			createdAdditionalPoints = true;
			return;
		}

		if (createdNewObject || Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			Changes::Commit();
			createdNewObject = false;
			target->AddVertice(cross->GetWorldPosition());
			return;
		}

		if (cross->GetWorldPosition() == target->GetVertices().back())
			return;

		target->SetVertice(target->GetVertices().size() - 1, cross->GetWorldPosition());
	}

	// Determines if the line will continue being straight after adding v4 to v1,v2,v3.
	static bool IsStraightLineMaintained(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const float E) {
		glm::vec3 r1 = v4 - v3;
		glm::vec3 r2 = v1 - v2;

		auto p = glm::dot(r1, r2);
		auto l1 = glm::length(r1);
		auto l2 = glm::length(r2);

		auto cos = p / l1 / l2;

		// Use abs(cos) to check if the line is maintained while moving backwards.
		return cos > 1 - E || isnan(cos);
	}

	void ProcessInput() {
		if (!target.HasValue()) {
			UnbindSceneObjects();
			return;
		}

		switch (mode)
		{
		case Mode::Immediate: return Immediate();
		case Mode::Step: return Step();
		default:
			Logger.Warning("Not suported mode was given");
			return;
		}
	}

	void TryCreateNewObject() {
		if (createNewObjectHandlerId)
			return;

		createNewObjectHandlerId = Input::AddHandler([&] {
			if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
				lockCreateNewObjectHandlerId = true;

				auto cmd = new CreateCommand();

				if (ObjectSelection::Selected().size() == 1) {
					auto d = ObjectSelection::Selected().begin()._Ptr->_Myval.Get();
					cmd->destination = d->GetParent() ? const_cast<SceneObject*>(d->GetParent()) : d;
				}

				cmd->init = [] {
					auto o = new PolyLine();
					o->SetWorldPosition(Scene::cross()->GetWorldPosition());
					o->SetWorldRotation(Scene::cross()->GetWorldRotation());
					Scene::AssignUniqueName(o, "PolyLine");
					return o;
				};
				cmd->onCreated = [&](SceneObject* o) {
					createdNewObject = true;
					ObjectSelection::Set(o);
					Input::RemoveHandler(createNewObjectHandlerId);
					lockCreateNewObjectHandlerId = false;
					Changes::Commit();
				};
			}
			});
	}

	ObjectSelection::Selection GetExistingObjects(const ObjectSelection::Selection& v) {
		ObjectSelection::Selection ns;
		for (auto& o : Scene::Objects().Get())
			if (v.find(o) != v.end())
				ns.insert(o);

		return ns;
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		UnbindTool();
		OnToolActivated(v);
	}
	virtual void OnToolActivated(const ObjectSelection::Selection& v) override {
		// Don't work with deleted objects even if they stay selected.
		auto targets = GetExistingObjects(v);
		Settings::ShouldRestrictTargetModeToPivot() = true;

		if (targets.empty())
			return TryCreateNewObject();

		if (targets.size() > 1) {
			Logger.Warning("Cannot work with multiple objects");
			return;
		}

		auto t = targets.begin()._Ptr->_Myval;

		if (t->GetType() != PolyLineT)
			return TryCreateNewObject();

		createdAdditionalPoints = false;

		target = t;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetWorldRotation(target.Get()->GetWorldRotation());

		if (!target->GetVertices().empty())
			cross->SetWorldPosition(target->GetVertices().back());

		inputHandlerId = Input::AddHandler([&] {
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput();
			});
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetWorldRotation(glm::quat());
				return;
			}

			if (!exists(Scene::Objects().Get(), target)) {
				target = PON();
				return;
			}

			if (target->GetVertices().empty())
				return;

			cross->SetWorldPosition(target->GetVertices().back());
		};
	}

public:
	NonAssignProperty<Cross*> cross;

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (target.HasValue() && !UnbindSceneObjects())
			return false;

		if (objs[0]->GetType() != PolyLineT) {
			Logger.Warning("Invalid Object passed to PointPenEditingTool");
			return true;
		}
		target = objs[0];

		createdAdditionalPoints = false;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetWorldRotation(target.Get()->GetWorldRotation());

		if (!target->GetVertices().empty())
			cross->SetWorldPosition(target->GetVertices().back());

		inputHandlerId = Input::AddHandler([&] {
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput();
			});
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetWorldRotation(glm::quat());
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetWorldRotation(target.Get()->GetWorldRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetWorldPosition(target->GetVertices().back());
		};
		Settings::ShouldRestrictTargetModeToPivot() = true;

		return true;
	}
	virtual bool UnbindSceneObjects() {
		if (!lockCreateNewObjectHandlerId)
			Input::RemoveHandler(createNewObjectHandlerId);
		
		Settings::ShouldRestrictTargetModeToPivot() = false;

		if (!target.HasValue())
			return true;

		if (mode == Mode::Step && createdAdditionalPoints) {
			target->RemoveVertice();
			createdAdditionalPoints = false;
		}
	
		Input::RemoveHandler(inputHandlerId);
		Changes::OnStateChange() -= stateChangedHandlerId;

		target = PON();
		
		// Go to creating mode after unbinding target;
		TryCreateNewObject();

		return true;
	}
	virtual void UnbindTool() override {
		UnbindSceneObjects();

		// On unbinding objects the tool goes to 
		// creating new object awaiting state.
		// But if the tool is unbind then it should be cancelled.
		if (!lockCreateNewObjectHandlerId)
			Input::RemoveHandler(createNewObjectHandlerId);
	}

	SceneObject* GetTarget() {
		return target.HasValue()
			? target.Get()
			: nullptr;
	}

	void SetMode(Mode mode) {
		this->mode = mode;
	}
};

template<ObjectType type>
class ExtrusionEditingTool : public EditingToolConfigured<ExtrusionEditingToolMode>, public CreatingTool<Mesh> {
#pragma region Types
	template<ObjectType type, Mode mode>
	struct Config : EditingToolConfigured::Config {

	};
	template<>
	struct Config<PolyLineT, Mode::Immediate> : EditingToolConfigured::Config {
		//bool shouldCreateMesh = false;

		// If the cos between vectors is less than E
		// then we merge those vectors.
		double E = 1e-6;

		std::vector<glm::vec3> directingPoints;

	};
	template<>
	struct Config<PolyLineT, Mode::Step> : EditingToolConfigured::Config {
		bool isPointCreated = false;
	};
#pragma endregion
	Log Logger = Log::For<ExtrusionEditingTool>();

	size_t inputHandlerId;
	size_t spaceModeChangeHandlerId;
	size_t stateChangedHandlerId;

	Mode mode;

	PON mesh = nullptr;
	PON pen = nullptr;

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
		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			return cross->GetLocalPosition();

		return mesh->ToLocalPosition(cross->GetLocalPosition());
	}


#pragma region ProcessInput
	template<ObjectType type, Mode mode>
	void ProcessInput() {
		std::cout << "Unsupported Editing Tool target Type or Unsupported combination of ObjectType and PointPenEditingToolMode" << std::endl;
	}
	template<>
	void ProcessInput<PolyLineT, Mode::Immediate>() {
		if (Input::HasContinuousMovementInputNoDelayStopped() && SceneObject::IsAnyElementChanged()) {
			Changes::Commit();
			SceneObject::ResetIsAnyElementChanged();
		}

		if (!mesh.HasValue() || crossOldPosition == GetPos())
			return;

		if (Input::IsDown(Key::Escape, true)) {
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
	void ProcessInput<PolyLineT, Mode::Step>() {
		if (!mesh.HasValue())
			return;
		
		if (Input::IsDown(Key::Escape, true)) {
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

		if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			auto s = meshPoints.size();
			mesh->AddVertice(penPoints[0] + transformVector);
			mesh->Connect(s - penPoints.size(), s);

			for (size_t i = 1; i < penPoints.size(); i++) {
				s = meshPoints.size();
				mesh->AddVertice(penPoints[i] + transformVector);
				mesh->Connect(s - penPoints.size(), s);
				mesh->Connect(s - 1, s);
			}

			Changes::Commit();
			return;
		}

		for (size_t i = 0; i < penPoints.size(); i++)
			mesh->SetVertice(meshPoints.size() - penPoints.size() + i, penPoints[i] + transformVector);
	}

	void ProcessInput() {
		switch (mode)
		{
		case Mode::Immediate:
			return ProcessInput<type, Mode::Immediate>();
		case Mode::Step:
			return ProcessInput<type, Mode::Step>();
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
	void UnbindSceneObjects<PolyLineT, Mode::Immediate>() {
		if (!pen.HasValue())
			return;
	}
	template<>
	void UnbindSceneObjects<PolyLineT, Mode::Step>() {
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
	}

	void ResetTool() {
		cross->SetParent(crossOriginalParent);
		cross->SetLocalPosition(crossOriginalPosition);

		Settings::SpaceMode().OnChanged() -= spaceModeChangeHandlerId;
		Input::RemoveHandler(inputHandlerId);
		Changes::OnStateChange().RemoveHandler(stateChangedHandlerId);

		DeleteConfig();
	}

	template<typename T>
	static int GetId() {
		static int i = 0;
		return i++;
	}

	virtual void OnToolActivated(const ObjectSelection::Selection& v) override { 
		Settings::ShouldRestrictTargetModeToPivot() = true;
	}

public:
	NonAssignProperty<Cross*> cross;

	ExtrusionEditingTool() {
		init = [&, mesh = &mesh](Mesh* o) {
			std::stringstream ss;
			ss << o->GetDefaultName() << GetId<ExtrusionEditingTool<PolyLineT>>();
			o->Name = ss.str();
			*mesh = o;

			o->SetWorldPosition(cross->GetWorldPosition());
			o->SetWorldRotation(pen->GetWorldRotation());

			if (Settings::SpaceMode().Get() == SpaceMode::Local) {
				cross->SetParent(o, false, true, true, false);
				cross->SetLocalPosition(glm::vec3());
			}

			crossStartPosition = this->GetPos();
		};
	}

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		Settings::ShouldRestrictTargetModeToPivot() = true;

		if (pen.HasValue() && !UnbindSceneObjects())
			return false;

		if (objs[0]->GetType() != type) {
			Logger.Warning("Invalid Object passed to ExtrusionEditingTool");
			return true;
		}
		pen = objs[0];

		crossOriginalPosition = cross->GetLocalPosition();
		crossOriginalParent = const_cast<SceneObject*>(cross->GetParent());
		stateChangedHandlerId = Changes::OnStateChange().AddHandler([&] {
			cross->SetParent(mesh.Get(), false, true, true, false);
			cross->SetLocalPosition(glm::vec3());
			});

		

		inputHandlerId = Input::AddHandler([&] { ProcessInput(); });
		spaceModeChangeHandlerId = Settings::SpaceMode().OnChanged() += [&](const SpaceMode& v) {
			if (!mesh.HasValue())
				return;

			auto pos = cross->GetWorldPosition();
			
			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetParent(mesh.Get(), false, true, true, false);
			else
				cross->SetParent(crossOriginalParent, false, true, true, false);

			cross->SetWorldPosition(pos);
			};

		return true;
	}
	virtual bool UnbindSceneObjects() {
		Settings::ShouldRestrictTargetModeToPivot() = false;

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
	virtual void UnbindTool() override {
		UnbindSceneObjects();
	}

	SceneObject* GetTarget() {
		return pen.HasValue()
			? pen.Get()
			: nullptr;
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

class TransformTool : public EditingTool {
	using Mode = TransformToolMode;

	const Log Logger = Log::For<TransformTool>();

	size_t inputHandlerId;
	size_t spaceModeChangeHandlerId;

	Mode mode;
	ObjectType type;

	std::vector<PON> targets;

	float minScale = 0.01;
	float oldScale = 1;
	glm::vec3 oldAngle;
	glm::vec3 transformOldPos;

	int oldAxeId;

	bool wasCommitDone = false;
	


	void ProcessInput(const ObjectType& type, const Mode& mode) {
		if (!shouldTrace && Input::HasContinuousMovementInputNoDelayStopped() && SceneObject::IsAnyElementChanged()) {
			Changes::Commit();
			SceneObject::ResetIsAnyElementChanged();

			transformPos = transformOldPos = glm::vec3();
			angle = oldAngle = glm::vec3();
			scale = oldScale = 1;
		}

		const auto newScale = Input::GetNewScale(scale);
		const auto relativeMovement = Input::GetRelativeMovement(transformPos - transformOldPos);
		const auto relativeRotation = Input::GetRelativeRotation(angle - oldAngle);

		// Check if we need to process tool.
		if (targets.empty() || !targets.front().HasValue()) {
			TransformCross(relativeMovement, newScale, relativeRotation);
			return;
		}
		if (Input::IsDown(Key::Escape, true)) {
			TransformCross(relativeMovement, newScale, relativeRotation);
			UnbindSceneObjects();
			return;
		}

		if (Settings::TargetMode().Get() == TargetMode::Pivot)
			TransformCross(relativeMovement, newScale, relativeRotation);
		else
			Transform(relativeMovement, newScale, relativeRotation);

		transformOldPos = transformPos;
	}
	void Scale(const glm::vec3& center, const float& oldScale, const float& scale, std::vector<PON>& targets) {
		if (shouldTrace)
			Trace(targets);

		Transform::Scale(center, oldScale, scale, targets);
	}
	void Translate(const glm::vec3& transformVector, std::vector<PON>& targets) {
		if (shouldTrace)
			Trace(targets);

		Transform::Translate(transformVector, targets, &cross.Get());
	}
	void Rotate(const glm::vec3& center, const glm::vec3& rotation, std::vector<PON>& targets) {
		if (shouldTrace)
			Trace(targets);

		Transform::Rotate(center, rotation, targets, &cross.Get());
	}

	void Trace(std::vector<PON>& targets) {
		static int id = 0;
		
		std::vector<PON> nonTraceObjects;
		for (auto& o : targets)
			if (o->GetType() != TraceObjectT)
				nonTraceObjects.push_back(o);

		for (size_t i = 0; i < nonTraceObjects.size(); i++)
		{
			auto pos = findBack(nonTraceObjects[i]->children, std::function([](SceneObject* so) {
				return so->GetType() == TraceObjectT;
				}));

			if (pos < 0) {
				traceObjectTool.destination = nonTraceObjects[i];
				traceObjectTool.init = [&, id = id](SceneObject* o) {
					std::stringstream ss;
					ss << "TraceObject" << id;
					o->Name = ss.str();
				};

				cloneTool.destination = traceObjectTool.Create();
				cloneTool.target = nonTraceObjects[i].Get();
				cloneTool.init = [](SceneObject* obj) {
					Scene::AssignUniqueName(obj, obj->Name);
					obj->children.clear();
				};
				cloneTool.onCreated = [isTheLastObjectInCloneGroup = i == nonTraceObjects.size() - 1](SceneObject*) {
					if (isTheLastObjectInCloneGroup)
						Changes::Commit();
				};
				cloneTool.Create();

				id++;
			}
			else {
				cloneTool.destination = nonTraceObjects[i]->children[pos];
				cloneTool.target = nonTraceObjects[i];
				cloneTool.init = [p = nonTraceObjects[i]->GetWorldPosition(), r = nonTraceObjects[i]->GetWorldRotation()](SceneObject* obj) {
					Scene::AssignUniqueName(obj, obj->Name);
					obj->children.clear();
				};
				cloneTool.onCreated = [isTheLastObjectInCloneGroup = i == nonTraceObjects.size() - 1](SceneObject*) {
					if (isTheLastObjectInCloneGroup)
						Changes::Commit();
				};
				cloneTool.Create();
			}
		}
	}

	void Transform(const glm::vec3& relativeMovement, const float newScale, const glm::vec3& relativeRotation) {
		if (relativeMovement != glm::vec3())
			Translate(relativeMovement, targets);
		if (relativeRotation != glm::vec3()) {
			Rotate(cross->GetWorldPosition(), relativeRotation, targets);
			nullifyUntouchedAngles(relativeRotation, angle, oldAngle);
			oldAngle = angle;
		}
		if (newScale != oldScale) {
			Scale(cross->GetWorldPosition(), oldScale, newScale, targets);
			oldScale = scale = newScale;
		}
	}
	void TransformCross(const glm::vec3& relativeMovement, const float newScale, const glm::vec3& relativeRotation) {
		if (relativeMovement != glm::vec3())
			Transform::Translate(relativeMovement, &Scene::cross().Get());
		if (relativeRotation != glm::vec3()) {
			Transform::Rotate(cross->GetWorldPosition(), relativeRotation, &Scene::cross().Get());
			nullifyUntouchedAngles(relativeRotation, angle, oldAngle);
			oldAngle = angle;
		}
		if (newScale != oldScale)
			oldScale = scale = newScale;
	}
	
	void nullifyUntouchedAngles(const glm::vec3& relativeRotation, glm::vec3& angle, glm::vec3& oldAngle) {
		auto da = angle + relativeRotation - oldAngle;
		for (auto i = 0; i < 3; i++)
			if (da[i] == 0.f)
				angle[i] = oldAngle[i] = 0.f;
	}

	static std::vector<glm::vec3> GetWorldPositions(std::list<PON>& vs) {
		std::vector<glm::vec3> l;
		for (auto o : vs)
			if (o.HasValue())
				l.push_back(o->GetWorldPosition());
		return l;
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		UnbindTool();
		if (v.empty())
			return;

		targets.clear();
		targets.insert(targets.end(), v.begin(), v.end());
		
		Input::RemoveHandler(cross->keyboardBindingHandlerId);
		inputHandlerId = Input::AddHandler([this]{ ProcessInput(type, mode); });
		spaceModeChangeHandlerId = Settings::SpaceMode().OnChanged() += [&](const SpaceMode& v) {
			transformOldPos = transformPos = oldAngle = angle = glm::vec3();
			oldAngle = angle = glm::vec3();
		};
	}

public:
	NonAssignProperty<Cross*> cross;

	// These are modified via GUI
	float scale = 1;
	glm::vec3 angle;
	glm::vec3 transformPos;
	bool shouldTrace;

	CreatingTool<TraceObject> traceObjectTool;
	CloneTool cloneTool;

	virtual bool UnbindSceneObjects() override {
		UnbindTool();
		return true;
	}


	virtual void UnbindTool() override {
		if (targets.empty())
			return;

		scale = 1;
		oldScale = 1;
		angle = glm::vec3();
		oldAngle = glm::vec3();
		transformPos = glm::vec3();

		Input::RemoveHandler(inputHandlerId);
		// Remove if exists and add a new one.
		Input::RemoveHandler(cross->keyboardBindingHandlerId);
		cross->keyboardBindingHandlerId = Input::AddHandler(cross->keyboardBindingHandler);

		Settings::SpaceMode().OnChanged().RemoveHandler(spaceModeChangeHandlerId);
	}
	SceneObject* GetTarget() {
		if (targets.empty() || !targets.front().HasValue())
			return nullptr;
		else 
			return targets.front().Get();
	}
	void SetMode(Mode mode) {
		this->mode = mode;
	}
	const Mode& GetMode() {
		return mode;
	}
};

class SinePenTool : public EditingTool {
	using Mode = SinePenEditingToolMode;

	const Log Logger = Log::For<SinePenTool>();

	size_t inputHandlerId;
	size_t stateChangedHandlerId;
	size_t modeChangedHandlerId;

	// Create new object by pressing Enter.
	size_t createNewObjectHandlerId;
	bool lockCreateNewObjectHandlerId;

	Property<Mode> mode;
	PON target;

	bool createdAdditionalPoints;
	bool createdNewObject;

	size_t currentVertice = 0;

	void Step123() {
		if (!createdAdditionalPoints || target->GetVertices().empty()) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(cross->GetWorldPosition());
			createdAdditionalPoints = true;
			return;
		}

		if (createdNewObject || Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			createdNewObject = false;
			target->AddVertice(cross->GetWorldPosition());

			Changes::Commit();
			return;
		}

		if (cross->GetWorldPosition() == target->GetVertices().back())
			return;

		target->SetVertice(target->GetVertices().size() - 1, cross->GetWorldPosition());
	}
	void Step132() {
		if (!createdAdditionalPoints || target->GetVertices().empty()) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			createVertice132();
			createdAdditionalPoints = true;
			return;
		}

		if (createdNewObject || Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			createdNewObject = false;
			createVertice132();

			Changes::Commit();
			return;
		}

		if (cross->GetWorldPosition() == target->GetVertices()[currentVertice])
			return;

		target->SetVertice(currentVertice, cross->GetWorldPosition());
	}

	void createVertice132() {
		if (target->GetVertices().size() > 1 && target->GetVertices().size() % 2 == 0)
		{
			currentVertice = target->GetVertices().size() - 1;
			target->AddVertice(target->GetVertices().back());
			target->SetVertice(currentVertice, cross->GetWorldPosition());
		}
		else
		{
			currentVertice = target->GetVertices().size();
			target->AddVertice(cross->GetWorldPosition());
		}
	}

	int getCurrentVertice() {
		if (!target.HasValue())
			return 0;

		if (target->GetVertices().size() == 0)
			return 0;

		if (mode.Get() == Mode::Step132 && target->GetVertices().size() % 2 != 0)
			return target->GetVertices().size() - 2;

		return target->GetVertices().size() - 1;
	}

	void moveCrossToVertice(int i) {
		cross->SetWorldPosition(target->GetVertices()[i]);
	}

	void ProcessInput() {
		if (!target.HasValue()) {
			UnbindSceneObjects();
			return;
		}

		switch (mode.Get())
		{
		case Mode::Step123: return Step123();
		case Mode::Step132: return Step132();
		default:
			Logger.Warning("Not suported mode was given");
			return;
		}
	}

	void TryCreateNewObject() {
		if (createNewObjectHandlerId)
			return;

		createNewObjectHandlerId = Input::AddHandler([&] {
			if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
				lockCreateNewObjectHandlerId = true;

				auto cmd = new CreateCommand();

				if (ObjectSelection::Selected().size() == 1) {
					auto d = ObjectSelection::Selected().begin()._Ptr->_Myval.Get();
					cmd->destination = d->GetParent() ? const_cast<SceneObject*>(d->GetParent()) : d;
				}

				cmd->init = [] {
					auto o = new SineCurve();
					o->SetWorldPosition(Scene::cross()->GetWorldPosition());
					o->SetWorldRotation(Scene::cross()->GetWorldRotation());
					Scene::AssignUniqueName(o, "SineCurve");
					return o;
				};
				cmd->onCreated = [&](SceneObject* o) {
					createdNewObject = true;
					ObjectSelection::Set(o);
					Input::RemoveHandler(createNewObjectHandlerId);
					lockCreateNewObjectHandlerId = false;
					Changes::Commit();
				};
			}
			});
	}

	ObjectSelection::Selection GetExistingObjects(const ObjectSelection::Selection& v) {
		ObjectSelection::Selection ns;
		for (auto& o : Scene::Objects().Get())
			if (v.find(o) != v.end())
				ns.insert(o);

		return ns;
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		UnbindTool();
		OnToolActivated(v);
	}
	virtual void OnToolActivated(const ObjectSelection::Selection& v) override {
		// Don't work with deleted objects even if they stay selected.
		auto targets = GetExistingObjects(v);
		Settings::ShouldRestrictTargetModeToPivot() = true;

		if (targets.empty())
			return TryCreateNewObject();

		if (targets.size() > 1) {
			Logger.Warning("Cannot work with multiple objects");
			return;
		}

		auto& t = targets.begin()._Ptr->_Myval;

		if (t->GetType() != SineCurveT)
			return TryCreateNewObject();

		createdAdditionalPoints = false;
		currentVertice = getCurrentVertice();

		target = t;

		if (!target->GetVertices().empty())
			cross->SetWorldPosition(target->GetVertices().back());

		inputHandlerId = Input::AddHandler([&]{
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput();
			});
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetWorldRotation(glm::quat());
				return;
			}

			if (!exists(Scene::Objects().Get(), target)) {
				target = PON();
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetWorldRotation(target.Get()->GetWorldRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetWorldPosition(target->GetVertices().back());
		};
		modeChangedHandlerId = mode.OnChanged() += [&](const Mode& v) {
			currentVertice = getCurrentVertice();
			if (Settings::ShouldMoveCrossOnSinePenModeChange().Get())
				moveCrossToVertice(currentVertice);
		};
	}

public:
	NonAssignProperty<Cross*> cross;

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (target.HasValue() && !UnbindSceneObjects())
			return false;

		if (objs[0]->GetType() != SineCurveT) {
			Logger.Warning("Invalid Object passed to SinePenTool");
			return true;
		}
		target = objs[0];

		createdAdditionalPoints = false;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetWorldRotation(target.Get()->GetWorldRotation());

		if (!target->GetVertices().empty())
			cross->SetWorldPosition(target->GetVertices().back());

		inputHandlerId = Input::AddHandler([&] {
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput();
			});
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetWorldRotation(glm::quat());
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetWorldRotation(target.Get()->GetWorldRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetWorldPosition(target->GetVertices().back());
		};
		Settings::ShouldRestrictTargetModeToPivot() = true;

		return true;
	}
	virtual bool UnbindSceneObjects() {
		// On unbinding objects the tool goes to 
		// creating new object awaiting state.
		// But if the tool is unbind then it should be cancelled.
		if (!lockCreateNewObjectHandlerId)
			Input::RemoveHandler(createNewObjectHandlerId);

		Settings::ShouldRestrictTargetModeToPivot() = false;

		if (!target.HasValue())
			return true;

		target->RemoveVertice();
		createdAdditionalPoints = false;

		Input::RemoveHandler(inputHandlerId);
		Changes::OnStateChange() -= stateChangedHandlerId;
		mode.OnChanged() -= modeChangedHandlerId;

		target = PON();

		// Go to creating mode after unbinding target;
		TryCreateNewObject();

		return true;
	}
	virtual void UnbindTool() override {
		UnbindSceneObjects();
	}

	SceneObject* GetTarget() {
		return target.HasValue()
			? target.Get()
			: nullptr;
	}

	void SetMode(Mode mode) {
		this->mode = mode;
	}
};