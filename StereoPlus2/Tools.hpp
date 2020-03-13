#pragma once

#include "GLLoader.hpp"
#include "DomainTypes.hpp"
#include "Commands.hpp"
#include "Input.hpp"
#include <map>
#include <stack>
#include <vector>
#include <queue>

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
	std::vector<SceneObject*>* source;

	static int& GetNextFreeId() {
		static int val = 0;
		return val;
	}

	static std::stack<SceneObject*>& GetCreatedObjects() {
		static std::stack<SceneObject*> val;
		return val;
	}
public:
	std::function<void(T*)> func;

	bool BindSource(std::vector<SceneObject*>* source) {
		this->source = source;
		return true;
	}

	bool Create() {
		auto command = new CreateCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;
		command->func = [=] {
			T* obj = new T();
			func(obj);
			GetCreatedObjects().push(obj);

			return obj;
		};

		return true;
	}

	virtual bool Rollback() {
		auto command = new DeleteCommand();
		if (!command->BindScene(scene))
			return false;

		command->source = source;
		command->target = GetCreatedObjects().top();
		GetCreatedObjects().pop();

		return true;
	}
};



class EditingTool : Tool {
protected:
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

#pragma endregion

template<ObjectType type>
class PointPenEditingTool : public EditingTool {
#pragma region Types
	using Mode = PointPenEditingToolMode;

	template<ObjectType type, Mode mode>
	struct Config : EditingTool::Config {

	};
	template<>
	struct Config<StereoPolyLineT, Mode::Immediate> : EditingTool::Config {
		int additionalPointCreatedCount = 0;

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

	size_t handlerId;
	Mode mode;

	Cross* cross = nullptr;
	SceneObject* target = nullptr;

	template<Mode mode>
	Config<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new Config<type, mode>();

		return (Config<type, mode>*) config;
	}

	template<typename T>
	T* GetTarget() {
		return (T*)this->target;
	}

#pragma region ProcessInput

	template<ObjectType type, Mode mode>
	void ProcessInput(Input* input) {
		std::cout << "Unsupported Editing Tool target Type or Unsupported combination of ObjectType and PointPenEditingToolMode" << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Mode::Immediate>(Input* input) {
		if (input->IsDown(Key::Escape))
		{
			UnbindSceneObjects();
			return;
		}
		auto points = &GetTarget<StereoPolyLine>()->Points;
		auto pointsCount = points->size();


		if (GetConfig<Mode::Immediate>()->additionalPointCreatedCount < 3)
		{
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			points->push_back(cross->Position);
			GetConfig<Mode::Immediate>()->additionalPointCreatedCount++;

			return;
		}



		// Drawing optimizing
		
		// If cross is located at distance less than Precision then don't create a new point
		// but move the last one to cross position.
		if (glm::length(cross->Position - (*points)[pointsCount - 2]) < GetConfig<Mode::Immediate>()->Precision)
		{
			(*points)[pointsCount - 1] = points->back() = cross->Position;
			return;
		}


		// If the line goes straight then instead of adding 
		// a new point - move the previous point to current cross position.
		auto E = GetConfig<Mode::Immediate>()->E;

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

		//std::cout << "PointPen tool Immediate mode points count: " << pointsCount << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Mode::Step>(Input* input) {
		if (input->IsDown(Key::Escape))
		{
			UnbindSceneObjects();
			return;
		}
		auto points = &GetTarget<StereoPolyLine>()->Points;
		auto pointsCount = points->size();

		if (!GetConfig<Mode::Step>()->isPointCreated)
		{
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			points->push_back(cross->Position);
			GetConfig<Mode::Step>()->isPointCreated = true;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter))
		{
			points->push_back(cross->Position);

			return;
		}

		points->back() = cross->Position;
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

public:
	std::vector<std::function<void()>> onTargetReleased;



	virtual bool BindSceneObjects(std::vector<SceneObject*> objs) {
		if (target != nullptr && !UnbindSceneObjects())
			return false;

		if (keyBinding == nullptr)
		{
			std::cout << "KeyBinding wasn't assigned" << std::endl;
			return false;
		}

		if (objs[0]->GetType() != type)
		{
			std::cout << "Invalid Object passed to PointPenEditingTool" << std::endl;
			return true;
		}
		target = objs[0];

		handlerId = keyBinding->AddHandler([this](Input * input) { this->ProcessInput(input); });

		return true;
	}
	virtual bool UnbindSceneObjects() {
		UnbindSceneObjects<type>();
		
		for (auto func : onTargetReleased)
			func();

		keyBinding->RemoveHandler(handlerId);

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
			std::cout << "Not suported mode was given" << std::endl;
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

		if (target->Points.size() > 0)
			target->Points.pop_back();

		this->target = nullptr;

		//existingPointsSelected.clear();
	}
	template<>
	void UnbindSceneObjects<StereoPolyLineT, Mode::Step>() {
		if (this->target == nullptr)
			return;

		auto target = (StereoPolyLine*)this->target;

		if (target->Points.size() > 0)
			target->Points.pop_back();

		this->target = nullptr;

		//existingPointsSelected.clear();
	}

	bool BindCross(Cross* cross) {
		this->cross = cross;
		return true;
	}

	void SetMode(Mode mode) {
		DeleteConfig();
		//switch (mode)
		//{
		//case Mode::Immediate:
		//	//GetConfig<Mode::Immediate>()->additionalPointCreatedCount = false;
		//	break;
		//default:
		//	break;
		//}
		//UnbindSceneObjects();
		this->mode = mode;
	}
};





template<ObjectType type>
class ExtrusionEditingTool : public EditingTool, public CreatingTool<LineMesh> {
#pragma region Types
	using Mode = ExtrusionEditingToolMode;

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

