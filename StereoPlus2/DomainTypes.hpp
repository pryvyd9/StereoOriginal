#pragma once
#include "GLLoader.hpp"
#include "Settings.hpp"
#include <stdlib.h>
#include <set>
#include <array>
#include "ImGuiExtensions.hpp"
#include "DomainUtils.hpp"
#include <glm/gtx/vector_angle.hpp>
#include <regex>
#include "Math.hpp"

class GroupObject : public SceneObject {
public:
	virtual ObjectType GetType() const override {
		return Group;
	}

	GroupObject() {}
	GroupObject(const GroupObject* copy) : SceneObject(copy) {}
	GroupObject& operator=(const GroupObject& o) {
		SceneObject::operator=(o);
		return *this;
	}
	virtual SceneObject* Clone() const override {
		return new GroupObject(this);
	}

};

class LeafObject : public SceneObject {
public:
	LeafObject() {}
	LeafObject(const LeafObject* copy) : SceneObject(copy){}
	LeafObject& operator=(const LeafObject& o) {
		SceneObject::operator=(o);
		return *this;
	}

};

class PolyLine : public LeafObject {
	std::vector<glm::vec3> vertices;

	std::vector<glm::vec3> verticesCache;

	std::vector<glm::vec3> leftBuffer;
	std::vector<glm::vec3> rightBuffer;

	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight) override {
		UpdateCache();

		leftBuffer = std::vector<glm::vec3>(verticesCache.size());
		rightBuffer = std::vector<glm::vec3>(verticesCache.size());
		for (size_t i = 0; i < verticesCache.size(); i++) {
			leftBuffer[i] = toLeft(verticesCache[i]);
			rightBuffer[i] = toRight(verticesCache[i]);
		}
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * verticesCache.size(), leftBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * verticesCache.size(), rightBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	void UpdateCache() {
		verticesCache = vertices;
		CascadeTransform(verticesCache);
	}

public:

	PolyLine() {}
	PolyLine(const PolyLine* copy) : LeafObject(copy){
		vertices = copy->vertices;
	}

	virtual ObjectType GetType() const override {
		return PolyLineT;
	}
	virtual const std::vector<glm::vec3>& GetVertices() const override {
		return vertices;
	}

	virtual void AddVertice(const glm::vec3& v) override {
		HandleBeforeUpdate();
		vertices.push_back(v);
		shouldUpdateCache = true;
	}
	virtual void AddVertices(const std::vector<glm::vec3>& vs) override {
		for (auto v : vs)
			AddVertice(v);
	}
	virtual void SetVertice(size_t index, const glm::vec3& v) override {
		HandleBeforeUpdate();
		vertices[index] = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeX(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].x = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeY(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].y = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeZ(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].z = v;
		shouldUpdateCache = true;
	}
	virtual void SetVertices(const std::vector<glm::vec3>& vs) override {
		HandleBeforeUpdate();
		vertices.clear();
		for (auto v : vs)
			AddVertice(v);
		shouldUpdateCache = true;
	}

	virtual void RemoveVertice() override {
		HandleBeforeUpdate();
		if (vertices.size() > 0)
			vertices.pop_back();
		shouldUpdateCache = true;
	}

	virtual void DesignProperties() override {
		if (ImGui::TreeNodeEx("polyline", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			std::stringstream ss;
			ss << (vertices.size() > 0 ? vertices.size() - 1 : 0);

			ImGui::LabelText("line count", ss.str().c_str());

			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}
		SceneObject::DesignProperties();
	}

	virtual void Reset() override {
		vertices.clear();
		SceneObject::Reset();
	}

	virtual void DrawLeft(GLuint shader) override {
		if (verticesCache.size() < 2)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_LINE_STRIP, 0, verticesCache.size());
	}
	virtual void DrawRight(GLuint shader) override {
		if (verticesCache.size() < 2)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_LINE_STRIP, 0, verticesCache.size());
	}

	SceneObject* Clone() const override {
		return new PolyLine(this);
	}
	PolyLine& operator=(const PolyLine& o) {
		vertices = o.vertices;
		LeafObject::operator=(o);
		return *this;
	}
};

class SineCurve : public LeafObject {
	const Log Logger = Log::For<SineCurve>();

