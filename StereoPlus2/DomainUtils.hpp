#pragma once
#include "DomainTypes.hpp"
#include <stack>

enum SelectPosition {
	Anchor = 0x01,
	Rest = 0x10,
};

class StateBuffer {
	struct State {
		// Reference, Shallow copy
		std::map<SceneObject*, SceneObject*> createdObjects;
		std::map<SceneObject*, SceneObject*> otherObjects;
		std::vector<SceneObject*> deletedObjects;
		
		// Objects in their original order
		std::vector<SceneObject*> objects;
	};

	static std::list<State>& states() {
		static std::list<State> v(1);
		return v;
	}

	static int& position() {
		// Point to states().size().
		static int v = 1;
		return v;
	}

	//static void PushPast(std::vector<SceneObject*> objects) {
	//	// check if item existed. If not - put to created
	//	// check if there are items that are no longer present. Put them to deleted objects.

	//	auto last = &states().top();
	//	auto last = &at(position());
	//	states().push(State());
	//	auto current = &states().top();

	//	// Find other and created objects
	//	for (auto o : objects) {
	//		if (last->otherObjects.find(o) != last->otherObjects.end())
	//			current->otherObjects[o] = o->Clone();
	//		else
	//			current->createdObjects[o] = o->Clone();
	//	}
	//	
	//	// Find deleted objects
	//	for (auto o : last->createdObjects)
	//		if (current->otherObjects.find(o.first) == current->otherObjects.end())
	//			current->deletedObjects.push_back(o.first);
	//	for (auto o : last->otherObjects)
	//		if (current->otherObjects.find(o.first) == current->otherObjects.end())
	//			current->deletedObjects.push_back(o.first);
	//}

	static void PushPast(std::vector<SceneObject*>& objects) {
		// check if item existed. If not - put to created
		// check if there are items that are no longer present. Put them to deleted objects.

		//if (states().empty()) {
		//	states().push_back(State());
		//	position()++;
		//	auto current = &states().back();

		//	for (auto o : objects)
		//		current->createdObjects[o] = o->Clone();

		//	current->objects = objects;

		//	return;
		//}


		auto last = &states().back();
		states().push_back(State());
		position()++;
		auto current = &states().back();

		// Find other and created objects
		for (auto o : objects) {
			if (last->otherObjects.find(o) != last->otherObjects.end())
				current->otherObjects[o] = o->Clone();
			else
				current->createdObjects[o] = o->Clone();
		}

		// Find deleted objects
		for (auto o : last->createdObjects)
			if (current->otherObjects.find(o.first) == current->otherObjects.end())
				current->deletedObjects.push_back(o.first);
		for (auto o : last->otherObjects)
			if (current->otherObjects.find(o.first) == current->otherObjects.end())
				current->deletedObjects.push_back(o.first);

		current->objects = objects;
	}

	// Erase saved copies.
	static void Erase(int from, int to) {
		auto f = iterAt(from);
		auto t = iterAt(to);
		for (auto it = f; it != t; it++) {
			for (auto o : it->createdObjects)
				delete o.second;
			for (auto o : it->otherObjects)
				delete o.second;
		}
		states().erase(f, t);
	}
	static void ClearPast() {
		if (position() < 1)
			return;

		Erase(0, position());

		position() = 0;
	}
	static void ClearFuture() {
		if (position() >= states().size() - 1)
			return;

		Erase(position() + 1, states().size());
	}

	static std::list<State>::iterator& iterAt(int pos) {
		auto it = states().begin();
		for (size_t i = 0; i < pos; i++, it++);
		return it;
	}
	static State& at(int pos) {
		return iterAt(pos)._Ptr->_Myval;
	}
	static bool isModern() {
		return position() == states().size();
	}

	static void ApplyPast(std::vector<SceneObject*>& objects) {
		auto current = at(position() - 1);
		auto past = at(position() - 2);

		// Delete current objects.
		for (auto o : objects)
			delete o;

		objects = past.objects;

		for (auto i = 0; i < objects.size(); i++) {
			if (auto d = past.otherObjects.find(objects[i]);
				d != past.otherObjects.end())
				objects[i] = d->second->Clone();
			else 
				objects[i] = past.createdObjects[objects[i]]->Clone();
		}
	}

public:
	static State& CurrentState() {
		return at(position());
	}

	// Revert, Undo, Apply previous state
	static void Rollback(std::vector<SceneObject*>& objects) {
		if (states().empty())
			return;

		if (isModern())
			Commit(objects);

		ApplyPast(objects);
	}