	size_t handlerId;
	Mode mode;

	Cross* cross = nullptr;

	Mesh* mesh = nullptr;
	SceneObject* pen = nullptr;
	glm::vec3 startCrossPosition;

	template<Mode mode>
	Config<type, mode>* GetConfig() {
		if (config == nullptr)
			config = new Config<type, mode>();

		return (Config<type, mode>*) config;
	}

	template<typename T>
	T* GetTarget() {
		return (T*)this->pen;
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

		if (input->IsDown(Key::Escape))
		{
			UnbindSceneObjects();
			return;
		}

		auto meshPoints = mesh->GetVertices();
		auto penPoints = &((StereoPolyLine*)pen)->Points;
		auto transformVector = cross->Position - startCrossPosition;


		if (GetConfig<Mode::Immediate>()->directingPoints.size() < 1)
		{
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			GetConfig<Mode::Immediate>()->directingPoints.push_back(cross->Position);

			mesh->AddVertice((*penPoints)[0] + transformVector);

			for (size_t i = 1; i < penPoints->size(); i++) {
				mesh->Connect(i - 1, i);
				mesh->AddVertice((*penPoints)[i] + transformVector);
			}

			return;
		}
		else if (GetConfig<Mode::Immediate>()->directingPoints.size() < 2)
		{
			// We need to select one point and create an additional point
			// so that we can perform some optimizations.
			GetConfig<Mode::Immediate>()->directingPoints.push_back(cross->Position);

			mesh->Connect(meshPoints->size() - penPoints->size(), meshPoints->size());
			mesh->AddVertice((*penPoints)[0] + transformVector);

			for (size_t i = 1; i < penPoints->size(); i++) {
				mesh->Connect(meshPoints->size() - penPoints->size(), meshPoints->size());
				mesh->Connect(meshPoints->size() - 1, meshPoints->size());
				mesh->AddVertice((*penPoints)[i] + transformVector);
			}

			return;
		}

		auto E = GetConfig<Mode::Immediate>()->E;

		auto directingPoints = &GetConfig<Mode::Immediate>()->directingPoints;

		glm::vec3 r1 = cross->Position - (*directingPoints)[1];
		glm::vec3 r2 = (*directingPoints)[0] - (*directingPoints)[1];


		auto p = glm::dot(r1, r2);
		auto l1 = glm::length(r1);
		auto l2 = glm::length(r2);

		auto cos = p / l1 / l2;

		if (abs(cos) > 1 - E || isnan(cos))
		{
			for (size_t i = 0; i < penPoints->size(); i++) {
				(*meshPoints)[meshPoints->size() - penPoints->size() + i] = (*penPoints)[i] + transformVector;
			}
		
			(*directingPoints)[1] = cross->Position;
		}
		else
		{
			mesh->Connect(meshPoints->size() - penPoints->size(), meshPoints->size());
			mesh->AddVertice((*penPoints)[0] + transformVector);

			for (size_t i = 1; i < penPoints->size(); i++) {
				mesh->Connect(meshPoints->size() - penPoints->size(), meshPoints->size());
				mesh->Connect(meshPoints->size() - 1, meshPoints->size());
				mesh->AddVertice((*penPoints)[i] + transformVector);
			}
			
			directingPoints->erase(directingPoints->begin());
			directingPoints->push_back(cross->Position);
		}
		
		//std::cout << "PointPen tool Immediate mode points count: " << meshPoints->size() << std::endl;
	}
	template<>
	void ProcessInput<StereoPolyLineT, Mode::Step>(Input* input) {
		if (mesh == nullptr)
			return;
		
		if (input->IsDown(Key::Escape))
		{
			UnbindSceneObjects();
			return;
		}

		auto meshPoints = mesh->GetVertices();
		auto penPoints = &((StereoPolyLine*)pen)->Points;

		auto transformVector = cross->Position - startCrossPosition;

		if (!GetConfig<Mode::Step>()->isPointCreated)
		{
			mesh->AddVertice((*penPoints)[0] + transformVector);

			for (size_t i = 1; i < penPoints->size(); i++) {
				mesh->Connect(i - 1, i);
				mesh->AddVertice((*penPoints)[i] + transformVector);
			}

			GetConfig<Mode::Step>()->isPointCreated = true;

			return;
		}

		if (input->IsDown(Key::Enter) || input->IsDown(Key::NEnter))
		{
			mesh->Connect(meshPoints->size() - penPoints->size(), meshPoints->size());
			mesh->AddVertice((*penPoints)[0] + transformVector);

			for (size_t i = 1; i < penPoints->size(); i++) {
				mesh->Connect(meshPoints->size() - penPoints->size(), meshPoints->size());
				mesh->Connect(meshPoints->size() - 1, meshPoints->size());
				mesh->AddVertice((*penPoints)[i] + transformVector);
			}
			return;
		}

		auto iter = ((std::vector<glm::vec3>*)meshPoints)->end() - penPoints->size();
		for (size_t i = 0; i < penPoints->size(); i++, iter++)
			*(iter._Ptr) = (*penPoints)[i] + transformVector;
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

		auto penPoints = &((StereoPolyLine*)pen)->Points;

		if (GetConfig<Mode::Step>()->isPointCreated && mesh->GetVertices()->size() > 0) {
			if (mesh->GetVertices()->size() > penPoints->size())
			{
				for (size_t i = 1; i < penPoints->size(); i++)
				{
					mesh->RemoveVertice(mesh->GetVertices()->size() - 1);
					mesh->Disconnect(mesh->GetVertices()->size() - penPoints->size(), mesh->GetVertices()->size());
					mesh->Disconnect(mesh->GetVertices()->size() - 1, mesh->GetVertices()->size());
				}

				mesh->RemoveVertice(mesh->GetVertices()->size() - 1);
				mesh->Disconnect(mesh->GetVertices()->size() - penPoints->size(), mesh->GetVertices()->size());
			}
			else
			{
				for (size_t i = 1; i < penPoints->size(); i++)
				{
					mesh->RemoveVertice(mesh->GetVertices()->size() - 1);
					mesh->Disconnect(mesh->GetVertices()->size() - 1, mesh->GetVertices()->size());
				}

				mesh->RemoveVertice(mesh->GetVertices()->size() - 1);
			}
		}

		this->pen = nullptr;
	}

