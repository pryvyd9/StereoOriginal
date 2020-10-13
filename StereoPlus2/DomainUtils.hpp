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
	StaticProperty(PON, RootObject)
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

	static Event<>& onStateChange() {
		static Event<> v;
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

		// Delete current objects.
		for (auto& o : objects)
			o.Delete();

		std::map<SceneObject*, SceneObject*> newCopies;
		for (auto [f,s] : saved.copies)
			newCopies[f] = s->Clone();

		objects.clear();
		for (auto [a,b] : saved.objects) {
			b.Set(newCopies[a]);
			objects.push_back(b);
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
	static IEvent<>& OnStateChange() {
		return onStateChange();
	}

	static bool Init() {
		if (!RootObject().Get().Get() ||
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

		onStateChange().Invoke();
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

		onStateChange().Invoke();
	}

	static void Clear() {
		Erase(0, states().size());
	}
};

class ObjectSelection {
public:
	using Selection = std::set<PON>;
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


class DragDropBuffer {
public:
	using Buffer = ObjectSelection::Selection*;
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
	static bool PopDragDropPayload(const char* name, ImGuiDragDropFlags flags, std::vector<PON>* outSceneObjects) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(name, flags)) {
			auto objectPointers = GetBuffer(payload->Data);

			for (auto objectPointer : *objectPointers)
				outSceneObjects->push_back(objectPointer);

			return true;
		}

		return false;
	}

	static void EmplaceDragDropSelected(const char* name) {
		ImGui::SetDragDropPayload("SceneObjects", ObjectSelection::SelectedP(), sizeof(ObjectSelection::Selection*));
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
	Log Logger = Log::For<Scene>();

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
	std::vector<PON> objects;

	Property<PON> root;
	StereoCamera* camera;
	Cross* cross;

	GLFWwindow* glWindow;

	

	IEvent<>& OnDeleteAll() {
		return deleteAll;
	}

	Scene() {
		root = CreateRoot();
	}
	
	bool Insert(SceneObject* destination, SceneObject* obj) {
		obj->SetParent(destination);
		objects.push_back(obj);
		return true;
	}
	bool Insert(SceneObject* obj) {
		obj->SetParent(root.Get().Get());
		objects.push_back(obj);
		return true;
	}

	bool Delete(SceneObject* source, SceneObject* obj) {
		if (!source) {
			Logger.Warning("You cannot delete root object");
			return true;
		}

		for (size_t i = 0; i < source->children.size(); i++)
			if (source->children[i] == obj)
				for (size_t j = 0; i < objects.size(); j++)
					if (objects[j].Get() == obj) {
						source->children.erase(source->children.begin() + i);
						objects.erase(objects.begin() + j);
						return true;
					}

		Logger.Error("The object for deletion was not found");
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
		deleteAll.Invoke();
		cross->SetParent(nullptr);

		objects.clear();
		root = CreateRoot();
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
		root->CallRecursive(root, std::function<SceneObject * (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* parent) {
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
		for (auto [o,np] : co.orphanedObjects)
			categorizedObjects.orphanedObjects.push_back({ o, np });
	}
	static void CategorizeObjects(SceneObject* root, std::vector<PON>& items, CategorizedPON& categorizedObjects) {
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
			std::for_each(categorizedObjects.parentObjects.rbegin(), categorizedObjects.parentObjects.rend(), [&] (auto o){
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

	~Scene() {
		objects.clear();
	}
};
