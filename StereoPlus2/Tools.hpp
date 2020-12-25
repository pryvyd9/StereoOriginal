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
	//static std::stack<SceneObject*>& GetCreatedObjects() {
	//	static std::stack<SceneObject*> val;
	//	return val;
	//}
public:
	std::function<void(T*)> init;

	ReadonlyProperty<PON> destination;

	bool Create() {
		StateBuffer::Commit();

		auto command = new CreateCommand();
		command->destination = destination.Get().Get();
		command->init = [=] {
			T* obj = new T();
			init(obj);
			//GetCreatedObjects().push(obj);

			return obj;
		};

		return true;
	}
};

class CloneTool : Tool {
	
public:
	ReadonlyProperty<PON> destination;
	ReadonlyProperty<PON> target;
	std::function<void(SceneObject*)> init;

	bool Create() {
		StateBuffer::Commit();

		auto command = new CreateCommand();
		command->destination = destination.Get().Get();
		command->init = [=] {
			auto obj = target->Clone();
			init(obj);
			return obj;
		};

		return true;
	}
};

template<typename EditMode>
class EditingTool : Tool {
	//static size_t& isBeingModifiedHandler() {
	//	static size_t v;
	//	return v;
	//}
	//static bool& isAnyActive() {
	//	static bool v;
	//	return v;
	//}
	//static size_t& selectionChangedhandlerId() {
	//	static size_t v;
	//	return v;
	//}
	//static EditingTool*& activeTool() {
	//	static EditingTool* v = nullptr;
	//	return v;
	//}

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


	void DeleteConfig() {
		if (config != nullptr)
		{
			delete config;
			config = nullptr;
		}
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) {}
	virtual void OnToolActivated(const ObjectSelection::Selection& v) { OnSelectionChanged(v); }
	//virtual void OnBeforeDeactivate() {}
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

	virtual bool BindSceneObjects(std::vector<PON> obj) { return true; };
	virtual bool UnbindSceneObjects() { return true; };

	virtual void Activate() {
		if (isAnyActive())
			Deactivate();

		selectionChangedhandlerId() = ObjectSelection::OnChanged() += [&](const ObjectSelection::Selection& v) {
			OnSelectionChanged(v);
		};

		isAnyActive() = true;
		//activeTool() = this;

		OnToolActivated(ObjectSelection::Selected());
	}
	static void Deactivate() {
		if (!isAnyActive())
			return;

		//if (activeTool())
		//	static_cast<EditingTool*>(activeTool())->UnbindSceneObjects();

		ObjectSelection::OnChanged() -= selectionChangedhandlerId();
		selectionChangedhandlerId() = 0;
		isAnyActive() = false;
	}


};

class PointPenEditingTool : public EditingTool<PointPenEditingToolMode>{
	Log Logger = Log::For<PointPenEditingTool>();
	size_t inputHandlerId;
	size_t spaceModeChangeHandlerId;
	size_t stateChangedHandlerId;
	size_t anyObjectChangedHandlerId;

	// Create new object by pressing Enter.
	size_t createNewObjectHandlerId;
	bool lockCreateNewObjectHandlerId;

	Mode mode;
	PON target;

	SceneObject* crossOriginalParent;
	bool wasCommitDone = false;

	bool createdAdditionalPoints;
	bool createdNewObject;
	
	// If the cos between vectors is less than E
	// then we merge those vectors.
	double E = 1e-6;

	// If distance between previous point and 
	// cursor is less than this number 
	// then don't create a new point.
	double Precision = 1e-4;

