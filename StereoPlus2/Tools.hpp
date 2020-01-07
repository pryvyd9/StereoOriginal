#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include "Input.hpp"
#include <map>
#include <stack>
#include <vector>


class Tool {
protected:
	struct Change {
		SceneObject* reference, * stateCopy;
	};
	static std::stack<Change>& GetChanges() {
		static auto val = std::stack<Change>();
		return val;
	}
public:
	virtual bool Rollback() {
		if (GetChanges().size() <= 0)
			return true;
		
		auto change = GetChanges().top();
		change.reference = change.stateCopy;
		
		return true;
	}
};

template<typename T>
class CreatingTool : Tool, public ISceneHolder {

	struct Pointer {
		int objectCreationId;
		SceneObject* obj;
	};

	T* obj;
	std::vector<SceneObject*>* source;
	int currentCreatedId;

	static int& GetNextFreeId() {
		static int val = 0;
		return val;
	}

	static std::map<int, SceneObject*>& GetCreatedObjects() {
		static auto val = std::map<int, SceneObject*>();
		return val;
	}
public:
	std::function<bool(SceneObject*)> initFunc;

	bool BindSource(std::vector<SceneObject*>* source) {
		this->source = source;
		return true;
	}
	bool Create(SceneObject*** createdObj) {
		currentCreatedId = GetNextFreeId()++;
		GetCreatedObjects()[currentCreatedId] = nullptr;

		// If passed nullptr then we should ignore created object.
		if (createdObj != nullptr)
			*createdObj = &GetCreatedObjects()[currentCreatedId];

		auto command = new CreateCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;

		command->func = [=] {
			T* obj = new T();
			initFunc(obj);
			GetCreatedObjects()[currentCreatedId] = obj;
			return obj;
		};

		return true;
	}
	virtual bool Rollback() {
		auto command = new DeleteCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;
		command->target = &GetCreatedObjects()[currentCreatedId];

		return true;
	}

};


class EditingTool : Tool {
protected:
	KeyBinding* keyBinding = nullptr;
public:
	virtual bool SelectTarget(SceneObject* obj) = 0;
	virtual bool ReleaseTarget() = 0;
	
	virtual bool BindInput(KeyBinding* keyBinding) {
		this->keyBinding = keyBinding;
		return true;
	}
};

enum PointPenEditingToolMode {
	Immediate,
	Step,
};


struct PointPenEditingToolConfigAbstract {
	template<typename T>
	T* Get() { return (T*)this; };
};
template<ObjectType type, PointPenEditingToolMode mode>
struct PointPenEditingToolConfig : PointPenEditingToolConfigAbstract {
	
};
template<>
struct PointPenEditingToolConfig<StereoPolyLineT, Immediate> : PointPenEditingToolConfigAbstract {
	bool isPointCreated = false;

	// If the cos between vectors is less than E
	// then we merge those vectors.
	double E = 1e-6;
};
template<>
struct PointPenEditingToolConfig<StereoPolyLineT, Step> : PointPenEditingToolConfigAbstract {
	bool isPointCreated = false;
};

template<ObjectType type>
class PointPenEditingTool : public EditingTool {
	size_t handlerId;
	PointPenEditingToolMode mode;

	Cross* cross = nullptr;
	SceneObject* target = nullptr;


	std::vector<glm::vec3*> existingPointsSelected;

	PointPenEditingToolConfigAbstract* config = nullptr;

	template<PointPenEditingToolMode mode>
	PointPenEditingToolConfig<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new PointPenEditingToolConfig<type, mode>();

		return (PointPenEditingToolConfig<type, mode>*) config;
	}

	template<typename T>
	T* GetTarget() {
		return (T*)this->target;
	}

#pragma region ProcessInput

	template<ObjectType type, PointPenEditingToolMode mode>
	void ProcessInput(Input* input) {
		std::cout << "Unsupported Editing Tool target Type or Unsupported combination of ObjectType and PointPenEditingToolMode" << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Immediate>(Input* input) {
		if (input->IsDown(Key::Escape))
		{
			ReleaseTarget();
			return;
		}
		auto points = &GetTarget<StereoPolyLine>()->Points;
		auto pointsCount = points->size();

		if (!GetConfig<Immediate>()->isPointCreated)
		{
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			points->push_back(cross->Position);
			GetConfig<Immediate>()->isPointCreated = true;
		}

		// Drawing optimizing
		// If the line goes straight then instead of adding 
		// a new point - move the previous point to current cross position.
		if (pointsCount > 2)
		{
			auto E = GetConfig<Immediate>()->E;

			glm::vec3 r1 = cross->Position - (*points)[pointsCount - 1];
			glm::vec3 r2 = (*points)[pointsCount - 3] - (*points)[pointsCount - 2];

			auto p = glm::dot(r1, r2);
			auto l1 = glm::length(r1);
			auto l2 = glm::length(r2);

			auto cos = p / l1 / l2;
				
			if (abs(cos) > 1 - E || isnan(cos))
			{
				(*points)[pointsCount - 2] = points->back() = cross->Position;
			}
			else
			{
				points->push_back(cross->Position);
			}
		}
		else
		{
			points->push_back(cross->Position);
		}

		//std::cout << "PointPen tool Immediate mode points count: " << pointsCount << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Step>(Input* input) {
		if (input->IsDown(Key::Escape))
		{
			ReleaseTarget();
			return;
		}
		auto points = &GetTarget<StereoPolyLine>()->Points;
		auto pointsCount = points->size();

		if (!GetConfig<Step>()->isPointCreated)
		{
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			points->push_back(cross->Position);
			GetConfig<Step>()->isPointCreated = true;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter))
		{
			points->push_back(cross->Position);

			return;
		}

		points->back() = cross->Position;
	}

	template<ObjectType type>
	void ProcessInput(Input* input) {
		switch (mode)
		{
		case Immediate:
			return ProcessInput<type, Immediate>(input);
		case Step:
			return ProcessInput<type, Step>(input);
		default:
			std::cout << "Not suported mode was given" << std::endl;
			return;
		}
	}