	std::vector<glm::vec3> vertices;
	bool isPositionCreated = false;

	std::vector<glm::vec3> verticesCache;

	std::vector<glm::vec3> leftBuffer;
	std::vector<glm::vec3> rightBuffer;

	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight) override {
		UpdateCache();

		leftBuffer = std::vector<glm::vec3>(verticesCache.size());
		rightBuffer = std::vector<glm::vec3>(verticesCache.size());
		for (size_t i = 0; i < verticesCache.size(); i++) {
			leftBuffer[i] = toLeft(verticesCache[i]);
			rightBuffer[i] = toRight(verticesCache[i]);
		}
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * verticesCache.size(), leftBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * verticesCache.size(), rightBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void updateCacheAsPolyLine() {
		verticesCache = vertices;
		CascadeTransform(verticesCache);
	}

	void updateCacheAsPolyLine(int from, int to) {
		verticesCache.insert(verticesCache.end(), vertices.begin() + from, vertices.begin() + to);
	}

	/// <summary>
	///    B
	///  / |  \
	/// A--D---C
	/// </summary>
void UpdateCache() {
	if (vertices.size() == 0)
		return;

	verticesCache = std::vector<glm::vec3>();

	if (!isPositionCreated) {
		isPositionCreated = true;

		// Requests a redundant cache update.
		SetWorldPosition(vertices[0]);
	}

	if (vertices.size() < 3) {
		updateCacheAsPolyLine(0, vertices.size());
		CascadeTransform(verticesCache);
		return;
	}

	for (size_t i = 0; i < vertices.size(); i += 2)
	{
		if (i + 2 >= vertices.size())
		{
			updateCacheAsPolyLine(i + 1, vertices.size());
			break;
		}
		
		auto points = Build::Sine(&vertices[i]);
		verticesCache.insert(verticesCache.end(), points.begin(), points.end());
	}

	CascadeTransform(verticesCache);
}

public:

	SineCurve() {
	}
	SineCurve(const SineCurve* copy) : LeafObject(copy) {
		vertices = copy->vertices;
	}

	virtual ObjectType GetType() const override {
		return SineCurveT;
	}
	virtual const std::vector<glm::vec3>& GetVertices() const override {
		return vertices;
	}

	virtual void AddVertice(const glm::vec3& v) override {
		HandleBeforeUpdate();
		vertices.push_back(v);
		shouldUpdateCache = true;
	}
	virtual void AddVertices(const std::vector<glm::vec3>& vs) override {
		for (auto v : vs)
			AddVertice(v);
	}
	virtual void SetVertice(size_t index, const glm::vec3& v) override {
		HandleBeforeUpdate();
		vertices[index] = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeX(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].x = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeY(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].y = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeZ(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].z = v;
		shouldUpdateCache = true;
	}
	virtual void SetVertices(const std::vector<glm::vec3>& vs) override {
		HandleBeforeUpdate();
		vertices.clear();
		for (auto v : vs)
			AddVertice(v);
		shouldUpdateCache = true;
	}

	virtual void RemoveVertice() override {
		HandleBeforeUpdate();
		if (vertices.size() > 0)
			vertices.pop_back();
		shouldUpdateCache = true;
	}

	virtual void DesignProperties() override {
		if (ImGui::TreeNodeEx("polyline", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			std::stringstream ss;
			ss << (vertices.size() > 0 ? vertices.size() - 1 : 0);

			ImGui::LabelText("line count", ss.str().c_str());

			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}
		SceneObject::DesignProperties();
	}

	virtual void Reset() override {
		vertices.clear();
		SceneObject::Reset();
	}

	virtual void DrawLeft(GLuint shader) override {
		if (verticesCache.size() < 2)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_LINE_STRIP, 0, verticesCache.size());
	}
	virtual void DrawRight(GLuint shader) override {
		if (verticesCache.size() < 2)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_LINE_STRIP, 0, verticesCache.size());
	}

	SceneObject* Clone() const override {
		return new SineCurve(this);
	}
	SineCurve& operator=(const SineCurve& o) {
		vertices = o.vertices;
		LeafObject::operator=(o);
		return *this;
	}
};

