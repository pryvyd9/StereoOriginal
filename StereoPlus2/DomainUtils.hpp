#pragma once
#include "SceneObject.hpp"
#include <stack>
#include <algorithm>

enum SelectPosition {
	Anchor = 0x01,
	Rest = 0x10,
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
		selected().clear();
		selected().emplace(o);
		onChanged().Invoke(selected());
	}
	static void Set(const std::vector<SceneObject*>& os) {
		selected().clear();
		for (auto o : os)
			selected().emplace(o);
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

class StateBuffer {
public:
	// Buffer requires 1 additional state for saving current state.
	StaticProperty(int, BufferSize)
	StaticProperty(PON, RootObject)
	StaticProperty(std::vector<PON>, Objects)
private:
	struct State {
		// Reference, Shallow copy
		std::map<SceneObject*, SceneObject*> copies;

		// Objects in their original order
		std::vector<std::pair<SceneObject*, PON>> objects;

		std::vector<SceneObject*> selection;

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

		for (auto& o : ObjectSelection::Selected())
			current->selection.push_back(o.Get());
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

		std::vector<SceneObject*> newSelection;
		for (auto o : saved.selection)
			newSelection.push_back(newCopies[o]);

		ObjectSelection::Set(newSelection);

		auto newRoot = saved.rootCopy->Clone();

		newRoot->CallRecursive((SceneObject*)nullptr, std::function<SceneObject * (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* p) {
			if (p != nullptr) 
				o->SetParent(p, false, true, false, false);
			
			for (size_t i = 0; i < o->children.size(); i++)
				o->children[i] = newCopies[o->children[i]];

			return o;
			}));

		RootObject() = newRoot;
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
			!Objects().IsAssigned()) {
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
			PushPast(Objects().Get());
			position()--;
		}
		else if (position() == states().size()) {
			PushPast(Objects().Get());
			position()--;
		}

		ApplyPast(Objects().Get());

		onStateChange().Invoke();
	}

	// Do, Save current state
	static void Commit() {
		if (position() < states().size())
			ClearFuture();

		PushPast(Objects().Get());
	}

	// Repeat, Redo, Apply next state
	static void Repeat() {
		if (states().empty() || position() >= states().size() - 1)
			return;

		ApplyFuture(Objects().Get());

		onStateChange().Invoke();
	}

	static void Clear() {
		Erase(0, states().size());
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

