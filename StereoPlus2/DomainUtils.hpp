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

		if (current->objects.size() != current->copies.size()) {
			logger().Error("The number of objects and the number their copies don't match.");
			throw new std::exception("The number of objects and the number their copies don't match.");
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

	static void ClearPresent() {
		if (presentBackup()) {
			DeleteState(presentBackup());
			presentBackup() = nullptr;
		}
	}
	static void ClearPast() {
		for (auto& s : pastStates())
			DeleteState(s);

		pastStates().clear();

		ClearPresent();
	}
	static void ClearFuture() {
		for (auto& s : futureStates())
			DeleteState(s);

		futureStates().clear();

		ClearPresent();
	}

	static void AbortApply(SceneObject* newRoot, std::map<SceneObject*, SceneObject*>& newCopies) {
		SceneObject::isDeletionExpected().push(true);

		for (auto& [f, s] : newCopies)
			delete s;
		delete newRoot;

		SceneObject::isDeletionExpected().pop();
	}

	static void Apply(std::vector<PON>& objects, State* saved) {
		std::map<SceneObject*, SceneObject*> newCopies;
		for (auto& [f, s] : saved->copies)
			newCopies[f] = s->Clone();

		auto newRoot = saved->rootCopy->Clone();

		SceneObject::isDeletionExpected().push(false);

		newRoot->CallRecursive((SceneObject*)nullptr, std::function<SceneObject* (SceneObject*, SceneObject*)>([&](SceneObject* o, SceneObject* p) {
			if (!o) {
				logger().Error("Null object was found in attempted state. Returning to previous active state.");
				AbortApply(newRoot, newCopies);
				
				SceneObject::isDeletionExpected().pop();
				throw new std::exception("State change aborted.");
			}

			if (p != nullptr)
				o->SetParent(p, false, true, false, false);

			for (size_t i = 0; i < o->children.size(); i++)
				if (auto pair = newCopies.find(o->children[i]); pair != newCopies.end())
					o->children[i] = pair->second;
				else {
					logger().Error("Child was not found in copied state. Returning to previous active state.");
					AbortApply(newRoot, newCopies);

					SceneObject::isDeletionExpected().pop();
					throw new std::exception("State change aborted.");
				}

			return o;
			}));

		SceneObject::isDeletionExpected().pop();

		RootObject() = newRoot;

		// Delete current objects.
		for (auto& o : objects)
			o.Delete();
		objects.clear();

		for (auto& [a, b] : saved->objects) {
			b.Set(newCopies[a]);
			objects.push_back(b);

			//int count = 0;
			//for (auto& v : objects)
			//	if (v.Get() == b.Get())
			//		count++;

			//if (count > 1)
			//	throw new std::exception("Found 2 nodes pointing to the same object. Each node must correspond to 1 object.");
		}



		std::vector<SceneObject*> newSelection;
		for (auto o : saved->selection)
			if (auto pair = newCopies.find(o); pair != newCopies.end())
				newSelection.push_back(pair->second);
			else
				logger().Warning("Attempted to select unexistent object during state change.");

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

		__try {
			ApplyPast(Objects().Get());
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger().Error("Error occured in Rollback");
			ClearPast();
			return;
		}

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

		__try {
			ApplyFuture(Objects().Get());
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			logger().Error("Error occured in Repeat");
			ClearFuture();
			return;
		}

		onStateChange().Invoke();
	}

	static void Clear() {
		ClearPast();
		ClearFuture();
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