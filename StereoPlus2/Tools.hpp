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

template<ObjectType type>
class PointPenEditingTool : public EditingTool {
	size_t handlerId;

	Cross* cross = nullptr;
	SceneObject* target = nullptr;


	std::vector<glm::vec3*> existingPointsSelected;


	template<typename T>
	T* GetTarget() {
		return (T*)this->target;
	}

	template<ObjectType type>
	void ProcessInput(Input* input) {
		std::cout << "Unsupported Editing Tool target Type" << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT>(Input* input) {
		if (input->IsDown(Key::Escape))
		{
			ReleaseTarget();
			return;
		}
		auto points = &GetTarget<StereoPolyLine>()->Points;
		auto pointsCount = points->size();

		if (isImmediateMode)
		{

			if (existingPointsSelected.size() == 0)
			{
				// We need to select one point and create an additional point
				// so that we can perform some optimizations.
				//SelectPoint<StereoPolyLine>(input);
					points->push_back(cross->Position);
					if (pointsCount == 0)
					{
						existingPointsSelected.push_back(&(*points)[0]);
					}
					else
					{
						existingPointsSelected.push_back(&(*points)[pointsCount - 1]);
					}
			}

			// Drawing oprimizing
			// If the line goes straight then instead of adding 
			// a new point - move the previous point to current cross position.
			if (pointsCount > 2)
			{
				auto E = 1e-6;

				glm::vec3 r1 = cross->Position - (*points)[pointsCount - 1];
				glm::vec3 r2 = (*points)[pointsCount - 3] - (*points)[pointsCount - 2];

				auto p = glm::dot(r1, r2);
				auto l1 = glm::length(r1);
				auto l2 = glm::length(r2);

				auto cos = p / l1 / l2;
				
				if (abs(cos) > 1 - E || isnan(cos))
				{
					(*points)[pointsCount - 2] = cross->Position;
				}
				else
				{
					AddPoint<StereoPolyLine>(input);

					existingPointsSelected[0] = &points->back();
				}
			}
			else
			{
				AddPoint<StereoPolyLine>(input);

				existingPointsSelected[0] = &points->back();
			}

			std::cout << "PointPen tool Immediate mode Added points count: " << pointsCount << std::endl;

			if (pointsCount > 1)
			{
				points->back() = cross->Position;
			}

			return;
		}


		else if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter))
		{
			points->push_back(cross->Position);

			if (existingPointsSelected.size() == 0)
			{
				points->push_back(cross->Position);
				existingPointsSelected.push_back(&(*points)[0]);
			}
			else
			{
				existingPointsSelected[0] = &(*points)[pointsCount - 1];
			}

			return;
		}

		if (pointsCount > 1)
		{
			points->back() = cross->Position;
		}

		
		
	}

	template<typename T>
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

	template<typename T>
	void AddPoint(Input* input) {
		std::cout << "Unsupported Editing Tool target Type" << std::endl;
	}
	template<>
	void AddPoint<StereoPolyLine>(Input* input) {
		auto target = (StereoPolyLine*)this->target;

		if (isnan(cross->Position.x) || isnan(cross->Position.y) || isnan(cross->Position.z))
		{
			int i = 0;

		}
		target->Points.push_back(cross->Position);

		existingPointsSelected.push_back(&target->Points[target->Points.size() - 1]);
	}


public:
	std::vector<std::function<void()>> onTargetReleased;
	bool isImmediateMode = true;

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

		return true;
	}

	template<ObjectType type>
	void ReleaseTarget() {

	}

	template<>
	void ReleaseTarget <StereoPolyLineT>() {
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

};

