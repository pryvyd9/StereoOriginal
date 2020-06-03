#pragma once
#include "GLLoader.hpp"
#include <stdlib.h>
#include <set>
#include <array>

#pragma region Scene Objects

enum ObjectType {
	Group,
	StereoPolyLineT,
	MeshT,
	CameraT,
	CrossT,
};

enum InsertPosition {
	Top = 0x01,
	Bottom = 0x10,
	Center = 0x100,
	Any = Top | Bottom | Center,
};

struct Pair {
	glm::vec3 p1, p2;
};

class SceneObject {
	glm::vec3 position;
	glm::fquat rotation = unitQuat();
	SceneObject* parent;


protected:
	bool shouldUpdateCache;

	void CascadeTransform(std::vector<glm::vec3>& vertices) const {
		if (GetLocalRotation() == unitQuat())
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] = vertices[i] + GetLocalPosition();
		else
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] = glm::rotate(GetLocalRotation(), vertices[i]) + GetLocalPosition();

		if (parent)
			parent->CascadeTransform(vertices);
	}
	void CascadeTransform(glm::vec3& v) const {
		if (GetLocalRotation() == unitQuat())
			v += GetLocalPosition();
		else
			v = glm::rotate(GetLocalRotation(), v) + GetLocalPosition();

		if (parent)
			parent->CascadeTransform(v);
	}
	void CascadeTransformInverse(glm::vec3& v) const {
		if (GetLocalRotation() == unitQuat())
			v -= GetLocalPosition();
		else
			v = glm::rotate(glm::inverse(GetLocalRotation()), v - GetLocalPosition());

		if (parent)
			parent->CascadeTransform(v);
	}

public:
	std::vector<SceneObject*> children;

	std::string Name = "noname";

	constexpr const glm::fquat unitQuat() const {
		return glm::fquat(1, 0, 0, 0);
	}

	//glm::vec3 GetRight() {
	//	return glm::rotate(GetLocalRotation(), glm::vec3(1, 0, 0));
	//}
	//glm::vec3 GetUp() {
	//	return glm::rotate(GetLocalRotation(), glm::vec3(0, 1, 0));
	//}
	//glm::vec3 GetForward() {
	//	return glm::rotate(GetLocalRotation(), glm::vec3(0, 0, 1));
	//}

	const SceneObject* GetParent() const {
		return parent;
	}
	void SetParent(SceneObject* newParent, int newParentPos, InsertPosition pos) {
		ForceUpdateCache();
		auto oldPosition = GetWorldPosition();
		auto oldRotation = GetWorldRotation();

		auto source = &parent->children;
		auto dest = &newParent->children;

		parent = newParent;

		SetWorldPosition(oldPosition);
		SetWorldRotation(oldRotation);

		auto sourcePositionInt = find(*source, this);

		if (dest->size() == 0) {
			dest->push_back(this);
			source->erase(source->begin() + sourcePositionInt);
			return;
		}

		if ((InsertPosition::Bottom & pos) != 0) {
			newParentPos++;
		}

		if (source == dest && newParentPos < (int)sourcePositionInt) {
			dest->erase(source->begin() + sourcePositionInt);
			dest->insert(dest->begin() + newParentPos, 1, this);
			return;
		}

		dest->insert(dest->begin() + newParentPos, 1, this);
		source->erase(source->begin() + sourcePositionInt);
	}
	void SetParent(SceneObject* newParent, bool shouldConvertValues = false) {
		ForceUpdateCache();
		
		if (parent && parent->children.size() > 0) {
			auto pos = std::find(parent->children.begin(), parent->children.end(), this);
			if (pos != parent->children.end())
				parent->children.erase(pos);
		}

		if (shouldConvertValues) {
			auto oldPosition = GetWorldPosition();
			auto oldRotation = GetWorldRotation();

			parent = newParent;

			SetWorldPosition(oldPosition);
			SetWorldRotation(oldRotation);
		}
		else {
			parent = newParent;
		}


		if (newParent)
			newParent->children.push_back(this);
	}

	glm::vec3 ToWorldPosition(const glm::vec3& v) const {
		glm::vec3 r = v;
		CascadeTransform(r);
		return r;
	}
	glm::vec3 ToLocalPosition(const glm::vec3& v) const {
		glm::vec3 r = v;
		CascadeTransformInverse(r);
		return r;
	}

	virtual ObjectType GetType() const = 0;
	virtual std::string GetDefaultName() {
		return "SceneObject";
	}

	const glm::vec3& GetLocalPosition() const {
		return position;
	}
	const glm::vec3 GetWorldPosition() const {
		if (parent)
			return ToWorldPosition(glm::vec3());
			//return GetLocalPosition() + parent->GetWorldPosition();
		
		return GetLocalPosition();
	}
	void SetLocalPosition(const glm::vec3& v) {
		ForceUpdateCache();
		position = v;
	}
	void SetWorldPosition(const glm::vec3& v) {
		ForceUpdateCache();

		if (parent) {
			position = parent->ToLocalPosition(v);
			return;
		}

		position = v;
	}

	const glm::quat& GetLocalRotation() const {
		return rotation;
	}
	const glm::quat GetWorldRotation() const {
		if (parent)
			//return glm::cross(GetLocalRotation(), parent->GetWorldRotation());
			return parent->GetWorldRotation() * GetLocalRotation();

		return GetLocalRotation();
	}
	void SetLocalRotation(const glm::quat& v) {
		ForceUpdateCache();
		rotation = v;
	}
	void SetWorldRotation(const glm::quat& v) {
		ForceUpdateCache();

		if (parent) {
			rotation = glm::inverse(parent->GetWorldRotation()) * v;
			return;
		}

		rotation = v;
	}

	void ForceUpdateCache() {
		shouldUpdateCache = true;
		for (auto c : children)
			c->ForceUpdateCache();
	}

	virtual const std::vector<Pair>& GetLines() {
		static const std::vector<Pair> empty;
		return empty;
	}
	virtual const std::vector<glm::vec3>& GetVertices() const {
		static const std::vector<glm::vec3> empty;
		return empty;
	}

	virtual void AddVertice(const glm::vec3& v) {}
	virtual void AddVertices(const std::vector<glm::vec3>& vs) {}
	virtual void SetVertice(size_t index, const glm::vec3& v) {}
	virtual void SetVerticeX(size_t index, const float& v) {}
	virtual void SetVerticeY(size_t index, const float& v) {}
	virtual void SetVerticeZ(size_t index, const float& v) {}
	virtual void SetVertices(const std::vector<glm::vec3>& vs) {}

	virtual void RemoveVertice() {}
};

