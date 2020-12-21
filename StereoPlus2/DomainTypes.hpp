#pragma once
#include "GLLoader.hpp"
#include "Settings.hpp"
#include <stdlib.h>
#include <set>
#include <array>
#include "ImGuiExtensions.hpp"

enum ObjectType {
	Group,
	StereoPolyLineT,
	MeshT,
	CameraT,
	CrossT,
	TraceObjectT,
	//TraceObjectNodeT,
};

enum InsertPosition {
	Top = 0x1,
	Bottom = 0x10,
	Center = 0x100,
	Any = Top | Bottom | Center,
};

// Abstract scene object.
// Parent to all scene objects.
class SceneObject {
	// Local position;
	glm::vec3 position;
	// Local rotation;
	glm::fquat rotation = unitQuat();
	SceneObject* parent = nullptr;
protected:
	bool shouldTransformPosition = false;
	bool shouldTransformRotation = false;
	GLuint VBOLeft, VBORight, VAO;
		
	static bool& isAnyObjectUpdated() {
		static bool v;
		return v;
	}
	static Event<>& onBeforeAnyElementChanged() {
		static Event<> v;
		return v;
	}

	// When true cache will be updated on reading.
	// Means the object was changed.
	bool shouldUpdateCache = true;
	const float propertyIndent = -20;

	virtual void HandleBeforeUpdate() {
		if (!isAnyObjectUpdated()) {
			isAnyObjectUpdated() = true;
			onBeforeAnyElementChanged().Invoke();

			(new FuncCommand())->func = [&] {
				isAnyObjectUpdated() = false;
			};
		}
	}
	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight) {}

	virtual void DrawLeft(GLuint shader) {}
	virtual void DrawRight(GLuint shader) {}

	// Adds or substracts transformations.

	virtual void CascadeTransform(std::vector<glm::vec3>& vertices) const {
		if (shouldTransformRotation && shouldTransformPosition)
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] = glm::rotate(GetLocalRotation(), vertices[i]) + GetLocalPosition();
		else if (shouldTransformRotation)
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] = glm::rotate(GetLocalRotation(), vertices[i]);
		else if (shouldTransformPosition)
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] += GetLocalPosition();

		if (GetParent())
			GetParent()->CascadeTransform(vertices);
	}
	virtual void CascadeTransform(glm::vec3& v) const {
		if (shouldTransformRotation)
			v += glm::rotate(GetLocalRotation(), v);
		if (shouldTransformPosition)
			v += GetLocalPosition();

		if (GetParent())
			GetParent()->CascadeTransform(v);
	}
	virtual void CascadeTransformInverse(glm::vec3& v) const {
		if (GetParent())
			GetParent()->CascadeTransformInverse(v);

		if (shouldTransformPosition)
			v -= GetLocalPosition();
		if (shouldTransformRotation)
			v += glm::rotate(glm::inverse(GetLocalRotation()), v);
	}