class Mesh : public LeafObject {
	std::vector<glm::vec3> vertices;
	std::vector<std::array<GLuint, 2>> connections;

	std::vector<glm::vec3> vertexCache;
	std::vector<glm::vec3> leftBuffer;
	std::vector<glm::vec3> rightBuffer;

	GLuint IBO;

	bool shouldUpdateIBO = true;

	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight) override {
		UpdateCache();

		leftBuffer = rightBuffer = std::vector<glm::vec3>(vertexCache.size());
		for (size_t i = 0; i < vertexCache.size(); i++) {
			leftBuffer[i] = toLeft(vertexCache[i]);
			rightBuffer[i] = toRight(vertexCache[i]);
		}

		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertexCache.size(), leftBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertexCache.size(), rightBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (shouldUpdateIBO) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(std::array<GLuint, 2>) * GetLinearConnections().size(), GetLinearConnections().data(), GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			shouldUpdateIBO = false;
		}
	}

	void UpdateCache() {
		vertexCache = vertices;
		CascadeTransform(vertexCache);
	}

	virtual void DrawLeft(GLuint shader) override {
		if (vertexCache.size() < 2)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawElements(GL_LINES, GetLinearConnections().size() * 2, GL_UNSIGNED_INT, nullptr);
	}
	virtual void DrawRight(GLuint shader) override {
		if (vertexCache.size() < 2)
			return;

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		// IBO is bind in DrawLeft already so we don't bind it here.
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawElements(GL_LINES, GetLinearConnections().size() * 2, GL_UNSIGNED_INT, nullptr);
	}

public:
	Mesh() {
		glGenBuffers(1, &IBO);
	}
	Mesh(const Mesh* copy) : LeafObject(copy) {
		glGenBuffers(1, &IBO);

		vertices = copy->vertices;
		connections = copy->connections;
	}

	~Mesh() {
		glDeleteBuffers(1, &IBO);
	}

	virtual ObjectType GetType() const override {
		return MeshT;
	}
	size_t GetVerticesSize() {
		return sizeof(glm::vec3) * vertices.size();
	}

	virtual void Connect(GLuint p1, GLuint p2) {
		HandleBeforeUpdate();
		connections.push_back({ p1, p2 });
		shouldUpdateCache = true;
		shouldUpdateIBO = true;
	}
	virtual void Disconnect(GLuint p1, GLuint p2) {
		auto pos = find(connections, std::array<GLuint, 2>{ p1, p2 });

		if (pos == -1)
			return;

		HandleBeforeUpdate();
		connections.erase(connections.begin() + pos);
		shouldUpdateCache = true;
		shouldUpdateIBO = true;
	}

	const std::vector<std::array<GLuint, 2>>& GetLinearConnections() {
		return connections;
	}

	virtual const std::vector<glm::vec3>& GetVertices() const override {
		return vertices;
	}
	virtual void AddVertice(const glm::vec3& v) override {
		HandleBeforeUpdate();
		vertices.push_back(v);
		shouldUpdateCache = true;
		shouldUpdateIBO = true;
	}
	virtual void AddVertices(const std::vector<glm::vec3>& vs) override {
		for (auto v : vs)
			AddVertice(v);
	}
	virtual void SetVertice(size_t index, const glm::vec3& v) override {
		HandleBeforeUpdate();
		vertices[index] = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeX(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].x = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeY(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].y = v;
		shouldUpdateCache = true;
	}
	virtual void SetVerticeZ(size_t index, const float& v) override {
		HandleBeforeUpdate();
		vertices[index].z = v;
		shouldUpdateCache = true;
	}
	virtual void SetVertices(const std::vector<glm::vec3>& vs) override {
		HandleBeforeUpdate();
		vertices = vs;
		shouldUpdateCache = true;
	}
	virtual void SetConnections(const std::vector<std::array<GLuint, 2>>& connections) {
		HandleBeforeUpdate();
		this->connections = connections;
		shouldUpdateCache = true;
		shouldUpdateIBO = true;
	}
	virtual void RemoveVertice() override {
		HandleBeforeUpdate();
		vertices.pop_back();
		shouldUpdateCache = true;
		shouldUpdateIBO = true;
	}

	virtual void DesignProperties() override {
		if (ImGui::TreeNodeEx("mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			std::stringstream ss;
			ss << connections.size();

			ImGui::LabelText("line count", ss.str().c_str());

			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}
		SceneObject::DesignProperties();
	}

	virtual void Reset() override {
		vertices.clear();
		connections.clear();
		shouldUpdateIBO = true;
		SceneObject::Reset();
	}


	SceneObject* Clone() const override {
		return new Mesh(this);
	}
	Mesh& operator=(const Mesh& o) {
		vertices = o.vertices;
		connections = o.connections;
		LeafObject::operator=(o);
		return *this;
	}

};

