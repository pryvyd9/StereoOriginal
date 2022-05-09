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
#include "Localization.hpp"

class Tool {
protected:
	static size_t& isBeingModifiedHandler() {
		static size_t v = 0;
		return v;
	}
	static bool& isAnyActive() {
		static bool v = false;
		return v;
	}
	static size_t& selectionChangedhandlerId() {
		static size_t v = 0;
		return v;
	}
	static Tool*& activeTool() {
		static Tool* v = nullptr;
		return v;
	}
	bool isToolActive() {
		return activeTool() == this;
	}
};

template<typename T>
class CreatingTool : Tool {
public:
	std::function<void(T*)> init;
	std::function<void(T*)> onCreated = [](T*) {};

	Property<PON> destination;

	T* Create() {
		T* obj = new T();

		auto command = new CreateCommand();
		command->destination = destination.Get().Get();
		command->init = [=] {
			init(obj);
			return obj;
		};
		command->onCreated = [=] (SceneObject*) { onCreated(obj); };

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

class EditingTool : public Tool {
protected:
	static bool& isBeingModified() {
		static bool v = false;
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
		activeTool() = this;

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
	using Mode = PolylinePenEditingToolMode;

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
			target->AddVertice(cross->GetPosition());
			return;
		}

		// If cross is located at distance less than Precision then don't do anything.
		if (glm::length(cross->GetPosition() - points[pointsCount - 2]) < Precision)
			return;

		// Create the third point to be able to measure the angle between the last 3 points.
		if (pointsCount < 3) {
			target->AddVertice(cross->GetPosition());
			return;
		}

		// If the line goes straight then instead of adding 
		// a new point - move the previous point to current cross position.
		if (IsStraightLineMaintained(
			points[pointsCount - 3], 
			points[pointsCount - 2], 
			points[pointsCount - 1], 
			cross->GetPosition(),
			E)) {
			target->SetVertice(pointsCount - 2, cross->GetPosition());
			target->SetVertice(pointsCount - 1, cross->GetPosition());
		}
		else
			target->AddVertice(cross->GetPosition());
	}
	void Step() {
		if (!createdAdditionalPoints || target->GetVertices().empty()) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(cross->GetPosition());
			createdAdditionalPoints = true;
			return;
		}

		if (createdNewObject || Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			Changes::Commit();
			createdNewObject = false;
			target->AddVertice(cross->GetPosition());
			return;
		}

		if (cross->GetPosition() == target->GetVertices().back())
			return;

		target->SetVertice(target->GetVertices().size() - 1, cross->GetPosition());
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
			if (!isToolActive()) {
				Input::continuousInputNoDelay().ContinuousOrStopped() -= createNewObjectHandlerId;
				createNewObjectHandlerId = 0;
				lockCreateNewObjectHandlerId = false;
				return;
			}

			if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
				lockCreateNewObjectHandlerId = true;

				auto cmd = new CreateCommand();

				if (ObjectSelection::Selected().size() == 1) {
					auto d = ObjectSelection::Selected().begin()._Ptr->_Myval.Get();
					cmd->destination = d->GetParent() ? const_cast<SceneObject*>(d->GetParent()) : d;
				}

				cmd->init = [] {
					auto o = new PolyLine();
					o->SetPosition(Scene::cross()->GetPosition());
					o->SetRotation(Scene::cross()->GetRotation());
					Scene::AssignUniqueName(o, LocaleProvider::Get("object:polyline"));
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

		if (t->GetType() != PolyLineT)
			return TryCreateNewObject();

		createdAdditionalPoints = false;

		target = t;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetRotation(target.Get()->GetRotation());

		if (!target->GetVertices().empty())
			cross->SetPosition(target->GetVertices().back());

		inputHandlerId = Input::AddHandler([&] {
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput();
			});
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetRotation(glm::quat());
				return;
			}

			if (!exists(Scene::Objects().Get(), target)) {
				target = PON();
				return;
			}

			if (target->GetVertices().empty())
				return;

			cross->SetPosition(target->GetVertices().back());
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
			cross->SetRotation(target.Get()->GetRotation());

		if (!target->GetVertices().empty())
			cross->SetPosition(target->GetVertices().back());

		inputHandlerId = Input::AddHandler([&] {
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput();
			});
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetRotation(glm::quat());
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetRotation(target.Get()->GetRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetPosition(target->GetVertices().back());
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

class TransformTool : public EditingTool {
	const Log Logger = Log::For<TransformTool>();

	size_t spaceModeChangeHandlerId;

	ObjectType type;

	std::vector<PON> targets;

	float minScale = 0.01;
	float oldScale = 1;
	glm::vec3 oldAngle;
	glm::vec3 transformOldPos;

	int oldAxeId;

	bool wasCommitDone = false;
	


	void ProcessInput(const ObjectType& type) {
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
			TransformCross(relativeMovement, newScale, relativeRotation, Source::Tool);
			return;
		}
		if (Input::IsDown(Key::Escape, true)) {
			TransformCross(relativeMovement, newScale, relativeRotation, Source::Tool);
			UnbindSceneObjects();
			return;
		}

		if (Settings::TargetMode().Get() == TargetMode::Pivot)
			TransformCross(relativeMovement, newScale, relativeRotation, Source::Tool);
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

		Transform::Rotate(center, rotation, targets, &cross.Get(), Source::Tool);
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

			auto cloneInitFunc = [](SceneObject* obj) {
				Scene::AssignUniqueName(obj, obj->Name);
				obj->children.clear();
			};
			auto cloneOnCreatedFunc = [isTheLastObjectInCloneGroup = i == nonTraceObjects.size() - 1, shouldTraceMesh = shouldTraceMesh](SceneObject*) {
				if (isTheLastObjectInCloneGroup && !shouldTraceMesh)
					Changes::Commit();
			};

			if (pos < 0) {
				traceObjectTool.destination = nonTraceObjects[i];
				traceObjectTool.init = [](SceneObject* o) {
					Scene::AssignUniqueName(o, LocaleProvider::Get("object:trace"));
				};

				cloneTool.destination = traceObjectTool.Create();
				cloneTool.target = nonTraceObjects[i].Get();
				cloneTool.init = cloneInitFunc;
				cloneTool.onCreated = cloneOnCreatedFunc;
				auto clonedObject = cloneTool.Create();

				if (shouldTraceMesh)
					TraceMesh(cloneTool.destination, nonTraceObjects[i], clonedObject, i == nonTraceObjects.size() - 1, shouldTraceMesh);

				id++;
			}
			else {
				cloneTool.destination = nonTraceObjects[i]->children[pos];
				cloneTool.target = nonTraceObjects[i];
				cloneTool.init = cloneInitFunc;
				cloneTool.onCreated = cloneOnCreatedFunc;
				auto clonedObject = cloneTool.Create();

				if (shouldTraceMesh)
					TraceMesh(cloneTool.destination, nonTraceObjects[i], clonedObject, i == nonTraceObjects.size() - 1, shouldTraceMesh);
			}
		}
	}
	void TraceMesh(PON destination, PON fromObject, PON toObject, bool isTheLastObjectInCloneGroup, bool shouldTraceMesh) {
		meshTool.destination = destination;
		meshTool.init = [](Mesh* obj) {
			Scene::AssignUniqueName(obj, obj->Name);
			obj->children.clear();
		};
		meshTool.onCreated = [fromObject, toObject, isTheLastObjectInCloneGroup, shouldTraceMesh](Mesh* obj) {
			if (fromObject->GetType() == PolyLineT || fromObject->GetType() == SineCurveT) {
				obj->AddVertices(fromObject->GetVertices());
				obj->AddVertices(toObject->GetVertices());

				auto verticesCountHalf = toObject->GetVertices().size();

				std::vector<std::array<GLuint, 2>> connections(verticesCountHalf);

				for (GLuint i = 0; i < verticesCountHalf; i++)
					connections.push_back({ i, i + (GLuint)verticesCountHalf });

				obj->SetConnections(connections);

				if (isTheLastObjectInCloneGroup && shouldTraceMesh)
					Changes::Commit();
			}
		};
		meshTool.Create();
	}

	void Transform(const glm::vec3& relativeMovement, const float newScale, const glm::vec3& relativeRotation) {
		if (relativeMovement != glm::vec3())
			Translate(relativeMovement, targets);
		if (relativeRotation != glm::vec3()) {
			Rotate(cross->GetPosition(), relativeRotation, targets);
			nullifyUntouchedAngles(relativeRotation, angle, oldAngle);
			oldAngle = angle;
		}
		if (newScale != oldScale) {
			Scale(cross->GetPosition(), oldScale, newScale, targets);
			oldScale = scale = newScale;
		}
	}
	void TransformCross(const glm::vec3& relativeMovement, const float newScale, const glm::vec3& relativeRotation, Source source) {
		if (relativeMovement != glm::vec3())
			Transform::Translate(relativeMovement, &Scene::cross().Get(), source);
		if (relativeRotation != glm::vec3()) {
			Transform::Rotate(cross->GetPosition(), relativeRotation, &Scene::cross().Get(), source);
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

	static std::vector<glm::vec3> GetPositions(std::list<PON>& vs) {
		std::vector<glm::vec3> l;
		for (auto& o : vs)
			if (o.HasValue())
				l.push_back(o->GetPosition());
		return l;
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		UnbindTool();
		if (v.empty())
			return;

		targets.clear();
		targets.insert(targets.end(), v.begin(), v.end());
		
		cross->keyboardBindingProcessor = [this] { ProcessInput(type); };
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
	bool shouldTraceMesh;

	// This is a GUI angle modified property.
	// It defines the onChanged handler via constructor
	Property<glm::vec3> GUIAngle{ {[&](const glm::vec3& v) {
		const auto relativeRotation = Input::GetRelativeRotation(v);
		Rotate(Scene::cross()->GetPosition(), relativeRotation, targets);
	}} };
	Property<glm::vec3> GUIPosition{ {[&](const glm::vec3& v) {
		const auto relativeMovement = Input::GetRelativeMovement(v - Scene::cross()->GetPosition());
		Translate(relativeMovement, targets);
	}} };
	Property<float> GUIScale{ {[&](const float& v) {
		const auto newScale = Input::GetNewScale(v);
		if (abs(newScale) < minScale)
			return;
		Scale(Scene::cross()->GetPosition(), oldScale, newScale, targets);
		oldScale = scale = newScale;
	}}, 1 };


	CreatingTool<TraceObject> traceObjectTool;
	CreatingTool<Mesh> meshTool;
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

		cross->keyboardBindingProcessor = cross->keyboardBindingProcessorDefault;

		Settings::SpaceMode().OnChanged().RemoveHandler(spaceModeChangeHandlerId);
	}
	SceneObject* GetTarget() {
		if (targets.empty() || !targets.front().HasValue())
			return nullptr;
		else 
			return targets.front().Get();
	}
};

class CosinePenTool : public EditingTool {
	using Mode = CosinePenEditingToolMode;

	const Log Logger = Log::For<CosinePenTool>();

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
			target->AddVertice(cross->GetPosition());
			createdAdditionalPoints = true;
			return;
		}

		if (createdNewObject || Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
			createdNewObject = false;
			target->AddVertice(cross->GetPosition());

			Changes::Commit();
			return;
		}

		if (cross->GetPosition() == target->GetVertices().back())
			return;

		target->SetVertice(target->GetVertices().size() - 1, cross->GetPosition());
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

		if (cross->GetPosition() == target->GetVertices()[currentVertice])
			return;

		target->SetVertice(currentVertice, cross->GetPosition());
	}

	void createVertice132() {
		if (target->GetVertices().size() > 1 && target->GetVertices().size() % 2 == 0)
		{
			currentVertice = target->GetVertices().size() - 1;
			target->AddVertice(target->GetVertices().back());
			target->SetVertice(currentVertice, cross->GetPosition());
		}
		else
		{
			currentVertice = target->GetVertices().size();
			target->AddVertice(cross->GetPosition());
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
		cross->SetPosition(target->GetVertices()[i]);
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
			if (!isToolActive()) {
				Input::continuousInputNoDelay().ContinuousOrStopped() -= createNewObjectHandlerId;
				createNewObjectHandlerId = 0;
				lockCreateNewObjectHandlerId = false;
				return;
			}

			if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
				lockCreateNewObjectHandlerId = true;

				auto cmd = new CreateCommand();

				if (ObjectSelection::Selected().size() == 1) {
					auto d = ObjectSelection::Selected().begin()._Ptr->_Myval.Get();
					cmd->destination = d->GetParent() ? const_cast<SceneObject*>(d->GetParent()) : d;
				}

				cmd->init = [] {
					auto o = new SineCurve();
					o->SetPosition(Scene::cross()->GetPosition());
					o->SetRotation(Scene::cross()->GetRotation());
					Scene::AssignUniqueName(o, LocaleProvider::Get("object:sinecurve"));
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
			cross->SetPosition(target->GetVertices().back());

		inputHandlerId = Input::continuousInputNoDelay().ContinuousOrStopped() += [&] (){ ProcessInput(); };
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetRotation(glm::quat());
				return;
			}

			if (!exists(Scene::Objects().Get(), target)) {
				target = PON();
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetRotation(target.Get()->GetRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetPosition(target->GetVertices().back());
		};
		modeChangedHandlerId = mode.OnChanged() += [&](const Mode& v) {
			currentVertice = getCurrentVertice();
			if (Settings::ShouldMoveCrossOnCosinePenModeChange().Get())
				moveCrossToVertice(currentVertice);
		};
	}

public:
	NonAssignProperty<Cross*> cross;

	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (target.HasValue() && !UnbindSceneObjects())
			return false;

		if (objs[0]->GetType() != SineCurveT) {
			Logger.Warning("Invalid Object passed to CosinePenTool");
			return true;
		}
		target = objs[0];

		createdAdditionalPoints = false;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetRotation(target.Get()->GetRotation());

		if (!target->GetVertices().empty())
			cross->SetPosition(target->GetVertices().back());

		inputHandlerId = Input::continuousInputNoDelay().ContinuousOrStopped() += [&]() { ProcessInput(); };
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetRotation(glm::quat());
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetRotation(target.Get()->GetRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetPosition(target->GetVertices().back());
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

		Input::continuousInputNoDelay().ContinuousOrStopped() -= inputHandlerId;

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

class PointPenTool : public EditingTool {
	const Log Logger = Log::For<PointPenTool>();

	size_t inputHandlerId;
	size_t stateChangedHandlerId;

	// Create new object by pressing Enter.
	size_t createNewObjectHandlerId;
	bool lockCreateNewObjectHandlerId;

	PON target;

	bool createdNewObject;

	void TryCreateNewObject() {
		if (createNewObjectHandlerId)
			return;

		createNewObjectHandlerId = Input::continuousInputNoDelay().ContinuousOrStopped() += [&] {
			if (!isToolActive()) {
				Input::continuousInputNoDelay().ContinuousOrStopped() -= createNewObjectHandlerId;
				createNewObjectHandlerId = 0;
				lockCreateNewObjectHandlerId = false;
				return;
			}

			if (Input::IsDown(Key::Enter, true) || Input::IsDown(Key::NEnter, true)) {
				lockCreateNewObjectHandlerId = true;

				auto cmd = new CreateCommand();

				if (ObjectSelection::Selected().size() == 1) {
					auto d = ObjectSelection::Selected().begin()._Ptr->_Myval.Get();
					cmd->destination = d->GetParent() ? const_cast<SceneObject*>(d->GetParent()) : d;
				}

				cmd->init = [] {
					auto o = new PointObject();
					o->SetPosition(Scene::cross()->GetPosition());
					o->SetRotation(Scene::cross()->GetRotation());
					Scene::AssignUniqueName(o, LocaleProvider::Get("object:point"));
					return o;
				};
				cmd->onCreated = [&](SceneObject* o) {
					createdNewObject = true;
					ObjectSelection::Set(o);
					Changes::Commit();
				};
			}
		};
	}

	ObjectSelection::Selection GetExistingObjects(const ObjectSelection::Selection& v) {
		ObjectSelection::Selection ns;
		for (auto& o : Scene::Objects().Get())
			if (v.find(o) != v.end())
				ns.insert(o);

		return ns;
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		//UnbindTool();
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

		if (t->GetType() != PointT)
			return TryCreateNewObject();

		target = t;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetRotation(target.Get()->GetRotation());

		if (!target->GetVertices().empty())
			cross->SetPosition(target->GetVertices().back());
		inputHandlerId = Input::continuousInputNoDelay().ContinuousOrStopped() += [&] { TryCreateNewObject(); };
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetRotation(glm::quat());
				return;
			}

			if (!exists(Scene::Objects().Get(), target)) {
				target = PON();
				return;
			}

			if (target->GetVertices().empty())
				return;

			cross->SetPosition(target->GetVertices().back());
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

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetRotation(target.Get()->GetRotation());

		if (!target->GetVertices().empty())
			cross->SetPosition(target->GetVertices().back());

		inputHandlerId = Input::continuousInputNoDelay().ContinuousOrStopped() += [&] { TryCreateNewObject(); };
		stateChangedHandlerId = Changes::OnStateChange() += [&] {
			if (!target.HasValue()) {
				cross->SetRotation(glm::quat());
				return;
			}

			if (Settings::SpaceMode().Get() == SpaceMode::Local)
				cross->SetRotation(target.Get()->GetRotation());

			if (target->GetVertices().empty())
				return;

			cross->SetPosition(target->GetVertices().back());
		};
		Settings::ShouldRestrictTargetModeToPivot() = true;

		return true;
	}
	virtual bool UnbindSceneObjects() {
		if (!lockCreateNewObjectHandlerId) {
			Input::continuousInputNoDelay().ContinuousOrStopped() -= createNewObjectHandlerId;
			createNewObjectHandlerId = 0;
		}

		Settings::ShouldRestrictTargetModeToPivot() = false;

		if (!target.HasValue())
			return true;

		Input::continuousInputNoDelay().ContinuousOrStopped() -= inputHandlerId;
		Changes::OnStateChange() -= stateChangedHandlerId;

		target = PON();

		// Go to creating mode after unbinding target;
		TryCreateNewObject();

		return true;
	}
	virtual void UnbindTool() override {
		lockCreateNewObjectHandlerId = false;
		UnbindSceneObjects();
	}

	SceneObject* GetTarget() {
		return target.HasValue()
			? target.Get()
			: nullptr;
	}
};