public:
	std::vector<SceneObject*> children;
	std::string Name = "noname";

	static IEvent<>& OnBeforeAnyElementChanged() {
		return onBeforeAnyElementChanged();
	}

	SceneObject() {
		glGenBuffers(2, &VBOLeft);
		glGenVertexArrays(1, &VAO);
	}
	SceneObject(const SceneObject* copy) : SceneObject() {
		position = copy->position;
		rotation = copy->rotation;
		parent = copy->parent;
		children = copy->children;
		Name = copy->Name;
	}
	~SceneObject() {
		glDeleteBuffers(2, &VBOLeft);
		glDeleteVertexArrays(1, &VAO);
	}

	virtual void Draw(
		std::function<glm::vec3(glm::vec3)> toLeft,
		std::function<glm::vec3(glm::vec3)> toRight,
		GLuint shaderLeft,
		GLuint shaderRight,
		GLuint stencilMaskLeft,
		GLuint stencilMaskRight) {
		if (shouldUpdateCache || Settings::ShouldDetectPosition().Get())
			UpdateOpenGLBuffer(toLeft, toRight);

		glStencilMask(stencilMaskLeft);
		glStencilFunc(GL_ALWAYS, stencilMaskLeft, stencilMaskLeft | stencilMaskRight);
		glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
		DrawLeft(shaderLeft);

		glStencilMask(stencilMaskRight);
		glStencilFunc(GL_ALWAYS, stencilMaskRight, stencilMaskLeft | stencilMaskRight);
		glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
		DrawRight(shaderRight);
	}


	constexpr const glm::fquat unitQuat() const {
		return glm::fquat(1, 0, 0, 0);
	}

	const virtual SceneObject* GetParent() const {
		return parent;
	}
	void SetParent(SceneObject* newParent, int newParentPos, InsertPosition pos) {
		ForceUpdateCache();
		auto source = &parent->children;
		auto dest = &newParent->children;

		if (Settings::MoveCoordinateAction().Get() == MoveCoordinateAction::Adapt) {
			auto oldPosition = GetWorldPosition();
			auto oldRotation = GetWorldRotation();

			parent = newParent;

			SetWorldPosition(oldPosition);
			SetWorldRotation(oldRotation);
		}
		else 
			parent = newParent;
		

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
	void SetParent(
		SceneObject* newParent,
		bool shouldConvertValues = false, 
		bool shouldIgnoreOldParent = false,
		bool shouldForceUpdateCache = true,
		bool shouldUpdateNewParent = true) {
		if (shouldForceUpdateCache)
			ForceUpdateCache();
		
		if (!shouldIgnoreOldParent && parent && parent->children.size() > 0) {
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


		if (shouldUpdateNewParent && newParent)
			newParent->children.push_back(this);
	}

	// Transforms position relative to the object.

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
	const virtual glm::vec3 GetWorldPosition() const {
		return shouldTransformPosition && GetParent()
			? ToWorldPosition(glm::vec3())
			: GetLocalPosition();
	}
	void SetLocalPosition(const glm::vec3& v) {
		ForceUpdateCache();
		position = v;
	}
	void SetWorldPosition(const glm::vec3& v) {
		ForceUpdateCache();

		position = GetParent()
			// Set world position means to set local position
			// relative to parent.
			? GetParent()->ToLocalPosition(v)
			: v;
	}

	const glm::quat& GetLocalRotation() const {
		return rotation;
	}
	const virtual glm::quat GetWorldRotation() const {
		return GetParent()
			? GetParent()->GetWorldRotation() * GetLocalRotation()
			: GetLocalRotation();
	}
	void SetLocalRotation(const glm::quat& v) {
		ForceUpdateCache();
		rotation = v;
	}
	void SetWorldRotation(const glm::quat& v) {
		ForceUpdateCache();

		rotation = GetParent()
			// Set world rotation means to set local rotation
			// relative to parent.
			? glm::inverse(GetParent()->GetWorldRotation()) * v
			: v;
	}

	// Forces the object and all children to update cache.
	void ForceUpdateCache() {
		HandleBeforeUpdate();

		shouldUpdateCache = true;
		for (auto c : children)
			c->ForceUpdateCache();
	}


	// Virtual methods to be overridden.
	// Do nothing here since we don't want cascade operations to fail 
	// just because the object doesn't implement some of it.

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

	virtual void DesignProperties() {

		if (ImGui::TreeNodeEx("local", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);
			
			if (auto v = GetLocalPosition(); ImGui::DragFloat3("local position", (float*)&v, 1, 0, 0, "%.1f"))
				SetLocalPosition(v);
			if (auto v = GetLocalRotation(); ImGui::DragFloat4("local rotation", (float*)&v, 0.01, 0, 1, "%.3f"))
				SetLocalRotation(v);
			
			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("world")) {
			ImGui::Indent(propertyIndent);

			if (auto v = GetWorldPosition(); ImGui::DragFloat3("world position", (float*)&v, 1, 0, 0, "%.1f"))
				SetWorldPosition(v);
			if (auto v = GetWorldRotation(); ImGui::DragFloat4("world rotation", (float*)&v, 0.01, 0, 1, "%.3f"))
				SetWorldRotation(v);
			
			ImGui::Unindent(propertyIndent);
			ImGui::TreePop();
		}
	}

	// Clears the object.
	virtual void Reset() {
		ForceUpdateCache();
	}

	void CallRecursive(std::function<void(SceneObject*)> f) {
		f(this);
		for (auto c : children)
			c->CallRecursive(f);
	}
	template<typename T>
	void CallRecursive(T parentState, std::function<T(SceneObject*, T)> f) {
		auto t = f(this, parentState);
		for (auto c : children)
			c->CallRecursive(t, f);
	}

	virtual SceneObject* Clone() const { return nullptr; }
	SceneObject& operator=(const SceneObject& o) {
		position = o.position;
		rotation = o.rotation;
		parent = o.parent;
		children = o.children;
		Name = o.Name;

		return *this;
	}
};

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

class StereoPolyLine : public LeafObject {
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
		shouldUpdateCache = false;
	}

public:

	StereoPolyLine() {}
	StereoPolyLine(const StereoPolyLine* copy) : LeafObject(copy){
		vertices = copy->vertices;
	}

	virtual ObjectType GetType() const override {
		return StereoPolyLineT;
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
		glDrawArrays(GL_LINE_STRIP, 0, GetVertices().size());
	}

	SceneObject* Clone() const override {
		return new StereoPolyLine(this);
	}
	StereoPolyLine& operator=(const StereoPolyLine& o) {
		vertices = o.vertices;
		LeafObject::operator=(o);
		return *this;
	}
};