	void Immediate(Input* input) {
		createdNewObject = false;

		if (!input->IsContinuousInputOneSecondDelay()) {
			isBeingModified() = false;
			wasCommitDone = false;
		}

		if (Input::IsDown(Key::Enter) || Input::IsDown(Key::NEnter)) {
			UnbindSceneObjects();
			TryCreateNewObject();
			return;
		}

		auto& points = target->GetVertices();
		auto pointsCount = points.size();

		// Create 2 points to be able to measure distance between them.
		if (pointsCount < 2) {
			target->AddVertice(cross->GetLocalPosition());
			return;
		}

		// If cross is located at distance less than Precision then don't create a new point
		// but move the last one to cross position.
		if (glm::length(cross->GetWorldPosition() - points[pointsCount - 2]) < Precision) {
			target->SetVertice(pointsCount - 1, cross->GetWorldPosition());
			return;
		}

		// Create the third point to be able to measure the angle between the last 3 points.
		if (pointsCount < 3) {
			target->AddVertice(cross->GetLocalPosition());
			return;
		}

		// If the line goes straight then instead of adding 
		// a new point - move the previous point to current cross position.
		glm::vec3 r1 = cross->GetWorldPosition() - points[pointsCount - 1];
		glm::vec3 r2 = points[pointsCount - 3] - points[pointsCount - 2];

		auto p = glm::dot(r1, r2);
		auto l1 = glm::length(r1);
		auto l2 = glm::length(r2);

		auto cos = p / l1 / l2;

		if (abs(cos) > 1 - E || isnan(cos)) {
			target->SetVertice(pointsCount - 2, cross->GetWorldPosition());
			target->SetVertice(pointsCount - 1, cross->GetWorldPosition());
		}
		else {
			target->AddVertice(cross->GetWorldPosition());
		}
	}
	void Step() {
		if (!createdAdditionalPoints || target->GetVertices().empty()) {
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			target->AddVertice(cross->GetWorldPosition());
			createdAdditionalPoints = true;
			return;
		}

		if (Input::IsDown(Key::Enter) || Input::IsDown(Key::NEnter) || createdNewObject) {
			isBeingModified() = false;
			wasCommitDone = false;
			createdNewObject = false;
			target->AddVertice(cross->GetWorldPosition());
			return;
		}

		if (cross->GetWorldPosition() == target->GetVertices().back())
			return;

		target->SetVertice(target->GetVertices().size() - 1, cross->GetWorldPosition());
	}

	void ProcessInput(Input* input) {
		if (!target.HasValue()) {
			UnbindSceneObjects();
			return;
		}
			
		switch (mode)
		{
		case Mode::Immediate: return Immediate(input);
		case Mode::Step: return Step();
		default:
			Logger.Warning("Not suported mode was given");
			return;
		}
	}