// Hexagon that keeps 2 pixel radius.
class Point : public LeafObject {
	float sizePixels = 2.f;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> leftBuffer;
	std::vector<glm::vec3> rightBuffer;

	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight) override {

		auto leftCenter = toLeft(GetWorldPosition());
		auto rightCenter = toRight(GetWorldPosition());
		auto millimeterSize = Convert::PixelsToMillimeters(sizePixels);

		leftBuffer = rightBuffer = std::vector<glm::vec3>(vertices.size());
		for (size_t i = 0; i < vertices.size(); i++) {
			auto p = Convert::MillimetersToViewCoordinates(vertices[i] * millimeterSize, ReadOnlyState::ViewSize().Get());
			leftBuffer[i] = leftCenter + p;
			rightBuffer[i] = rightCenter + p;
		}

		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * leftBuffer.size(), leftBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * rightBuffer.size(), rightBuffer.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	virtual void DrawLeft(GLuint shader) override {
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_TRIANGLE_FAN, 0, leftBuffer.size());
	}
	virtual void DrawRight(GLuint shader) override {
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_TRIANGLE_FAN, 0, leftBuffer.size());
	}

public:
	Point() {
		vertices = Build::Circle(6, 1);
	}
	Point(const Point* copy) : LeafObject(copy) {
		vertices = copy->vertices;
	}
	~Point() {}

	virtual ObjectType GetType() const override {
		return PointT;
	}

	SceneObject* Clone() const override {
		return new Point(this);
	}
};

class WhiteSquare
{
public:
	float vertices[18] = {
		-1, -1, 0,
		 1, -1, 0,
		-1,  1, 0,
		 1,  1, 0,
	};

	static const uint_fast8_t VerticesSize = sizeof(vertices);

	GLuint VBOLeftTop, VAOLeftTop;
	GLuint VBORightBottom, VAORightBottom;
	GLuint ShaderProgram;


	bool Init()
	{

		auto vertexShaderSource = GLLoader::ReadShader("shaders/.vert");
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/.frag");

		ShaderProgram = GLLoader::CreateShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

		glGenVertexArrays(1, &VAOLeftTop);
		glGenBuffers(1, &VBOLeftTop);
		glGenVertexArrays(1, &VAORightBottom);
		glGenBuffers(1, &VBORightBottom);

		return true;
	}
};

class Cross : public LeafObject {
	std::vector<glm::vec3> vertices;

	std::vector<glm::vec3> leftBuffer;
	std::vector<glm::vec3> rightBuffer;

	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight) override {
		UpdateCache();

		leftBuffer = rightBuffer = std::vector<glm::vec3>(vertices.size());
		for (size_t i = 0; i < vertices.size(); i++) {
			leftBuffer[i] = toLeft(vertices[i]);
			rightBuffer[i] = toRight(vertices[i]);
		}

		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), leftBuffer.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), rightBuffer.data(), GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	void UpdateCache() {
		vertices = std::vector<glm::vec3>(6);

		vertices[0].x -= size;
		vertices[1].x += size;

		vertices[2].y -= size;
		vertices[3].y += size;

		vertices[4].z -= size;
		vertices[5].z += size;

		CascadeTransform(vertices);
	}

	// Cross is being continuously modified so don't notify it's updates.
	virtual void HandleBeforeUpdate() override {}