	void ResetTool() {
		keyBinding->RemoveHandler(handlerId);

		DeleteConfig();
	}

	template<typename T>
	static int GetId() {
		static int i = 0;
		return i++;
	}


public:

	ExtrusionEditingTool() {
		func = [mesh = &mesh](LineMesh* o) {
			std::stringstream ss;
			ss << o->GetDefaultName() << GetId<ExtrusionEditingTool<StereoPolyLineT>>();
			o->Name = ss.str();
			*mesh = o;
		};
	}

	virtual bool BindSceneObjects(std::vector<SceneObject*> objs) {
		if (pen != nullptr && !UnbindSceneObjects())
			return false;

		if (keyBinding == nullptr)
		{
			std::cout << "KeyBinding wasn't assigned" << std::endl;
			return false;
		}

		if (objs[0]->GetType() != type)
		{
			std::cout << "Invalid Object passed to ExtrusionEditingTool" << std::endl;
			return true;
		}
		pen = objs[0];

		handlerId = keyBinding->AddHandler([this](Input * input) { this->ProcessInput(input); });

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
		//UnbindSceneObjects();
		DeleteConfig();
		this->mode = mode;
	}

	bool Create() {
		if (pen == nullptr) {
			std::cout << "Pen was not assigned" << std::endl;
			return false;
		}

		DeleteConfig();

		startCrossPosition = cross->Position;
		
		return CreatingTool<LineMesh>::Create();
	};
};


template<ObjectType type>
class TransformTool : EditingTool {

};