#pragma endregion

	/*template<typename T>
	void SelectPoint(Input* input) {
		std::cout << "Unsupported Editing Tool target Type" << std::endl;
	}
	template<>
	void SelectPoint<StereoPolyLine>(Input* input) {
		auto target = (StereoPolyLine*)this->target;


		auto crossViewSurfacePos = glm::vec2(cross->Position.x, cross->Position.y);
		if (target->Points.size() == 0)
		{
			target->Points.push_back(cross->Position);
		}

		auto closestPoint = &target->Points[0];
		auto viewSurfacePos = *(glm::vec2*)closestPoint;
		double minDistance = glm::distance(crossViewSurfacePos, viewSurfacePos);

		for (size_t i = 1; i < target->Points.size(); i++)
		{
			viewSurfacePos = *(glm::vec2*)&target->Points[i];
			auto currentDistance = glm::distance(crossViewSurfacePos, viewSurfacePos);
			if (minDistance > currentDistance)
				closestPoint = &target->Points[0];
		}

		existingPointsSelected.push_back(closestPoint);
	}
*/
	//template<typename T>
	//void AddPoint(Input* input) {
	//	std::cout << "Unsupported Editing Tool target Type" << std::endl;
	//}
	//template<>
	//void AddPoint<StereoPolyLine>(Input* input) {
	//	auto target = (StereoPolyLine*)this->target;

	//	target->Points.push_back(cross->Position);

	//	existingPointsSelected.push_back(&target->Points[target->Points.size() - 1]);
	//}


public:
	std::vector<std::function<void()>> onTargetReleased;

	virtual bool SelectTarget(SceneObject* obj) {
		if (target != nullptr && !ReleaseTarget())
			return false;

		if (keyBinding == nullptr)
		{
			std::cout << "KeyBinding wasn't assigned" << std::endl;
			return false;
		}

		if (obj->GetType() != type)
		{
			std::cout << "Invalid Object passed to PointPenEditingTool" << std::endl;
			return true;
		}
		target = obj;

		handlerId = keyBinding->AddHandler([this](Input * input) { this->ProcessInput<type>(input); });

		return true;
	}
	virtual bool ReleaseTarget() {
		ReleaseTarget<type>();
		
		for (auto func : onTargetReleased)
			func();

		keyBinding->RemoveHandler(handlerId);

		if (config != nullptr)
		{
			delete config;
			config = nullptr;
		}

		return true;
	}

	SceneObject** GetTarget() {
		return &target;
	}

	template<ObjectType type>
	void ReleaseTarget() {
		switch (mode)
		{
		case Immediate:
			return ReleaseTarget<type, Immediate>();
		case Step:
			return ReleaseTarget<type, Step>();
		default:
			std::cout << "Not suported mode was given" << std::endl;
			return;
		}

	}

	template<ObjectType type, PointPenEditingToolMode mode>
	void ReleaseTarget() {
		
	}
	template<>
	void ReleaseTarget<StereoPolyLineT, Immediate>() {
		if (this->target == nullptr)
			return;

		auto target = (StereoPolyLine*)this->target;

		if (target->Points.size() > 0)
			target->Points.pop_back();

		this->target = nullptr;

		existingPointsSelected.clear();
	}
	template<>
	void ReleaseTarget<StereoPolyLineT, Step>() {
		if (this->target == nullptr)
			return;

		auto target = (StereoPolyLine*)this->target;

		if (target->Points.size() > 0)
			target->Points.pop_back();

		this->target = nullptr;

		existingPointsSelected.clear();
	}

	bool BindCross(Cross* cross) {
		this->cross = cross;
		return true;
	}

	void SetPointPenEditingToolMode(PointPenEditingToolMode mode) {
		ReleaseTarget();
		this->mode = mode;
	}


};

