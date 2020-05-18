#pragma once
#include "GLLoader.hpp"
#include <stdlib.h>
#include <set>
#include <array>



#pragma region Scene Objects

enum ObjectType {
	Group,
	Leaf,

	StereoLineT,
	StereoPolyLineT,
	LineMeshT,
	MeshT,
	QuadMeshT,
};

class SceneObject {
public:
	SceneObject* parent;
	std::vector<SceneObject*> children;

	glm::vec3 position;
	std::string Name = "noname";
	virtual ObjectType GetType() const = 0;
	virtual std::string GetDefaultName() {
		return "SceneObject";
	}
};



class GroupObject : public SceneObject {
public:
	virtual ObjectType GetType() const {
		return Group;
	}
};

class LeafObject : public SceneObject {
public:
	virtual ObjectType GetType() const {
		return Leaf;
	}
};

struct Line
{
	glm::vec3 Start, End;

	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 2;

	GLuint VBO, VAO;
	GLuint ShaderProgram;
};

struct StereoLine : LeafObject
{
	glm::vec3 Start, End;

	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 2;

	virtual ObjectType GetType() const {
		return StereoLineT;
	}
};

struct StereoPolyLine : LeafObject {
	std::vector<glm::vec3> Points;

	StereoPolyLine() {}

	StereoPolyLine(StereoPolyLine& copy) {
		for (auto p : copy.Points)
			Points.push_back(p);
	}

	virtual ObjectType GetType() const {
		return StereoPolyLineT;
	}
};


struct Triangle
{
	glm::vec3 p1, p2, p3;

	static const uint_fast8_t VerticesSize = sizeof(glm::vec3) * 3;

	GLuint VBO, VAO;
	GLuint ShaderProgram;
};

struct Mesh : LeafObject {
protected:
	std::vector<glm::vec3> vertices;
public:
	virtual ObjectType GetType() const {
		return MeshT;
	}
	size_t GetVerticesSize() {
		return sizeof(glm::vec3) * vertices.size();
	}


	std::vector<glm::vec3>* GetVertices() {
		return &vertices;
	}

	virtual void AddVertice(glm::vec3 v) {
		vertices.push_back(v);
	}
	virtual void RemoveVertice(size_t i) {
		vertices.erase(vertices.begin() + i);
	}
	virtual void ReplaceVertice(size_t i, glm::vec3 v) {
		vertices[i] = v;
	}

	virtual void Connect(size_t p1, size_t p2) = 0;
	virtual void Disconnect(size_t p1, size_t p2) = 0;
};

struct LineMesh : Mesh{
	virtual ObjectType GetType() const {
		return LineMeshT;
	}

	std::vector<std::array<size_t, 2>> lines;

	virtual void Connect(size_t p1, size_t p2) {
		lines.push_back({ p1, p2 });
	}
	virtual void Disconnect(size_t p1, size_t p2) {
		auto pos = find(lines.begin(), lines.end(), std::array<size_t, 2>{ p1, p2 });
		
		if (pos == lines.end())
			return;

		lines.erase(pos);
	}

	const std::vector<std::array<size_t, 2>>& GetLinearConnections() {
		return lines;
	}
};

struct TriangleMesh : LineMesh {
	std::vector<std::array<size_t, 3>> triangles;
};

struct QuadMesh : TriangleMesh {
	virtual ObjectType GetType() const {
		return QuadMeshT;
	}
	std::vector<std::array<size_t, 4>> quads;
};




// Created for the sole purpose of crutching the broken 
// Line - triangle drawing mechanism.
// When you draw line/triangle and then change to other type then 
// First triangle and First line with the last frame shader's vertices get mixed.
// Dunno how it happens nor how to fix it. 
// Will return to it after some more immediate tasks are done.
struct ZeroLine
{
	Line line;

	bool Init()
	{
		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/zero.frag");

		line.ShaderProgram = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &line.VAO);
		glGenBuffers(1, &line.VBO);

		line.Start = line.End = glm::vec3(-1000);

		return true;
	}
};
struct ZeroTriangle
{
	Triangle triangle;

	bool Init()
	{
		for (size_t i = 0; i < 9; i++)
		{
			((float*)&triangle)[i] = -1000;
		}

		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/zero.frag");

		triangle.ShaderProgram = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &triangle.VAO);
		glGenBuffers(1, &triangle.VBO);

		return true;
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

class Cross : public LeafObject
{
	bool isCreated = false;

	bool CreateLines()
	{
		lines[0].Start = position;
		lines[0].End = position;
		lines[0].Start.x -= size;
		lines[0].End.x += size;

		lines[1].Start = position;
		lines[1].End = position;
		lines[1].Start.y -= size;
		lines[1].End.y += size;

		lines[2].Start = position;
		lines[2].End = position;
		lines[2].Start.z -= size;
		lines[2].End.z += size;

		return true;
	}


	bool RefreshLines()
	{
		return CreateLines();
	}

public:
	StereoLine* lines;
	const uint_fast8_t lineCount = 3;

	float size = 0.1;

	bool Refresh()
	{
		if (!isCreated)
		{
			isCreated = true;
			return CreateLines();
		}

		return RefreshLines();
	}


	bool Init()
	{
		lines = new StereoLine[lineCount];

		return CreateLines();
	}

	~Cross() {
		delete[] lines;
	}
};



class StereoCamera : public LeafObject
{
	glm::vec3 GetPos() {
		return positionModifier + position;
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

	glm::vec3 GetLeft(glm::vec3 pos) {
		auto cameraPos = GetPos();
		float denominator = cameraPos.z - pos.z;
		return glm::vec3(
			(pos.x * cameraPos.z - pos.z * (cameraPos.x - eyeToCenterDistance)) / denominator,
			(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
			0
		);
	}

	glm::vec3 GetRight(glm::vec3 pos) {
		auto cameraPos = GetPos();
		float denominator = cameraPos.z - pos.z;
		return glm::vec3(
			(pos.x * cameraPos.z - pos.z * (cameraPos.x + eyeToCenterDistance)) / denominator,
			(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
			0
		);
	}

	Line GetLeft(StereoLine* stereoLine)
	{
		Line line;

		line.Start = PreserveAspectRatio(GetLeft(stereoLine->Start));
		line.End = PreserveAspectRatio(GetLeft(stereoLine->End));

		return line;
	}

	Line GetRight(StereoLine* stereoLine)
	{
		Line line;

		line.Start = PreserveAspectRatio(GetRight(stereoLine->Start));
		line.End = PreserveAspectRatio(GetRight(stereoLine->End));

		return line;
	}
};

#pragma endregion


//struct ObjectPointer {
//	std::vector<SceneObject*>* source;
//	int pos;
//};

//struct ObjectPointerComparator {
//	bool operator() (const ObjectPointer& lhs, const ObjectPointer& rhs) const {
//		return lhs.pos < rhs.pos || lhs.source < rhs.source;
//	}
//};

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

	template<typename T>
	static int find(const std::vector<T>& source, T item) {
		for (size_t i = 0; i < source.size(); i++)
			if (source[i] == item)
				return i;

		return -1;
	}

public:
	

	// Stores all objects.
	std::vector<SceneObject*> objects;

	// Scene selected object buffer.
	std::set<SceneObject*> selectedObjects;
	GroupObject* root = &defaultObject;
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

		auto sourcePositionInt = Scene::find(*source, item);
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