struct Mesh : LeafObject {
private:
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
		shouldUpdateCache = false;
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
		auto fragmentShaderSource = GLLoader::ReadShader("shaders/WhiteSquare.frag");

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
		shouldUpdateCache = false;
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
		SetLocalPosition(oldPos);
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

class StereoCamera : public LeafObject
{
	// View coordinates
	float eyeToCenterDistance;
	glm::vec3 positionModifier;

	glm::vec3 GetPos() {
		return positionModifier + GetLocalPosition();
	}

	glm::vec3 getLeft(const glm::vec3& posMillimeters) {
		auto pos = ConvertMillimetersToViewCoordinates(posMillimeters, ViewSize.Get(), viewSizeZ);
		auto cameraPos = GetPos();
		float denominator = cameraPos.z - pos.z;
		return glm::vec3(
			(pos.x * cameraPos.z - pos.z * (cameraPos.x - eyeToCenterDistance)) / denominator,
			(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
			0
		);
	}
	glm::vec3 getRight(const glm::vec3& posMillimeters) {
		auto pos = ConvertMillimetersToViewCoordinates(posMillimeters, ViewSize.Get(), viewSizeZ);
		auto cameraPos = GetPos();
		float denominator = cameraPos.z - pos.z;
		return glm::vec3(
			(pos.x * cameraPos.z - pos.z * (cameraPos.x + eyeToCenterDistance)) / denominator,
			(cameraPos.z * -pos.y + cameraPos.y * pos.z) / denominator,
			0
		);
	}


	Event<> onPropertiesChanged;

	virtual void HandleBeforeUpdate() override {
		onPropertiesChanged.Invoke();
	}

	// Millimeters to pixels
	static glm::vec3 ConvertMillimetersToPixels(const glm::vec3& vMillimeters) {
		static float inchToMillimeter = 0.0393701;
		// vMillimiters[millimiter]
		// inchToMillemeter[inch/millimeter]
		// PPI[pixel/inch]
		// vMillimiters*PPI*inchToMillemeter[millimeter*(pixel/inch)*(inch/millimeter) = millimeter*(pixel/millimeter) = pixel]
		auto vPixels = Settings::PPI().Get() * inchToMillimeter * vMillimeters;
		return vPixels;
	}

	// Millimeters to [-1;1]
	// World center-centered
	// (0;0;0) in view coordinates corresponds to (0;0;0) in world coordinates
	static glm::vec3 ConvertMillimetersToViewCoordinates(const glm::vec3& vMillimeters, const glm::vec2& viewSizePixels, const float& viewSizeZMillimeters) {
		static float inchToMillimeter = 0.0393701;
		auto vsph = viewSizePixels / 2.f;
		auto vszmh = viewSizeZMillimeters / 2.f;

		auto vView = glm::vec3(
			vMillimeters.x * Settings::PPI().Get() * inchToMillimeter / vsph.x,
			vMillimeters.y * Settings::PPI().Get() * inchToMillimeter / vsph.y,
			vMillimeters.z / vszmh
		);
		return vView;
	}

	// Millimeters to [-1;1]
	// World center-centered
	// (0;0;0) in view coordinates corresponds to (0;0;0) in world coordinates
	static float ConvertMillimetersToViewCoordinates(const float& vMillimeters, const float& viewSizePixels) {
		static float inchToMillimeter = 0.0393701;
		auto vsph = viewSizePixels / 2.f;

		auto vView = vMillimeters * Settings::PPI().Get() * inchToMillimeter / vsph;
		return vView;
	}


	// Pixels to Millimeters
	static glm::vec3 ConvertPixelsToMillimeters(const glm::vec3& vPixels) {
		static float inchToMillimeter = 0.0393701;
		// vPixels[pixel]
		// inchToMillimeter[inch/centimeter]
		// PPI[pixel/inch]
		// vPixels/(PPI*inchToMillimeter)[pixel/((pixel/inch)*(inch/millimeter)) = pixel/(pixel/millimeter) = (pixel/pixel)*(millimeter) = millimiter]
		auto vMillimiters = vPixels / Settings::PPI().Get() / inchToMillimeter;
		return vMillimiters;
	}
	static glm::vec2 ConvertPixelsToMillimeters(const glm::vec2& vPixels) {
		static float inchToMillimeter = 0.0393701;
		// vPixels[pixel]
		// inchToMillimeter[inch/centimeter]
		// PPI[pixel/inch]
		// vPixels/(PPI*inchToMillimeter)[pixel/((pixel/inch)*(inch/millimeter)) = pixel/(pixel/millimeter) = (pixel/pixel)*(millimeter) = millimiter]
		auto vMillimiters = vPixels / Settings::PPI().Get() / inchToMillimeter;
		return vMillimiters;
	}

public:
	// Pixels
	Property<glm::vec2> ViewSize;
	// Millimeters
	float viewSizeZ = 100;
	Property<float> EyeToCenterDistance = 50;
	Property<glm::vec3> PositionModifier = glm::vec3(0, 5, 600);
	

	StereoCamera() {
		Name = "camera";
		EyeToCenterDistance.OnChanged() += [&](const float& v) {
			eyeToCenterDistance = ConvertMillimetersToViewCoordinates(v, ViewSize->x); };
		PositionModifier.OnChanged() += [&](const glm::vec3& v) {
			positionModifier = ConvertMillimetersToViewCoordinates(v, ViewSize.Get(), viewSizeZ); };

		ViewSize.OnChanged() += [&](const glm::vec2 v) {
			// Trigger conversion.
			EyeToCenterDistance.Set(EyeToCenterDistance.Get());
			PositionModifier.Set(PositionModifier.Get());
		};
	}

	IEvent<>& OnPropertiesChanged() {
		return onPropertiesChanged;
	}

	glm::vec3 GetLeft(const glm::vec3& v) {
		return getLeft(v);
	}
	glm::vec3 GetRight(const glm::vec3& v) {
		return getRight(v);
	}


	virtual ObjectType GetType() const override {
		return CameraT;
	}
	virtual void DesignProperties() override {
		if (ImGui::TreeNodeEx("camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			ImGui::Extensions::PushActive(false);
			ImGui::DragFloat2("view size", (float*)&ViewSize.Get());
			auto viewSizeMillimeters = ConvertPixelsToMillimeters(ViewSize.Get());
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
	
};
// Persistent object node
class PON {
	struct Node {
		SceneObject* object = nullptr;
		int referenceCount = 0;
	};
	Node* node = nullptr;
	static std::map<SceneObject*, Node*>& existingNodes() {
		static std::map<SceneObject*, Node*> v;
		return v;
	}
	constexpr void Init(const PON& o) {
		node = o.node;
		if (node)
			node->referenceCount++;
	}
public:
	PON() {
		node = nullptr;
	}
	PON(const PON& o) {
		Init(o);
	}
	PON(SceneObject* o) {
		if (auto n = existingNodes().find(o);
			n != existingNodes().end()) {
			node = n->second;
		}
		else {
			node = new Node();
			existingNodes()[o] = node;
			Set(o);
		}

		node->referenceCount++;
	}
	~PON() {
		if (!node)
			return;

		node->referenceCount--;
		if (node->referenceCount > 0)
			return;

		existingNodes().erase(node->object);
		Delete();
		delete node;
	}

	bool HasValue() const {
		return node && node->object;
	}

	SceneObject* Get() const {
		if (!node)
			throw new std::exception("PON doesn't have value");

		return node->object;
	}
	void Set(SceneObject* o) {
		if (node) {
			if (node->object)
				existingNodes().erase(node->object);
			Delete();
		}
		else
			node = new Node();

		node->object = o;
		existingNodes()[o] = node;
	}
	void Delete() {
		if (!node->object)
			return;

		delete node->object;
		node->object = nullptr;
	}

	SceneObject* operator->() {
		return Get();
	}

	constexpr PON& operator=(const PON& o) {
		Init(o);
		return *this;
	}
	constexpr bool operator<(const PON& o) const {
		return node < o.node;
	}
	constexpr bool operator!=(const PON& o) const {
		return node != o.node;
	}

	struct less {
		bool operator()(const PON& o1, const PON& o2) {
			return o1 < o2;
		}
	};
};