#pragma once
#include "GLLoader.hpp"
#include "Settings.hpp"
#include <stack>

enum ObjectType {
	Group,
	PolyLineT,
	MeshT,
	CameraT,
	CrossT,
	TraceObjectT,
	SineCurveT,
	PointT,
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
	StaticField(size_t, freeId)

	// Unique id. Needed for PON. Doesn't need to be saved to file.
	size_t id;
	SceneObject* parent = nullptr;
protected:
	bool shouldTransformPosition = false;
	bool shouldTransformRotation = false;
	GLuint VBOLeft, VBORight;

	static bool& isAnyElementChanged() {
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
		static bool isTriggered = false;
		
		isAnyElementChanged() = true;

		if (!isTriggered) {
			isTriggered = true;
			onBeforeAnyElementChanged().Invoke();

			(new FuncCommand())->func = [&] {
				isTriggered = false;
			};
		}
	}
	virtual void UpdateOpenGLBuffer(
		std::function<glm::vec3(glm::vec3)> toStereo) {}


	// Adds or substracts transformations.

	virtual void CascadeTransform(std::vector<glm::vec3>& vertices) const {
		if (shouldTransformRotation && shouldTransformPosition)
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] = glm::rotate(GetRotation(), vertices[i]) + GetPosition();
		else if (shouldTransformRotation)
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] = glm::rotate(GetRotation(), vertices[i]);
		else if (shouldTransformPosition)
			for (size_t i = 0; i < vertices.size(); i++)
				vertices[i] += GetPosition();

		if (GetParent())
			GetParent()->CascadeTransform(vertices);
	}
	virtual void CascadeTransform(glm::vec3& v) const {
		if (shouldTransformRotation)
			v += glm::rotate(GetRotation(), v);
		if (shouldTransformPosition)
			v += GetPosition();

		if (GetParent())
			GetParent()->CascadeTransform(v);
	}
	virtual void CascadeTransformInverse(glm::vec3& v) const {
		if (GetParent())
			GetParent()->CascadeTransformInverse(v);

		if (shouldTransformPosition)
			v -= GetPosition();
		if (shouldTransformRotation)
			v += glm::rotate(glm::inverse(GetRotation()), v);
	}

public:
	std::vector<SceneObject*> children;
	std::string Name = "noname";

	SceneObjectMSProperty(glm::vec3, Position);
	SceneObjectMSPropertyDefault(glm::quat, Rotation, { unitQuat() });

	const size_t& Id() const {
		return id;
	}

	static IEvent<>& OnBeforeAnyElementChanged() {
		return onBeforeAnyElementChanged();
	}
	static bool IsAnyElementChanged() {
		return isAnyElementChanged();
	}
	static void ResetIsAnyElementChanged() {
		isAnyElementChanged() = false;
	}

	StaticFieldDefault(std::stack<bool>, isDeletionExpected, std::stack<bool>(std::deque<bool>({ true })))

	SceneObject() {
		id = freeId()++;

		glGenBuffers(2, &VBOLeft);
	}
	SceneObject(const SceneObject* copy) : SceneObject() {
		id = freeId()++;

		Position = copy->Position.Get();
		Rotation = copy->Rotation.Get();
		parent = copy->parent;
		children = copy->children;
		Name = copy->Name;
	}
	~SceneObject() {
		glDeleteBuffers(2, &VBOLeft);

		if (!isDeletionExpected().empty() && !isDeletionExpected().top())
			Log::For<SceneObject>().Warning("Deletion is not expected");
	}

	void UdateBuffer(std::function<glm::vec3(glm::vec3)> toStereo) {
		if (shouldUpdateCache || Settings::ShouldDetectPosition().Get()) {
			UpdateOpenGLBuffer(toStereo);
			shouldUpdateCache = false;
		}
	}
	virtual void DrawLeft(GLuint shader) {}
	virtual void DrawRight(GLuint shader) {}


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

		parent = newParent;

		if (shouldUpdateNewParent && newParent)
			newParent->children.push_back(this);
	}

	virtual ObjectType GetType() const = 0;
	virtual std::string GetDefaultName() {
		return "SceneObject";
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

		if (ImGui::TreeNodeEx("world", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent(propertyIndent);

			if (auto v = GetPosition(); ImGui::DragFloat3("Position", (float*)&v, 1, 0, 0, "%.1f"))
				Position = EventArgs<glm::vec3>(v, Source::GUI);
			if (auto v = GetRotation(); ImGui::DragFloat4("Rotation", (float*)&v, 0.01, -1, 1, "%.3f"))
				Rotation = EventArgs<glm::quat>(v, Source::GUI);

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

	virtual SceneObject* Clone() const { throw std::exception("not implemented"); }
	SceneObject& operator=(const SceneObject& o) {
		Position = o.Position.Get();
		Rotation = o.Rotation.Get();
		parent = o.parent;
		children = o.children;
		Name = o.Name;

		return *this;
	}
};

// Persistent object node
class PON {
	struct Node {
		SceneObject* object = nullptr;
		int referenceCount = 0;
	};
	Node* node = nullptr;

	static std::map<size_t, Node*>& existingNodes() {
		static std::map<size_t, Node*> v;
		return v;
	}
	constexpr void Init(const PON& o) {
		node = o.node;
		if (node)
			node->referenceCount++;
	}
	static const size_t* findNode(Node* o) {
		for (auto& [k, v] : existingNodes())
			if (v == o)
				return &k;

		return nullptr;
	}
public:
	PON() {
		node = nullptr;
	}
	PON(const PON& o) {
		Init(o);
	}
	PON(SceneObject* o) {
		if (auto n = existingNodes().find(o->Id()); n != existingNodes().end()) {
			node = n->second;
		}
		else {
			node = new Node();
			existingNodes()[o->Id()] = node;
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

		if (node->object)
			existingNodes().erase(node->object->Id());
		else if (auto n = findNode(node); n)
			existingNodes().erase(*n);

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
				existingNodes().erase(node->object->Id());
			Delete();
		}
		else
			node = new Node();

		node->object = o;
		existingNodes()[o->Id()] = node;
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
	const SceneObject* operator->() const {
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
	constexpr bool operator==(const PON& o) const {
		return node == o.node;
	}

	struct less {
		bool operator()(const PON& o1, const PON& o2) {
			return o1 < o2;
		}
	};
};