class GroupObject : public SceneObject {
public:
	virtual ObjectType GetType() const {
		return Group;
	}
};

class LeafObject : public SceneObject {
};

class StereoPolyLine : public LeafObject {
	std::vector<Pair> linesCache;
	std::vector<glm::vec3> vertices;

	void UpdateCache() {
		if (vertices.size() < 2) {
			linesCache.clear();
			return;
		}

		auto transformedVertices = vertices;
		CascadeTransform(transformedVertices);

		linesCache = std::vector<Pair>(vertices.size() - 1);

		for (size_t i = 0; i < vertices.size() - 1; i++) {
			linesCache[i].p1 = transformedVertices[i];
			linesCache[i].p2 = transformedVertices[i + 1];
		}

		shouldUpdateCache = false;
	}

public:

	StereoPolyLine() {}

	StereoPolyLine(StereoPolyLine& copy) {
		SetVertices(copy.GetVertices());
	}

	virtual ObjectType GetType() const {
		return StereoPolyLineT;
	}

	virtual const std::vector<Pair>& GetLines() {
		if (shouldUpdateCache)
			UpdateCache();

		return linesCache;
	}
	virtual const std::vector<glm::vec3>& GetVertices() const {
		return vertices;
	}

	virtual void AddVertice(const glm::vec3& v) {
		vertices.push_back(v);
		shouldUpdateCache = true;
	}
	virtual void AddVertices(const std::vector<glm::vec3>& vs) {
		for (auto v : vs)
			AddVertice(v);
	}
	virtual void SetVertice(size_t index, const glm::vec3& v) {
		vertices[index] = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeX(size_t index, const float& v) {
		vertices[index].x = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeY(size_t index, const float& v) {
		vertices[index].y = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeZ(size_t index, const float& v) {
		vertices[index].z = v;
		shouldUpdateCache = true;
	}
	virtual void SetVertices(const std::vector<glm::vec3>& vs) {
		vertices.clear();
		linesCache.clear();
		for (auto v : vs)
			AddVertice(v);
		shouldUpdateCache = true;
	}

	virtual void RemoveVertice() {
		linesCache.pop_back();
		vertices.pop_back();
		shouldUpdateCache = true;
	}
};

struct Mesh : LeafObject {
private:
	std::vector<glm::vec3> vertices;
	std::vector<Pair> linesCache;
	std::vector<std::array<size_t, 2>> lines;


	void UpdateCache() {
		if (lines.size() < 1) {
			linesCache.clear();
			return;
		}

		auto transformedVertices = vertices;
		CascadeTransform(transformedVertices);

		linesCache = std::vector<Pair>(lines.size());

		for (size_t i = 0; i < lines.size() - 1; i++) {
			linesCache[i].p1 = transformedVertices[lines[i][0]];
			linesCache[i].p2 = transformedVertices[lines[i][1]];
		}

		shouldUpdateCache = false;
	}

public:
	virtual ObjectType GetType() const {
		return MeshT;
	}
	size_t GetVerticesSize() {
		return sizeof(glm::vec3) * vertices.size();
	}

	virtual void Connect(size_t p1, size_t p2) {
		lines.push_back({ p1, p2 });
		linesCache.push_back(Pair{ vertices[p1], vertices[p2] });
		shouldUpdateCache = true;
	}
	virtual void Disconnect(size_t p1, size_t p2) {
		auto pos = find(lines, std::array<size_t, 2>{ p1, p2 });

		if (pos == -1)
			return;

		lines.erase(lines.begin() + pos);
		linesCache.erase(linesCache.begin() + pos);
		shouldUpdateCache = true;
	}

	const std::vector<std::array<size_t, 2>>& GetLinearConnections() {
		return lines;
	}

	virtual const std::vector<Pair>& GetLines() {
		if (shouldUpdateCache)
			UpdateCache();

		return linesCache;
	}
	virtual const std::vector<glm::vec3>& GetVertices() const {
		return vertices;
	}
	virtual void AddVertice(const glm::vec3& v) {
		vertices.push_back(v);
		shouldUpdateCache = true;
	}
	virtual void AddVertices(const std::vector<glm::vec3>& vs) {
		for (auto v : vs)
			AddVertice(v);
	}
	virtual void SetVertice(size_t index, const glm::vec3& v) {
		vertices[index] = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeX(size_t index, const float& v) {
		vertices[index].x = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeY(size_t index, const float& v) {
		vertices[index].y = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeZ(size_t index, const float& v) {
		vertices[index].z = v;
		shouldUpdateCache = true;
	}
	virtual void SetVertices(const std::vector<glm::vec3>& vs) {
		vertices = vs;
		shouldUpdateCache = true;
	}
	virtual void SetConnections(const std::vector<std::array<size_t, 2>>& connections) {
		lines = connections;
		shouldUpdateCache = true;
	}
	virtual void RemoveVertice() {
		vertices.pop_back();
		shouldUpdateCache = true;
	}
};

class WhiteSquare
{
public:

	float leftTop[9] = {
		-1, -1, 0,
		 1, -1, 0,
		-1,  1, 0,
	};

	float rightBottom[9] = {
		 1, -1, 0,
		 1,  1, 0,
		-1,  1, 0,
	};

	static const uint_fast8_t VerticesSize = sizeof(leftTop);

	GLuint VBOLeftTop, VAOLeftTop;
	GLuint VBORightBottom, VAORightBottom;
	GLuint ShaderProgramLeftTop, ShaderProgramRightBottom;


	bool Init()
	{

		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/WhiteSquare.frag");

		ShaderProgramLeftTop = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
		ShaderProgramRightBottom = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &VAOLeftTop);
		glGenBuffers(1, &VBOLeftTop);
		glGenVertexArrays(1, &VAORightBottom);
		glGenBuffers(1, &VBORightBottom);

		return true;
	}
};

class Cross : public LeafObject {
	bool isCreated = false;
	std::vector<Pair> linesCache;

	void UpdateCache() {
		std::vector<glm::vec3> vertices(6);

		vertices[0].x -= size;
		vertices[1].x += size;

		vertices[2].y -= size;
		vertices[3].y += size;

		vertices[4].z -= size;
		vertices[5].z += size;

		CascadeTransform(vertices);

		linesCache = std::vector<Pair>(3);
		for (size_t i = 0; i < 3; i++) {
			linesCache[i].p1 = vertices[i * 2];
			linesCache[i].p2 = vertices[i * 2 + 1];
		}
	}
public:
	float size = 0.1;
	std::function<void()> keyboardBindingHandler;
	size_t keyboardBindingHandlerId;

	bool Init() {
		UpdateCache();
		return true;
	}
	virtual const std::vector<Pair>& GetLines() {
		if (shouldUpdateCache)
			UpdateCache();

		return linesCache;
	}
	virtual ObjectType GetType() const {
		return CrossT;
	}

};



class StereoCamera : public LeafObject
{
	glm::vec3 GetPos() {
		return positionModifier + GetLocalPosition();
	}

public:
	glm::vec2* viewSize = nullptr;
	glm::vec3 positionModifier = glm::vec3(0, 3, -10);

	float eyeToCenterDistance = 0.5;

	StereoCamera() {
		Name = "camera";
	}

	// Preserve aspect ratio
	// From [0;1] to ([0;viewSize->x];[0;viewSize->y])
	glm::vec3 PreserveAspectRatio(glm::vec3 pos) {
		return glm::vec3(
			pos.x * viewSize->y / viewSize->x,
			pos.y,
			pos.z
		);
	}

	glm::vec3 GetLeft(const glm::vec3& pos) {
		auto cameraPos = GetPos();
		float denominator = cameraPos.z - pos.z;
		return glm::vec3(
			(pos.x * cameraPos.z - pos.z * (cameraPos.x - eyeToCenterDistance)) / denominator,
			(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
			0
		);
	}
	glm::vec3 GetRight(const glm::vec3& pos) {
		auto cameraPos = GetPos();
		float denominator = cameraPos.z - pos.z;
		return glm::vec3(
			(pos.x * cameraPos.z - pos.z * (cameraPos.x + eyeToCenterDistance)) / denominator,
			(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
			0
		);
	}

	Pair GetLeft(const Pair& stereoLine)
	{
		Pair line;

		line.p1 = PreserveAspectRatio(GetLeft(stereoLine.p1));
		line.p2 = PreserveAspectRatio(GetLeft(stereoLine.p2));

		return line;
	}
	Pair GetRight(const Pair& stereoLine)
	{
		Pair line;

		line.p1 = PreserveAspectRatio(GetRight(stereoLine.p1));
		line.p2 = PreserveAspectRatio(GetRight(stereoLine.p2));

		return line;
	}

	virtual ObjectType GetType() const {
		return CameraT;
	}

};

#pragma endregion



class SceneObjectBuffer {
public:
	using Buffer = std::set<SceneObject*>*;
private:
	static const ImGuiPayload* AcceptDragDropPayload(const char* name, ImGuiDragDropFlags flags) {
		return ImGui::AcceptDragDropPayload(name, flags);
	}
	static Buffer GetBuffer(void* data) {
		return *(Buffer*)data;
	}
public:
	static Buffer GetDragDropPayload(const char* name, ImGuiDragDropFlags flags) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(name, flags))
			return GetBuffer(payload->Data);

		return nullptr;
	}
	static bool PopDragDropPayload(const char* name, ImGuiDragDropFlags flags, std::vector<SceneObject*>* outSceneObjects) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(name, flags)) {
			auto objectPointers = GetBuffer(payload->Data);

			for (auto objectPointer : *objectPointers)
				outSceneObjects->push_back(objectPointer);

			objectPointers->clear();

			return true;
		}

		return false;
	}

	static void EmplaceDragDropSceneObject(const char* name, SceneObject* objectPointer, Buffer* buffer) {
		(*buffer)->emplace(objectPointer);

		ImGui::SetDragDropPayload("SceneObjects", buffer, sizeof(Buffer));
	}
};

class Scene {
public:
	
	template<InsertPosition p>
	static bool is(InsertPosition pos) {
		return p == pos;
	}
	template<InsertPosition p>
	static bool has(InsertPosition pos) {
		return (p & pos) != 0;
	}
private:
	GroupObject defaultObject;
public:
	// Stores all objects.
	std::vector<SceneObject*> objects;

	// Scene selected object buffer.
	std::set<SceneObject*> selectedObjects;
	SceneObject* root = &defaultObject;
	StereoCamera* camera;
	Cross* cross;

	float whiteZ = 0;
	float whiteZPrecision = 0.1;
	GLFWwindow* glWindow;

	const Event<> deleteAll;

	Scene() {
		defaultObject.Name = "Root";
	}

	bool Insert(SceneObject* destination, SceneObject* obj) {
		obj->SetParent(destination);
		objects.push_back(obj);
		return true;
	}

	bool Insert(SceneObject* obj) {
		obj->SetParent(root);
		objects.push_back(obj);
		return true;
	}

	bool Delete(SceneObject* source, SceneObject* obj) {
		for (size_t i = 0; i < source->children.size(); i++)
			if (source->children[i] == obj)
			{
				for (size_t j = 0; i < objects.size(); j++)
					if (objects[j] == obj) {
						source->children.erase(source->children.begin() + i);
						objects.erase(objects.begin() + j);
						delete obj;
						return true;
					}
			}

		std::cout << "The object for deletion was not found" << std::endl;
		return false;
	}

	

	static bool MoveTo(SceneObject* destination, int destinationPos, std::set<SceneObject*>* items, InsertPosition pos) {
		// Move single object
		if (items->size() > 1)
		{
			std::cout << "Moving of multiple objects is not implemented" << std::endl;
			return false;
		}

		// Find if item is present in target;

		auto item = items->begin()._Ptr->_Myval;
		item->SetParent(destination, destinationPos, pos);

		return true;
	}

	void DeleteAll() {
		for (auto o : objects)
			delete o;

		objects.clear();
		root = &defaultObject;
		root->children.clear();

		cross->SetParent(nullptr);

		const_cast<Event<>*>(&deleteAll)->Invoke();
	}

	~Scene() {
		for (auto o : objects)
			delete o;
	}
};