public:
	float size = 10;
	std::function<void()> keyboardBindingHandler;
	size_t keyboardBindingHandlerId;

	std::function<void()> GUIPositionEditHandler;
	size_t GUIPositionEditHandlerId;
	glm::vec3 GUIPositionEditDifference;

	Cross() {
		shouldTransformPosition = true;
		shouldTransformRotation = true;
	}

	virtual ObjectType GetType() const override {
		return CrossT;
	}
	virtual void DesignProperties() override {
		if (ImGui::TreeNodeEx("cross", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			if (ImGui::DragFloat("size", &size, 1, 0, 0, "%.1f"))
				ForceUpdateCache();

			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}

		auto oldPos = GetLocalPosition();

		SceneObject::DesignProperties();

		// Hask to enable cross movement via GUI editing count as input movement.
		// It enables tools to react to it properly and work correctly.
		GUIPositionEditDifference = GetLocalPosition() - oldPos;

		if (GUIPositionEditDifference != glm::vec3())
			SetLocalPosition(oldPos);

		//shouldUpdateCache = true;
	}

	virtual void DrawLeft(GLuint shader) override {
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBOLeft);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_LINES, 0, vertices.size());
	}
	virtual void DrawRight(GLuint shader) override {
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBORight);
		glVertexAttribPointer(GL_POINTS, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(GL_POINTS);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Apply shader
		glUseProgram(shader);
		glDrawArrays(GL_LINES, 0, vertices.size());
	}
};

class Camera : public LeafObject
{
	// View coordinates
	float eyeToCenterDistance;
	glm::vec3 positionModifier;

	glm::vec3 GetPos() {
		return positionModifier + GetLocalPosition();
	}

	Event<> onPropertiesChanged;

	virtual void HandleBeforeUpdate() override {
		onPropertiesChanged.Invoke();
	}
public:
	// Pixels
	ReadonlyProperty<glm::vec2> ViewSize;
	// Millimeters
	float viewSizeZ = 100;
	Property<float> EyeToCenterDistance = 34;
	Property<glm::vec3> PositionModifier = glm::vec3(0, 50, 600);
	

	Camera() {
		Name = "camera";
		EyeToCenterDistance.OnChanged() += [&](const float& v) {
			eyeToCenterDistance = Convert::MillimetersToViewCoordinates(v, ViewSize->x); };
		PositionModifier.OnChanged() += [&](const glm::vec3& v) {
			positionModifier = Convert::MillimetersToViewCoordinates(v, ViewSize.Get(), viewSizeZ); };

		ViewSize.OnChanged() += [&](const glm::vec2 v) {
			// Trigger conversion.
			EyeToCenterDistance = EyeToCenterDistance.Get();
			PositionModifier = PositionModifier.Get();
		};
	}

	IEvent<>& OnPropertiesChanged() {
		return onPropertiesChanged;
	}

	glm::vec3 GetLeft(const glm::vec3& v) {
		return Stereo::GetLeft(v, GetPos(), eyeToCenterDistance, ViewSize.Get(), viewSizeZ);
	}
	glm::vec3 GetRight(const glm::vec3& v) {
		return Stereo::GetRight(v, GetPos(), eyeToCenterDistance, ViewSize.Get(), viewSizeZ);
	}


	virtual ObjectType GetType() const override {
		return CameraT;
	}
	virtual void DesignProperties() override {
		if (ImGui::TreeNodeEx("camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			ImGui::Extensions::PushActive(false);
			ImGui::DragFloat2("view size", (float*)&ViewSize.Get());
			auto viewSizeMillimeters = Convert::PixelsToMillimeters(ViewSize.Get());
			ImGui::DragFloat2("view size millimeters", (float*)(&viewSizeMillimeters), 1, 0, 0, "%.1f");
			ImGui::Extensions::PopActive();

			if (auto v = PositionModifier.Get();
				ImGui::DragFloat3("user position", (float*)&v, 1, 0, 0, "%.0f")) {
				PositionModifier = v;
				ForceUpdateCache();
			}

			if (auto v = EyeToCenterDistance.Get();
				ImGui::DragFloat("eye to center distance", &v, 1, 0, 0, "%.0f")) {
				EyeToCenterDistance = v;
				ForceUpdateCache();
			}

			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}
		SceneObject::DesignProperties();
	}
};

class TraceObject : public GroupObject {
	bool shouldIgnoreParent;
	virtual void HandleBeforeUpdate() override {
		GroupObject::HandleBeforeUpdate();
		shouldIgnoreParent = false;
	}

public:
	TraceObject() {}
	TraceObject(const TraceObject* copy) : GroupObject(copy) {}

