#pragma once
#include "DomainTypes.hpp"
#include <stack>

enum SelectPosition {
	Anchor = 0x01,
	Rest = 0x10,
};

class StateBuffer {
public:
	// Buffer requires 1 additional state for saving current state.
	StaticProperty(int, BufferSize)
	StaticProperty(SceneObject*, RootObject)
	StaticProperty(std::vector<PON>*, Objects)
private:
	struct State {
		// Reference, Shallow copy
		std::map<SceneObject*, SceneObject*> copies;

		// Objects in their original order
		std::vector<std::pair<SceneObject*, PON>> objects;

		SceneObject* rootCopy;
	};

	static std::list<State>& states() {
		static std::list<State> v;
		return v;
	}

	static int& position() {
		static int v = 0;
		return v;
	}

	static void PushPast(std::vector<PON>& objects) {
		states().push_back(State());

		if (auto correctedBuffer = BufferSize().Get() + 1;
			correctedBuffer < states().size())
			Erase(0, states().size() - correctedBuffer);

		position() = states().size();

		auto current = &states().back();

		current->rootCopy = RootObject().Get()->Clone();

		for (auto o : objects) {
			current->objects.push_back({ o.Get(), o });
			current->copies[o.Get()] = o->Clone();
		}
	}

	// Erase saved copies.
	static void Erase(int from, int to) {
		auto f = iterAt(from);
		auto t = iterAt(to);
		for (auto it = f; it != t; it++) {
			delete it->rootCopy;
			for (auto o : it->copies)
				delete o.second;
		}

		states().erase(f, t);
	}
	static void ClearFuture() {
		Erase(position(), states().size());
	}

	static std::list<State>::iterator iterAt(int pos) {
		auto it = states().begin();
		for (size_t i = 0; i < pos; i++, it++);
		return it;
	}
	static State& at(int pos) {
		return iterAt(pos)._Ptr->_Myval;
	}

	static void Apply(std::vector<PON>& objects, int pos) {
		// Saved state at position pos.
		auto saved = at(pos);

		delete RootObject().Get();

		// Delete current objects.
		for (auto o : objects)
			o.Delete();

		std::map<SceneObject*, SceneObject*> newCopies;
		for (auto o : saved.copies)
			newCopies[o.first] = o.second->Clone();

		objects.clear();
		for (auto o : saved.objects) {
			o.second.Set(newCopies[o.first]);
			objects.push_back(o.second);
		}

		auto newRoot = saved.rootCopy->Clone();

		newRoot->CallRecursive((SceneObject*)nullptr, std::function<SceneObject * (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* p) {
			if (p != nullptr) 
				o->SetParent(p, false, true, false, false);
			
			for (size_t i = 0; i < o->children.size(); i++)
				o->children[i] = newCopies[o->children[i]];

			return o;
			}));

		RootObject().Set(newRoot);
	}
	static void ApplyPast(std::vector<PON>& objects) {
		position()--;
		Apply(objects, position());
	}
	static void ApplyFuture(std::vector<PON>& objects) {
		position()++;
		Apply(objects, position());
	}

public:
	static bool Init() {
		if (!RootObject().Get() ||
			!Objects().Get()) {
			Log::For<StateBuffer>().Error("Initialization failed.");
			return false;
		}

		return true;
	}

	// Revert, Undo, Apply previous state
	static void Rollback() {
		if (position() < 1)
			return;

		if (position() == states().size() - 1) {
			ClearFuture();
			PushPast(*Objects().Get());
			position()--;
		}
		else if (position() == states().size()) {
			PushPast(*Objects().Get());
			position()--;
		}

		ApplyPast(*Objects().Get());
	}

	// Do, Save current state
	static void Commit() {
		if (position() < states().size())
			ClearFuture();

		PushPast(*Objects().Get());
	}

	// Repeat, Redo, Apply next state
	static void Repeat() {
		if (states().empty() || position() >= states().size() - 1)
			return;

		ApplyFuture(*Objects().Get());
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
	static Event<const Selection&>& onChanged() {
		static Event<const Selection&> v;
		return v;
	}
public:
	static IEvent<const Selection&>& OnChanged() {
		return onChanged();
	}

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
		onChanged().Invoke(selected());
	}
	static void Add(SceneObject* o) {
		selected().emplace(o);
		onChanged().Invoke(selected());
	}
	static void RemoveAll() {
		selected().clear();
		onChanged().Invoke(selected());
	}
	static void Remove(SceneObject* o) {
		selected().erase(o);
		onChanged().Invoke(selected());
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
	std::vector<PON> objects;

	Property<SceneObject*> root;
	StereoCamera* camera;
	Cross* cross;

	GLFWwindow* glWindow;

	IEvent<>& OnDeleteAll() {
		return deleteAll;
	}

	Scene() {
		root = new GroupObject();
		root.Get()->Name = "Root";
	}

	bool Insert(SceneObject* destination, SceneObject* obj) {
		obj->SetParent(destination);
		objects.push_back(obj);
		return true;
	}
	bool Insert(SceneObject* obj) {
		obj->SetParent(root.Get());
		objects.push_back(obj);
		return true;
	}

	bool Delete(SceneObject* source, SceneObject* obj) {
		for (size_t i = 0; i < source->children.size(); i++)
			if (source->children[i] == obj)
				for (size_t j = 0; i < objects.size(); j++)
					if (objects[j].Get() == obj) {
						source->children.erase(source->children.begin() + i);
						objects.erase(objects.begin() + j);
						return true;
					}

		std::cout << "The object for deletion was not found" << std::endl;
		return false;
	}
	void DeleteSelected() {
		for (auto o : SceneObjectSelection::Selected())
			Delete(const_cast<SceneObject*>(o->GetParent()), o);
	}
	void DeleteAll() {
		deleteAll.Invoke();
		cross->SetParent(nullptr);

		delete root.Get();

		objects.clear();
		root = new GroupObject();
		root.Get()->Name = "Root";
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


	~Scene() {
		delete root.Get();
		objects.clear();
	}
};
