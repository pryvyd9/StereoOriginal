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
		selected().insert(os.begin(), os.end());
		onChanged().Invoke(selected());
	}
	static void Set(const std::vector<PON>& os) {
		selected().clear();
		selected().insert(os.begin(), os.end());
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

// State Buffer
class Changes {
public:
	StaticProperty(PON, RootObject)
	StaticProperty(std::vector<PON>, Objects)
	StaticProperty(bool, ShouldIgnoreCommit)
private:
	struct State {
		// Reference, Shallow copy
		std::map<SceneObject*, SceneObject*> copies;

		// Objects in their original order
		std::vector<std::pair<SceneObject*, PON>> objects;

		std::vector<SceneObject*> selection;

		SceneObject* rootCopy;
	};

	StaticField(std::list<State*>, pastStates)
	StaticField(std::list<State*>, futureStates)
	StaticField(State*, presentBackup)
	StaticFieldDefault(Log, logger, Log::For<State>())

	StaticField(Event<>, onStateChange)

	static State* CreateState(std::vector<PON>& objects) {
		auto current = new State();

		current->rootCopy = RootObject().Get()->Clone();

		for (auto& o : objects) {
			current->objects.push_back({ o.Get(), o });
			current->copies[o.Get()] = o->Clone();
		}

		for (auto& o : ObjectSelection::Selected())
			current->selection.push_back(o.Get());

		return current;
	}


	static void PushPast(std::vector<PON>& objects) {
		auto current = new State();
		{
			pastStates().push_back(current);

			if (Settings::StateBufferLength().Get() < pastStates().size())
				EraseOldestState();
		}

		current->rootCopy = RootObject().Get()->Clone();

		for (auto& o : objects) {
			current->objects.push_back({ o.Get(), o });
			current->copies[o.Get()] = o->Clone();
		}

		for (auto& o : ObjectSelection::Selected())
			current->selection.push_back(o.Get());
	}

	static void EraseOldestState() {
		DeleteState(pastStates().front());

		pastStates().pop_front();
	}

	static void DeleteState(State* state) {
		delete state->rootCopy;
		for (auto& o : state->copies)
			delete o.second;

		delete state;
	}

	static void ClearFuture() {
		for (auto& s : futureStates())
			DeleteState(s);

		futureStates().clear();
	}

	static void Apply(std::vector<PON>& objects, State* saved) {
		std::map<SceneObject*, SceneObject*> newCopies;
		for (auto& [f, s] : saved->copies)
			newCopies[f] = s->Clone();

		if (newCopies.size() != saved->copies.size()) {
			std::cout << "sizes don't match" << std::endl;
		}

		auto newRoot = saved->rootCopy->Clone();

		SceneObject::isDeletionExpected() = false;

		newRoot->CallRecursive((SceneObject*)nullptr, std::function<SceneObject* (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* p) {
			if (!o) {
				logger().Error("Null object was found in attempted state. Returning to previous active state.");
				//std::cout << "null" << std::endl;
			}
			if (p != nullptr)
				o->SetParent(p, false, true, false, false);

			for (size_t i = 0; i < o->children.size(); i++)
				o->children[i] = newCopies[o->children[i]];

			return o;
			}));

		SceneObject::isDeletionExpected() = true;

		RootObject() = newRoot;

		// Delete current objects.
		for (auto& o : objects)
			o.Delete();
		objects.clear();

		for (auto& [a, b] : saved->objects) {
			b.Set(newCopies[a]);
			objects.push_back(b);
		}

		std::vector<SceneObject*> newSelection;
		for (auto o : saved->selection)
			newSelection.push_back(newCopies[o]);

		ObjectSelection::Set(newSelection);
	}

	static void ApplyPast(std::vector<PON>& objects) {
		auto stateToApply = pastStates().back();
		pastStates().pop_back();
		
		if (presentBackup())
			futureStates().push_back(presentBackup());

		presentBackup() = stateToApply;

		Apply(objects, stateToApply);
	}
	static void ApplyFuture(std::vector<PON>& objects) {
		auto stateToApply = futureStates().back();
		futureStates().pop_back();

		if (presentBackup())
			pastStates().push_back(presentBackup());

		presentBackup() = stateToApply;

		Apply(objects, stateToApply);
	}

public:
	static IEvent<>& OnStateChange() {
		return onStateChange();
	}

	static bool Init() {
		if (!RootObject().Get().Get() ||
			!Objects().IsAssigned()) {
			Log::For<Changes>().Error("Initialization failed.");
			return false;
		}

		// Commit initial state
		Commit();

		return true;
	}

	// Revert, Undo, Apply previous state
	static void Rollback() {
		if (pastStates().empty())
			return;

		ApplyPast(Objects().Get());

		onStateChange().Invoke();
	}

	// Do, Save current state
	static void Commit() {
		if (ShouldIgnoreCommit().Get())
			return;

		if (!futureStates().empty())
			ClearFuture();

		if (presentBackup()) {
			pastStates().push_back(presentBackup());
			presentBackup() = nullptr;
		}

		if (Settings::StateBufferLength().Get() < pastStates().size())
			EraseOldestState();

		presentBackup() = CreateState(Objects().Get());
	}

	// Repeat, Redo, Apply next state
	static void Repeat() {
		if (futureStates().empty())
			return;

		ApplyFuture(Objects().Get());

		onStateChange().Invoke();
	}

	static void Clear() {
		ClearFuture();

		for (auto& s : pastStates())
			DeleteState(s);

		pastStates().clear();

		if (presentBackup()) {
			DeleteState(presentBackup());
			presentBackup() = nullptr;
		}
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