	void IgnoreParentOnce() {
		shouldIgnoreParent = true;
	}
	virtual ObjectType GetType() const override {
		return TraceObjectT;
	}
	const virtual SceneObject* GetParent() const override {
		return shouldIgnoreParent
			? nullptr
			: GroupObject::GetParent();
	}
	virtual SceneObject* Clone() const override {
		return new TraceObject(this);
	}
};

class Scene {
	Log Logger = Log::For<Scene>();

	static Event<>& deleteAll() {
		static Event<> v;
		return v;
	}

	static const SceneObject* FindRoot(const SceneObject* o) {
		auto parent = o->GetParent();
		if (parent == nullptr)
			return o;

		return FindRoot(parent);
	}
	static SceneObject* FindNewParent(SceneObject* oldParent, std::set<SceneObject*>* items) {
		if (oldParent == nullptr) {
			Log::For<Scene>().Error("Object doesn't have a parent");
			return nullptr;
		}

		if (exists(*items, oldParent))
			return FindNewParent(const_cast<SceneObject*>(oldParent->GetParent()), items);

		return oldParent;
	}
	static SceneObject* FindConnectedParent(SceneObject* oldParent, std::list<SceneObject*>& disconnectedItemsToBeMoved) {
		if (oldParent == nullptr)
			return nullptr;

		if (exists(disconnectedItemsToBeMoved, oldParent))
			return oldParent;

		return FindConnectedParent(const_cast<SceneObject*>(oldParent->GetParent()), disconnectedItemsToBeMoved);
	}

	static PON CreateRoot() {
		auto r = new GroupObject();
		r->Name = "Root";
		return PON(r);
	}
public:
	// Stores all objects.
	StaticProperty(std::vector<PON>, Objects);
	StaticProperty(PON, root);
	StaticProperty(Cross*, cross);
	Camera* camera;

	GLFWwindow* glWindow;



	static IEvent<>& OnDeleteAll() {
		return deleteAll();
	}

	Scene() {
		root() = CreateRoot();
	}

	static bool Insert(SceneObject* destination, SceneObject* obj) {
		obj->SetParent(destination);
		Objects().Get().push_back(obj);
		return true;
	}
	static bool Insert(SceneObject* obj) {
		obj->SetParent(root().Get().Get());
		Objects().Get().push_back(obj);
		return true;
	}

	static bool Delete(SceneObject* source, SceneObject* obj) {
		if (!source) {
			Log::For<Scene>().Warning("You cannot delete root object");
			return true;
		}

		for (size_t i = 0; i < source->children.size(); i++)
			if (source->children[i] == obj)
				for (size_t j = 0; i < Objects()->size(); j++)
					if (Objects().Get()[j].Get() == obj) {
						source->children.erase(source->children.begin() + i);
						Objects().Get().erase(Objects()->begin() + j);
						return true;
					}

		Log::For<Scene>().Error("The object for deletion was not found");
		return false;
	}
	void DeleteSelected() {
		for (auto o : ObjectSelection::Selected()) {
			o->Reset();
			(new FuncCommand())->func = [this, o] {
				Delete(const_cast<SceneObject*>(o.Get()->GetParent()), o.Get());
			};
		}
	}
	void DeleteAll() {
		deleteAll().Invoke();
		cross()->SetParent(nullptr);

		Objects().Get().clear();
		root() = CreateRoot();
	}

	struct CategorizedObjects {
		std::list<SceneObject*> parentObjects;
		std::set<SceneObject*> childObjects;
		// Items that will not be modified and their new parents in case current parents are removed.
		// Item, NewParent
		std::list<std::pair<SceneObject*, SceneObject*>> orphanedObjects;
	};
	struct CategorizedPON {
		std::list<PON> parentObjects;
		std::set<PON> childObjects;
		// Items that will not be modified and their new parents in case current parents are removed.
		// Item, NewParent
		std::list<std::pair<PON, PON>> orphanedObjects;
	};