	// Do, Save current state
	static void Commit(std::vector<SceneObject*>& objects) {
		if (!states().empty())
			ClearFuture();

		PushPast(objects);
	}

	// Repeat, Redo, Apply next state
	static void Repeat(std::vector<SceneObject*>& objects) {

	}

	static void Clear() {
		Erase(0, states().size());
	}
};

class SceneObjectSelection {
public:
	using Selection = std::set<SceneObject*>;
private:
	static Selection& selected() {
		static Selection v;
		return v;
	}
public:
	static const Selection& Selected() {
		return selected();
	}
	static const Selection** SelectedP() {
		static const Selection* v = &selected();
		return &v;
	}


	static void Set(SceneObject* o) {
		RemoveAll();
		Add(o);
	}

	static void Add(SceneObject* o) {
		selected().emplace(o);
	}

	static void RemoveAll() {
		selected().clear();
	}

	static void Remove(SceneObject* o) {
		selected().erase(o);
	}
};

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

	//static void EmplaceDragDropSceneObject(const char* name, SceneObject* objectPointer, Buffer* buffer) {
	//	(*buffer)->emplace(objectPointer);

	//	ImGui::SetDragDropPayload("SceneObjects", buffer, sizeof(Buffer));
	//}
	//static void EmplaceDragDropSceneObject(const char* name, const std::set<SceneObject*>* objectPointers, Buffer* buffer) {
	//	(*buffer)->merge(*const_cast<std::set<SceneObject*>*>(objectPointers));
	//	//(*buffer)->set_uni(*const_cast<std::set<SceneObject*>*>(objectPointers));
	//	//(*buffer)->emplace(objectPointer);
	//	auto j = objectPointers->size();
	//	ImGui::SetDragDropPayload("SceneObjects", buffer, sizeof(Buffer));
	//}

	static void EmplaceDragDropSelected(const char* name) {
		ImGui::SetDragDropPayload("SceneObjects", SceneObjectSelection::SelectedP(), sizeof(SceneObjectSelection::Selection*));
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
	Event<> deleteAll;

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
public:
	// Stores all objects.
	std::vector<SceneObject*> objects;

	SceneObject* root = &defaultObject;
	StereoCamera* camera;
	Cross* cross;

	GLFWwindow* glWindow;

	IEvent<>& OnDeleteAll() {
		return deleteAll;
	}

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
			if (source->children[i] == obj) {
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

	void DeleteSelected() {
		for (auto o : SceneObjectSelection::Selected())
			Delete(const_cast<SceneObject*>(o->GetParent()), o);
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

		// Will be moved to destination
		std::list<SceneObject*> disconnectedItemsToBeMoved;

		// Will be ignored during move but will be moved with their parents
		std::set<SceneObject*> connectedItemsToBeMoved;

		// Items which will not be moved with their parents.
		// New parents will be assigned.
		// Item, NewParent
		std::list<std::pair<SceneObject*, SceneObject*>> strayItems;


		root->CallRecursive(root, std::function<SceneObject * (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* parent) {
			if (exists(*items, o)) {
				if (exists(connectedItemsToBeMoved, parent) || exists(disconnectedItemsToBeMoved, parent))
					connectedItemsToBeMoved.emplace(o);
				else if (auto newParent = FindConnectedParent(parent, disconnectedItemsToBeMoved))
					strayItems.push_back({ o, newParent });
				else
					disconnectedItemsToBeMoved.push_back(o);
			}
			else if (exists(*items, parent))
				strayItems.push_back({ o, FindNewParent(parent, items) });

			return o;
			}));

		if ((pos & InsertPosition::Bottom) != 0)
			for (auto o : disconnectedItemsToBeMoved)
				o->SetParent(destination, destinationPos, pos);
		else
			std::for_each(disconnectedItemsToBeMoved.rbegin(), disconnectedItemsToBeMoved.rend(), [&] (auto o){
				o->SetParent(destination, destinationPos, pos); });

		for (auto pair : strayItems)
			pair.first->SetParent(pair.second, true);

		return true;
	}


	void DeleteAll() {
		deleteAll.Invoke();
		cross->SetParent(nullptr);

		for (auto o : objects)
			delete o;

		objects.clear();
		root = &defaultObject;
		root->children.clear();
	}

	~Scene() {
		for (auto o : objects)
			delete o;
	}
};
