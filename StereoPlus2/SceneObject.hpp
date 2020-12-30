#pragma once
#include "GLLoader.hpp"
#include "Settings.hpp"

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

		//glStencilMask(stencilMaskLeft);
		//glStencilFunc(GL_ALWAYS, stencilMaskLeft, stencilMaskLeft | stencilMaskRight);
		//glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
		DrawLeft(shaderLeft);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

		//glStencilMask(stencilMaskRight);
		//glStencilFunc(GL_ALWAYS, stencilMaskRight, stencilMaskLeft | stencilMaskRight);
		//glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
		DrawRight(shaderRight);

		glDisable(GL_BLEND);

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

		position = shouldTransformPosition && GetParent()
			// Set world position means to set local position
			// relative to parent.
			? GetParent()->ToLocalPosition(v)
			: v;
	}

	const glm::quat& GetLocalRotation() const {
		return rotation;
	}
	const virtual glm::quat GetWorldRotation() const {
		return shouldTransformRotation && GetParent()
			? GetParent()->GetWorldRotation() * GetLocalRotation()
			: GetLocalRotation();
	}
	void SetLocalRotation(const glm::quat& v) {
		ForceUpdateCache();
		rotation = v;
	}
	void SetWorldRotation(const glm::quat& v) {
		ForceUpdateCache();

		rotation = shouldTransformRotation && GetParent()
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

		//if (ImGui::TreeNodeEx("local", ImGuiTreeNodeFlags_DefaultOpen)) {
		//	ImGui::Indent(propertyIndent);

		//	if (auto v = GetLocalPosition(); ImGui::DragFloat3("local position", (float*)&v, 1, 0, 0, "%.1f"))
		//		SetLocalPosition(v);
		//	if (auto v = GetLocalRotation(); ImGui::DragFloat4("local rotation", (float*)&v, 0.01, 0, 1, "%.3f"))
		//		SetLocalRotation(v);

		//	ImGui::Unindent(propertyIndent);
		//	ImGui::TreePop();
		//}

		if (ImGui::TreeNodeEx("world", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			if (auto v = GetWorldPosition(); ImGui::DragFloat3("world position", (float*)&v, 1, 0, 0, "%.1f"))
				SetWorldPosition(v);
			if (auto v = GetWorldRotation(); ImGui::DragFloat4("world rotation", (float*)&v, 0.01, -1, 1, "%.3f"))
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