	static void CategorizeObjects(SceneObject* root, std::set<SceneObject*>* items, CategorizedObjects& categorizedObjects) {
		root->CallRecursive(root, std::function<SceneObject* (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* parent) {
			if (exists(*items, o)) {
				if (exists(categorizedObjects.childObjects, parent) || exists(categorizedObjects.parentObjects, parent))
					categorizedObjects.childObjects.emplace(o);
				else if (auto newParent = FindConnectedParent(parent, categorizedObjects.parentObjects))
					categorizedObjects.orphanedObjects.push_back({ o, newParent });
				else
					categorizedObjects.parentObjects.push_back(o);
			}
			else if (exists(*items, parent))
				categorizedObjects.orphanedObjects.push_back({ o, FindNewParent(parent, items) });

			return o;
			}));
	}
	static void CategorizeObjects(SceneObject* root, std::set<SceneObject*>* items, CategorizedPON& categorizedObjects) {
		CategorizedObjects co;
		CategorizeObjects(root, items, co);
		for (auto o : co.parentObjects)
			categorizedObjects.parentObjects.push_back(o);
		for (auto o : co.childObjects)
			categorizedObjects.childObjects.emplace(o);
		for (auto [o, np] : co.orphanedObjects)
			categorizedObjects.orphanedObjects.push_back({ o, np });
	}
	static void CategorizeObjects(SceneObject* root, std::vector<PON>& items, CategorizedPON& categorizedObjects) {
		std::set<SceneObject*> is;
		for (auto o : items)
			is.emplace(o.Get());

		CategorizeObjects(root, &is, categorizedObjects);
	}
	static void CategorizeObjects(SceneObject* root, const std::set<PON>& items, CategorizedPON& categorizedObjects) {
		std::set<SceneObject*> is;
		for (auto o : items)
			is.emplace(o.Get());

		CategorizeObjects(root, &is, categorizedObjects);
	}


	// Tree structure is preserved even if there is an unselected link.
	//-----------------------------------------------------------------
	// * - selected
	//
	//   *a      b  *a
	//    b   ->    *c
	//   *c
	static bool MoveTo(SceneObject* destination, int destinationPos, std::set<SceneObject*>* items, InsertPosition pos) {
		auto root = const_cast<SceneObject*>(FindRoot(destination));

		// parentObjects will be moved to destination
		// childObjects will be ignored during move but will be moved with their parents
		// orphanedObjects will not be moved with their parents.
		// New parents will be assigned.
		CategorizedObjects categorizedObjects;

		CategorizeObjects(root, items, categorizedObjects);

		if ((pos & InsertPosition::Bottom) != 0)
			for (auto o : categorizedObjects.parentObjects)
				o->SetParent(destination, destinationPos, pos);
		else
			std::for_each(categorizedObjects.parentObjects.rbegin(), categorizedObjects.parentObjects.rend(), [&](auto o) {
			o->SetParent(destination, destinationPos, pos); });

		for (auto pair : categorizedObjects.orphanedObjects)
			pair.first->SetParent(pair.second, true);

		return true;
	}
	static bool MoveTo(SceneObject* destination, int destinationPos, std::set<PON>* items, InsertPosition pos) {
		std::set<SceneObject*> nitems;
		for (auto o : *items)
			nitems.emplace(o.Get());

		return MoveTo(destination, destinationPos, &nitems, pos);
	}

	static int GetNextDuplicateIndex(const std::string& originalName) {
		std::string regex;
		{
			std::stringstream ss;
			ss << originalName << " " << "(\\d+)";
			regex = ss.str();
		}
		auto r = std::regex(regex);

		int max = 0;
		for (auto& o : Objects().Get()) {
			std::smatch m;
			if (!std::regex_search(o->Name, m, r))
				continue;

			std::stringstream ss;
			ss << m[1];
			int current;
			ss >> current;
			if (current > max)
				max = current;
		}

		return max + 1;
	}

	static void AssignUniqueName(SceneObject* o, const std::string& originalName) {
		std::stringstream ss;
		ss << originalName << " " << GetNextDuplicateIndex(originalName);
		o->Name = ss.str();
	}


	~Scene() {
		Objects().Get().clear();
	}
};