	void TryCreateNewObject() {
		if (createNewObjectHandlerId)
			return;

		createNewObjectHandlerId = keyBinding->AddHandler([&]() {
			if ((Input::IsDown(Key::Enter) || Input::IsDown(Key::NEnter)) && !ImGui::IsAnyItemActive()) {
				lockCreateNewObjectHandlerId = true;

				auto cmd = new CreateCommand();

				if (ObjectSelection::Selected().size() == 1) {
					auto d = ObjectSelection::Selected().begin()._Ptr->_Myval.Get();
					cmd->destination = d->GetParent() ? const_cast<SceneObject*>(d->GetParent()) : d;
				}

				cmd->init = [] {
					auto o = new StereoPolyLine();
					o->SetWorldPosition(Scene::cross().Get()->GetWorldPosition());
					o->SetWorldRotation(Scene::cross().Get()->GetWorldRotation());
					return o;
				};
				cmd->onCreated = [&](SceneObject* o) {
					createdNewObject = true;
					ObjectSelection::Set(o);
					keyBinding->RemoveHandler(createNewObjectHandlerId);
					lockCreateNewObjectHandlerId = false;
				};
			}
			});
	}

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		UnbindTool();
		OnToolActivated(v);
	}
	virtual void OnToolActivated(const ObjectSelection::Selection& v) override {
		if (v.empty())
			return TryCreateNewObject();

		if (v.size() > 1) {
			Logger.Warning("Cannot work with multiple objects");
			return;
		}

		auto t = v.begin()._Ptr->_Myval;

		if (t->GetType() != StereoPolyLineT)
			return TryCreateNewObject();

		createdAdditionalPoints = false;

		target = t;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetWorldRotation(target.Get()->GetWorldRotation());

		if (!target->GetVertices().empty())
			cross->SetWorldPosition(target->GetVertices().back());

		inputHandlerId = keyBinding->AddHandler([&](Input* input) { 
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput(input);
			});
		spaceModeChangeHandlerId = Settings::SpaceMode().OnChanged() += [&](const SpaceMode& v) {
			if (v == SpaceMode::Local)
				cross->SetWorldRotation(target.Get()->GetWorldRotation());
			else
				cross->SetWorldRotation(glm::quat());
		};
		stateChangedHandlerId = StateBuffer::OnStateChange() += [&] {
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
		anyObjectChangedHandlerId = SceneObject::OnBeforeAnyElementChanged() += [&] {
			if (wasCommitDone)
				return;

			StateBuffer::Commit();
			wasCommitDone = true;
		};

	}

public:
	ReadonlyProperty<Cross*> cross;


	virtual bool BindSceneObjects(std::vector<PON> objs) {
		if (target.HasValue() && !UnbindSceneObjects())
			return false;

		if (objs[0]->GetType() != StereoPolyLineT) {
			Logger.Warning("Invalid Object passed to PointPenEditingTool");
			return true;
		}
		target = objs[0];

		createdAdditionalPoints = false;

		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			cross->SetWorldRotation(target.Get()->GetWorldRotation());

		if (!target->GetVertices().empty())
			cross->SetWorldPosition(target->GetVertices().back());

		inputHandlerId = keyBinding->AddHandler([&](Input* input) {
			// If handler was removed via shortcuts then don't execute it.
			if (inputHandlerId)
				ProcessInput(input);
			});
		spaceModeChangeHandlerId = Settings::SpaceMode().OnChanged() += [&](const SpaceMode& v) {
			if (v == SpaceMode::Local)
				cross->SetWorldRotation(target.Get()->GetWorldRotation());
			else
				cross->SetWorldRotation(glm::quat());
		};
		stateChangedHandlerId = StateBuffer::OnStateChange() += [&] {
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
		anyObjectChangedHandlerId = SceneObject::OnBeforeAnyElementChanged() += [&] {
			if (wasCommitDone)
				return;

			StateBuffer::Commit();
			wasCommitDone = true;
		};

		return true;
	}
	virtual bool UnbindSceneObjects() {
		if (!lockCreateNewObjectHandlerId)
			keyBinding->RemoveHandler(createNewObjectHandlerId);

		if (!target.HasValue())
			return true;

		if (mode == Mode::Step && createdAdditionalPoints) {
			target->RemoveVertice();
			createdAdditionalPoints = false;
		}
	
		keyBinding->RemoveHandler(inputHandlerId);
		Settings::SpaceMode().OnChanged() -= spaceModeChangeHandlerId;
		StateBuffer::OnStateChange() -= stateChangedHandlerId;
		SceneObject::OnBeforeAnyElementChanged() -= anyObjectChangedHandlerId;

		target = PON();
		
		// Go to creating mode after unbinding target;
		TryCreateNewObject();

		return true;
	}
	void UnbindTool() {
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
		if (Settings::SpaceMode().Get() == SpaceMode::Local)
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

		Settings::SpaceMode().OnChanged() -= spaceModeChangeHandlerId;
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
		init = [&, mesh = &mesh](Mesh* o) {
			std::stringstream ss;
			ss << o->GetDefaultName() << GetId<ExtrusionEditingTool<StereoPolyLineT>>();
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
	

	const glm::vec3 GetRelativeMovement(Input* input) {
		static glm::vec3 zero = glm::vec3();

		if (!input->IsPressed(Key::Modifier::Alt) || input->movement == zero)
			return transformPos - transformOldPos;

		return input->movement * Settings::TranslationStep().Get();
	}
	const glm::vec3 GetRelativeRotation(Input* input) {
		static glm::vec3 zero = glm::vec3();

		if (!input->IsPressed(Key::Modifier::Control) || input->movement == zero)
			return angle - oldAngle;

		// If all 3 axes are modified then don't apply such rotation.
		// Quaternion can't rotate around 3 axes.
		if (input->movement.x && input->movement.y && input->movement.z)
			return angle - oldAngle;

		auto mouseThresholdMin = 0.8;
		auto mouseThresholdMax = 1.25;
		auto mouseAxe = input->MouseAxe;
		auto maxAxe = 0;

		// Find non-zero axes
		int t[2] = {0,0};
		size_t n = 0;
		for (size_t i = 0; i < 3; i++) {
			if (mouseAxe[i] != 0)
				t[n] = i;
		}

		// Nullify weak axes (those with small values)
		auto ratio = abs(mouseAxe[t[0]]) / abs(mouseAxe[t[1]]);
		if (ratio < mouseThresholdMin)
			mouseAxe[t[0]] = 0;
		else if (ratio > mouseThresholdMax)
			mouseAxe[t[1]] = 0;
		// If 2 axes are used simultaneously it breaks the quaternion somehow.
		else
			mouseAxe = zero;

		mouseAxe *= Settings::MouseSensivity().Get() * input->MouseSpeed();

		auto na = (input->ArrowAxe + input->NumpadAxe + mouseAxe) * Settings::RotationStep().Get();
		angle += na;
		return na;
	}
	const float GetRelativeScale(Input* input) {
		static glm::vec3 zero = glm::vec3();

		if (!input->IsPressed(Key::Modifier::Shift) || input->movement == zero)
			return scale;

		return scale + input->movement.x * Settings::ScalingStep().Get();
	}

	void ProcessInput(const ObjectType& type, const Mode& mode, Input* input) {
		static glm::vec3 zero = glm::vec3();

		if (!input->IsContinuousInputNoDelay()) {
			isBeingModified() = false;
			wasCommitDone = false;
		}

		// We can move cross without having any objects bind.
		if (!input->IsContinuousInputNoDelay()) {
			transformPos = transformOldPos = zero;
			angle = oldAngle = zero;
			scale = oldScale = 1;
		}

		const auto relativeScale = GetRelativeScale(input);
		const auto relativeMovement = GetRelativeMovement(input);
		const auto relativeRotation = GetRelativeRotation(input);

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
			if (relativeScale == oldScale)
				return;

			Scale(cross->GetWorldPosition(), oldScale, relativeScale, targets.parentObjects);
			oldScale = scale = relativeScale;
			break;
		case Mode::Rotate:
			MoveCross(relativeMovement);
			if (relativeRotation == zero)
				return;

			Rotate(cross->GetWorldPosition(), relativeRotation, targets.parentObjects);
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
		if (shouldTrace)
			Trace(targets);

		for (auto target : targets) {
			target->SetWorldPosition((target->GetWorldPosition() - center) / oldScale * scale + center);
			for (size_t i = 0; i < target->GetVertices().size(); i++)
				target->SetVertice(i, (target->GetVertices()[i] - center) / oldScale * scale + center);
		}
	}
	void Translate(const glm::vec3& transformVector, std::list<PON>& targets) {
		if (shouldTrace)
			Trace(targets);

		// Need to calculate average rotation.
		// https://stackoverflow.com/questions/12374087/average-of-multiple-quaternions/27410865#27410865
		if (Settings::SpaceMode().Get() == SpaceMode::Local){
			auto r = glm::rotate(cross->GetWorldRotation(), transformVector);

			MoveCross(r);
			for (auto o : targets) {
				o->SetWorldPosition(o->GetWorldPosition() + r);
				for (size_t i = 0; i < o->GetVertices().size(); i++)
					o->SetVertice(i, o->GetVertices()[i] + r);
			}
			return;
		}

		MoveCross(transformVector);
		for (auto o : targets) {
			o->SetWorldPosition(o->GetWorldPosition() + transformVector);
			for (size_t i = 0; i < o->GetVertices().size(); i++)
				o->SetVertice(i, o->GetVertices()[i] + transformVector);
		}
	}
	void Rotate(const glm::vec3& center, const glm::vec3& rotation, std::list<PON>& targets) {
		if (shouldTrace)
			Trace(targets);

		auto axe = glm::vec3();
		{
			glm::vec3 t[2] = { glm::vec3(), glm::vec3() };
			size_t n = 0;
			for (size_t i = 0; i < 3; i++) {
				assert(n < 3);
				if (rotation[i] != 0)
					t[n++][i] = rotation[i] > 0 ? 1 : -1;
			}

			if (n == 0)
				return;

			axe = t[1] + t[0];
		}

		float angle = 0;
		{
			for (size_t i = 0; i < 3; i++)
				angle += abs(rotation[i]);
		}


		if (Settings::SpaceMode().Get() == SpaceMode::World)
			cross->SetWorldRotation(cross->unitQuat());

		auto trimmedDeltaAngle = getTrimmedAngle(angle) * 3.1415926f * 2 / 360;
		auto r = glm::angleAxis(trimmedDeltaAngle, axe);

		auto crossOldRotation = cross->GetLocalRotation();
		cross->SetLocalRotation(cross->GetLocalRotation() * r);

		if (Settings::SpaceMode().Get() == SpaceMode::World)
			for (auto& target : targets) {
				target->SetWorldPosition(glm::rotate(r, target->GetWorldPosition() - center) + center);
				for (size_t i = 0; i < target->GetVertices().size(); i++)
					target->SetVertice(i, glm::rotate(r, target->GetVertices()[i] - center) + center);

				target->SetWorldRotation(r * target->GetWorldRotation());
			}
		else {
			auto rIsolated = cross->GetLocalRotation() * glm::inverse(crossOldRotation);
			for (auto& target : targets) {
				// Rotate relative to cross.
				target->SetWorldPosition(glm::rotate(rIsolated, target->GetWorldPosition() - center) + center);
				for (size_t i = 0; i < target->GetVertices().size(); i++)
					target->SetVertice(i, glm::rotate(rIsolated, target->GetVertices()[i] - center) + center);

				target->SetLocalRotation(target->GetLocalRotation() * r);
			}
		}
	}

	void Trace(std::list<PON>& targets) {
		static int id = 0;

		for (auto& o : targets) {
			auto pos = findBack(o->children, std::function([](SceneObject* so) {
				return so->GetType() == TraceObjectT;
				}));

			if (pos < 0) {
				traceObjectTool.destination = o;
				traceObjectTool.init = [&, id = id, target = o](SceneObject* o) {
					std::stringstream ss;
					ss << "TraceObject" << id;
					o->Name = ss.str();
					
					cloneTool.destination = PON(o);
					cloneTool.target = target;
					cloneTool.init = [target](SceneObject* obj) {
						obj->children.clear();
						//obj->SetLocalPosition(target.Get()->GetWorldPosition());
						//obj->SetLocalRotation(target.Get()->GetWorldRotation());
					};
					cloneTool.Create();
				};
				traceObjectTool.Create();

				id++;
			}
			else {
				auto to = PON(o->children[pos]);

				//Logger.Information(to->GetWorldPosition());

				cloneTool.destination = to;
				cloneTool.target = o;
				cloneTool.init = [o](SceneObject* obj) {
					obj->children.clear();
					obj->SetWorldPosition(o.Get()->GetWorldPosition());
					obj->SetWorldRotation(o.Get()->GetWorldRotation());
				};
				cloneTool.Create();
			}
		}
	}


	void MoveCross(const glm::vec3& movement) {
		cross->SetLocalPosition(cross->GetLocalPosition() + movement);
	}


	//bool haveSingleParent(std::list<PON>& targets) {
	//	auto firstParent = targets.front()->GetParent();
	//	
	//	// First object check is redundant.
	//	for (auto& o : targets)
	//		if (o->GetParent() != firstParent)
	//			return false;
	//	
	//	return true;
	//}

	float getTrimmedAngle(float a) {
		int b = a;
		return b % 360 + (a - b);
	}
	void nullifyUntouchedAngles() {
		auto da = angle - oldAngle;
		for (size_t i = 0; i < 3; i++)
			if (da[i] == 0)
				angle[i] = oldAngle [i] = 0;
	}
	int getChangedAxe(const glm::vec3& rotation) {
		for (size_t i = 0; i < 3; i++)
			if (rotation[i] != 0)
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
		if (vs.empty())
			return glm::vec3();

		glm::vec3 sum = glm::vec3();
		for (auto o : vs)
			sum += o;

		return glm::vec3(sum.x / vs.size(), sum.y / vs.size(), sum.z / vs.size());
	}
	static std::vector<glm::vec3> GetWorldPositions(std::list<PON>& vs) {
		std::vector<glm::vec3> l;
		for (auto o : vs)
			if (o.HasValue())
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

	virtual void OnSelectionChanged(const ObjectSelection::Selection& v) override {
		UnbindTool();
		if (v.empty())
			return;

		targets.childObjects.clear();
		targets.orphanedObjects.clear();
		targets.parentObjects.clear();

		for (auto o : v)
			targets.parentObjects.push_back(o);
		//Scene::CategorizeObjects(StateBuffer::RootObject().Get().Get(), v, targets);

		cross->SetLocalPosition(Avg(GetWorldPositions(targets.parentObjects)));
		if (Settings::SpaceMode().Get() == SpaceMode::World)
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
		spaceModeChangeHandlerId = Settings::SpaceMode().OnChanged() += [&](const SpaceMode& v) {
			transformOldPos = transformPos = oldAngle = angle = glm::vec3();
			oldAngle = angle = glm::vec3();

			if (v == SpaceMode::Local) {
				if (!targets.parentObjects.empty() && targets.parentObjects.front().HasValue())
					cross->SetLocalRotation(targets.parentObjects.front()->GetWorldRotation());
				//cross->SetLocalRotation(cross->unitQuat());
			}
			else
				cross->SetWorldRotation(cross->unitQuat());
		};

	}

public:
	ReadonlyProperty<Cross*> cross;

	float scale = 1;
	glm::vec3 angle;
	glm::vec3 transformPos;
	bool isRelativeMode;
	bool shouldTrace;

	CreatingTool<TraceObject> traceObjectTool;
	CloneTool cloneTool;

	virtual bool UnbindSceneObjects() override {
		UnbindTool();
		return true;
	}


	void UnbindTool() {
		if (targets.parentObjects.empty())
			return;

		scale = 1;
		oldScale = 1;
		angle = glm::vec3();
		oldAngle = glm::vec3();
		transformPos = glm::vec3();

		keyBinding->RemoveHandler(inputHandlerId);
		// Remove if exists and add a new one.
		keyBinding->RemoveHandler(cross->keyboardBindingHandlerId);
		cross->keyboardBindingHandlerId = keyBinding->AddHandler(cross->keyboardBindingHandler);

		Settings::SpaceMode().OnChanged().RemoveHandler(spaceModeChangeHandlerId);
		StateBuffer::OnStateChange().RemoveHandler(stateChangedHandlerId);
		SceneObject::OnBeforeAnyElementChanged().RemoveHandler(anyObjectChangedHandlerId);
		DeleteConfig();
	}
	SceneObject* GetTarget() {
		if (targets.parentObjects.empty() || !targets.parentObjects.front().HasValue())
			return nullptr;
		else 
			return targets.parentObjects.front().Get();
	}
	void SetMode(Mode mode) {
		this->mode = mode;
	}
	const Mode& GetMode() {
		return mode;
	}
};