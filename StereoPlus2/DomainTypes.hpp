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

struct Pair {
	glm::vec3 p1, p2;
};

class SceneObject {
	glm::vec3 position;
protected:
	bool shouldUpdateCache;
public:
	SceneObject* parent;
	std::vector<SceneObject*> children;

	std::string Name = "noname";

	virtual ObjectType GetType() const = 0;
	virtual std::string GetDefaultName() {
		return "SceneObject";
	}

	const glm::vec3& GetLocalPosition() const {
		return position;
	}
	const glm::vec3& GetWorldPosition() const {
		if (parent)
			return GetLocalPosition() + parent->GetLocalPosition();
		
		return GetLocalPosition();
	}

	void SetLocalPosition(glm::vec3 v) {
		shouldUpdateCache = true;
		position = v;
	}
	void SetWorldPosition(glm::vec3 v) {
		shouldUpdateCache = true;

		if (parent) {
			position += v - GetWorldPosition();
			return;
		}

		position = v;
	}

	void ForceUpdateCache() {
		shouldUpdateCache = true;
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

		linesCache = std::vector<Pair>(vertices.size() - 1);

		auto worldPos = GetWorldPosition();

		for (size_t i = 0; i < vertices.size() - 1; i++) {
			linesCache[i].p1 = vertices[i] + worldPos;
			linesCache[i].p2 = vertices[i + 1] + worldPos;
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

		linesCache = std::vector<Pair>(lines.size());

		auto worldPos = GetWorldPosition();

		for (size_t i = 0; i < lines.size(); i++) {
			linesCache[i].p1 = vertices[lines[i][0]] + worldPos;
			linesCache[i].p2 = vertices[lines[i][1]] + worldPos;
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

	bool CreateLines() {
		linesCache = std::vector<Pair>(3, Pair{ GetLocalPosition() , GetLocalPosition() });
		
		linesCache[0].p1.x -= size;
		linesCache[0].p2.x += size;

		linesCache[1].p1.y -= size;
		linesCache[1].p2.y += size;

		linesCache[2].p1.z -= size;
		linesCache[2].p2.z += size;

		return true;
	}
public:
	const uint_fast8_t lineCount = 3;

	float size = 0.1;

	bool Refresh() {
		if (!isCreated) {
			isCreated = true;
			return CreateLines();
		}
		
		return CreateLines();
	}
	bool Init() {
		return CreateLines();
	}
	virtual const std::vector<Pair>& GetLines() {
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

enum InsertPosition
{
	Top = 0x01,
	Bottom = 0x10,
	Center = 0x100,
	Any = Top | Bottom | Center,
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

	Scene() {
		defaultObject.Name = "Root";
	}

	bool Insert(SceneObject* destination, SceneObject* obj) {
		destination->children.push_back(obj);
		obj->parent = destination;
		objects.push_back(obj);
		return true;
	}

	bool Insert(SceneObject* obj) {
		root->children.push_back(obj);
		obj->parent = root;
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

		auto source = &item->parent->children;
		auto dest = &destination->children;

		auto sourcePosition = std::find(source->begin(), source->end(), item);

		item->parent = destination;

		if (dest->size() == 0)
		{
			dest->push_back(item);
			source->erase(std::find(source->begin(), source->end(), item));
			return true;
		}

		if (has<InsertPosition::Bottom>(pos))
		{
			destinationPos++;
		}

		auto sourcePositionInt = find(*source, item);
		if (source == dest && destinationPos < (int)sourcePositionInt)
		{
			dest->erase(sourcePosition);
			dest->insert(dest->begin() + destinationPos, (const size_t)1, item);
			return true;
		}

		dest->insert(dest->begin() + destinationPos, (const size_t)1, item);
		source->erase(sourcePosition);

		return true;
	}


	~Scene() {
		for (auto o : objects)
			delete o;
	}
};
