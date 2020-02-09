#pragma once
#include "GLLoader.hpp"

#include <set>


enum ObjectType {
	Group,
	Leaf,

	StereoLineT,
	StereoPolyLineT,
	FreeMeshT,
};

class SceneObject {
public:
	std::string Name = "noname";
	virtual ObjectType GetType() = 0;
};

struct ObjectPointer {
	std::vector<SceneObject*>* source;
	int pos;
};

struct ObjectPointerComparator {
	bool operator() (const ObjectPointer& lhs, const ObjectPointer& rhs) const {
		return lhs.pos < rhs.pos || lhs.source < rhs.source;
	}
};

class GroupObject : public SceneObject {
public:
	std::vector<SceneObject*> Children;
	virtual ObjectType GetType() {
		return Group;
	}
};

class LeafObject : public SceneObject {
public:
	virtual ObjectType GetType() {
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

	virtual ObjectType GetType() {
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

	virtual ObjectType GetType() {
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
	std::vector<glm::vec3> vertices;
	//std::vector<std::array<size_t, 2>> lines;

	size_t GetVerticesSize() {
		return sizeof(glm::vec3) * vertices.size();
	}
};

struct FreeMesh : Mesh{
	virtual ObjectType GetType() {
		return FreeMeshT;
	}

	std::vector<std::array<size_t, 2>> lines;
};

struct QuadMesh : Mesh {
	std::vector<std::array<size_t, 4>> quads;
};

struct TriangleMesh : Mesh {
	std::vector<std::array<size_t, 3>> triangles;
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
		lines[0].Start = Position;
		lines[0].End = Position;
		lines[0].Start.x -= size;
		lines[0].End.x += size;

		lines[1].Start = Position;
		lines[1].End = Position;
		lines[1].Start.y -= size;
		lines[1].End.y += size;

		lines[2].Start = Position;
		lines[2].End = Position;
		lines[2].Start.z -= size;
		lines[2].End.z += size;

		return true;
	}


	bool RefreshLines()
	{
		return CreateLines();
	}

public:
	glm::vec3 Position = glm::vec3(0);

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
public:
	glm::vec2* viewSize = nullptr;
	glm::vec2 viewCenter = glm::vec2(0, 0);
	glm::vec3 transformVec = glm::vec3(0, 0, 0);

	glm::vec3 position = glm::vec3(0, 3, -10);

	float eyeToCenterDistance = 0.5;

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
		float denominator = position.z - pos.z - transformVec.z;
		return glm::vec3(
			(pos.x * position.z - (pos.z + transformVec.z) * (position.x - eyeToCenterDistance) + position.z * transformVec.x) / denominator,
			(position.z * (transformVec.y - pos.y) + position.y * (pos.z + transformVec.z)) / denominator,
			0
		);
	}

	glm::vec3 GetRight(glm::vec3 pos) {
		float denominator = position.z - pos.z - transformVec.z;
		return glm::vec3(
			(pos.x * position.z - (pos.z + transformVec.z) * (position.x + eyeToCenterDistance) + position.z * transformVec.x) / denominator,
			(position.z * (transformVec.y - pos.y) + position.y * (pos.z + transformVec.z)) / denominator,
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

//#pragma region Move
//
//	void Move(glm::vec3 value)
//	{
//		//position += value;
//		viewCenter.x += value.x;
//		viewCenter.y += value.y;
//
//		transformVec += value;
//	}
//
//	void MoveLeft(float value) {
//		Move(left * value);
//	}
//
//	void MoveRight(float value) {
//		Move(-left * value);
//	}
//
//	void MoveUp(float value) {
//		Move(up * value);
//	}
//
//	void MoveDown(float value) {
//		Move(-up * value);
//	}
//
//	void MoveForward(float value) {
//		Move(forward * value);
//	}
//
//	void MoveBack(float value) {
//		Move(-forward * value);
//	}
//
//#pragma endregion
};




class Scene {
public:
	// Stores all objects.
	std::vector<SceneObject*> objects;

	// Scene selected object buffer.
	std::set<ObjectPointer, ObjectPointerComparator> selectedObjects;
	GroupObject* root;
	StereoCamera* camera;
	Cross* cross;

	float whiteZ = 0;
	float whiteZPrecision = 0.1;
	GLFWwindow* window;

	bool Insert(std::vector<SceneObject*>* source, SceneObject* obj) {
		objects.push_back(obj);
		source->push_back(obj);
		return true;
	}

	bool Delete(std::vector<SceneObject*>* source, SceneObject* obj) {
		for (size_t i = 0; i < source->size(); i++)
			if ((*source)[i] == obj)
			{
				for (size_t j = 0; i < objects.size(); j++)
					if (objects[j] == obj) {
						source->erase(source->begin() + i);
						objects.erase(objects.begin() + j);
						delete obj;
						return true;
					}
			}

		std::cout << "The object for deletion was not found" << std::endl;
		return false;
	}

	~Scene() {
		for (auto o : objects)
			delete o;
	}
};


//class ClipBoard {
//
//}

//class SceneObjectClipBoard {
//	static std::set<ObjectPointer, ObjectPointerComparator>* GetBuffer(void* data) {
//		return *(std::set<ObjectPointer, ObjectPointerComparator> * *)data;
//	}
//
//	static char* defaultName = "SceneObjects";
//public:
//
//	static std::set<ObjectPointer, ObjectPointerComparator>* Pull(const char* name, ImGuiDragDropFlags target_flags) {
//		if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload(name, target_flags))
//		{
//			return GetBuffer(payload->Data);
//		}
//
//		return nullptr;
//	}
//
//	static std::set<ObjectPointer, ObjectPointerComparator>* Pull(ImGuiDragDropFlags target_flags) {
//		return Pull(defaultName, target_flags);
//	}
//
//	static void Pop(const char* name, ImGuiDragDropFlags target_flags) {
//		Pull(name, target_flags)->clear();
//	}
//
//	static void Pop(std::set<ObjectPointer, ObjectPointerComparator>* buffer) {
//		buffer->clear();
//	}
//
//	static void Push(const char* name, std::set<ObjectPointer, ObjectPointerComparator>* buffer) {
//		ImGui::SetDragDropPayload(name, &buffer, sizeof(std::set<ObjectPointer, ObjectPointerComparator>*));
//	}
//};
