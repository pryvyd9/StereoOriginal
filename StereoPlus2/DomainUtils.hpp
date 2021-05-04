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
	StaticProperty(std::vector<PON>*, Objects)
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

class Transform {
	// Trims angle to 360 degrees
	static float trimAngle(float a) {
		int b = a;
		return b % 360 + (a - b);
	}

	// Trims angle to [-180;180] and converts to radian
	static float convertToDeltaDegree(float a) {
		return trimAngle(a) * 3.1415926f * 2 / 360;
	}

	// Get rotation angle around the axe.
	// Calculates a simple sum of absolute values of angles from all axes,
	// converts angle from radians to degrees
	// and then trims it to 180 degrees.
	static float getDirectedDeltaAngle(const glm::vec3& rotation) {
		float angle = 0;
		// Rotation axe determines the rotating direction 
		// so the rotation angles should be positive
		for (auto i = 0; i < 3; i++)
			angle += abs(rotation[i]);
		return trimAngle(angle) * 3.1415926f * 2 / 360;
	}

	// Get rotation angle around the axe.
	// Calculates a simple sum of absolute values of angles from all axes,
	// trims angle to 180 degrees and then converts to radians
	static bool tryGetLocalRotation(const glm::vec3& rotation, glm::vec3& axe, float& angle) {
		glm::vec3 t[2] = { glm::vec3(), glm::vec3() };
		float angles[2] = { 0, 0 };

		// Supports rotation around 1,2 axes simultaneously.
		auto modifiedAxeNumber = 0;
		for (auto i = 0; i < 3; i++) {
			assert(modifiedAxeNumber < 3);
			if (rotation[i] != 0) {
				t[modifiedAxeNumber][i] = rotation[i] > 0 ? 1 : -1;
				angles[modifiedAxeNumber] = abs(rotation[i]);
				modifiedAxeNumber++;
			}
		}

		if (modifiedAxeNumber == 0) {
			axe = glm::vec3();
			return false;
		}
		if (modifiedAxeNumber == 1) {
			angle = convertToDeltaDegree(angles[0]);
			axe = t[0];
			return true;
		}
		
		angle = convertToDeltaDegree(angles[0] + angles[1]);
		axe = t[0] + t[1];
		return true;
	}

	// Convert local rotation to global rotation relative to cross rotation.
	// 
	// Shortly, Isolates rotation in cross' local space.
	// 	   
	// Based on quaternion intransitivity, applying new local rotation and then applying inverted initial global rotation 
	// results in new local rotation being converted to global rotation.
	// r0 * r1Local * r0^-1 = r1Global
	// 
	// Basically, moves rotation initial point from (0;0;0;1) to cross global rotation.
	// Compromises performance since cross new rotation is calculated both here and ourside of this method
	// but improves readability and maintainability of the code.
	static glm::quat getIsolatedRotation(const glm::quat& localRotation, const glm::quat crossGlobalRotation) {
		auto crossNewRotation = crossGlobalRotation * localRotation;
		auto rIsolated = crossNewRotation * glm::inverse(crossGlobalRotation);
		return rIsolated;
	}

public:
	static void Scale(const glm::vec3& center, const float& oldScale, const float& scale, std::list<PON>& targets) {
		for (auto target : targets) {
			target->SetWorldPosition((target->GetWorldPosition() - center) / oldScale * scale + center);
			for (size_t i = 0; i < target->GetVertices().size(); i++)
				target->SetVertice(i, (target->GetVertices()[i] - center) / oldScale * scale + center);
		}
	}
	static void Translate(const glm::vec3& transformVector, std::list<PON>& targets, SceneObject* cross) {
		// Need to calculate average rotation.
		// https://stackoverflow.com/questions/12374087/average-of-multiple-quaternions/27410865#27410865
		if (Settings::SpaceMode().Get() == SpaceMode::Local) {
			auto r = glm::rotate(cross->GetWorldRotation(), transformVector);

			cross->SetWorldPosition(cross->GetWorldPosition() + r);
			for (auto o : targets) {
				o->SetWorldPosition(o->GetWorldPosition() + r);
				for (size_t i = 0; i < o->GetVertices().size(); i++)
					o->SetVertice(i, o->GetVertices()[i] + r);
			}
			return;
		}

		cross->SetWorldPosition(cross->GetWorldPosition() + transformVector);
		for (auto o : targets) {
			o->SetWorldPosition(o->GetWorldPosition() + transformVector);
			for (size_t i = 0; i < o->GetVertices().size(); i++)
				o->SetVertice(i, o->GetVertices()[i] + transformVector);
		}
	}
	static void Rotate(const glm::vec3& center, const glm::vec3& rotation, std::list<PON>& targets, SceneObject* cross) {
		glm::vec3 axe;
		float angle;

		if (!tryGetLocalRotation(rotation, axe, angle))
			return;

		auto r = glm::normalize(glm::angleAxis(angle, axe));

		// Convert local rotation to global rotation.
		// It is required to rotate all vertices since they don't store their own rotation
		// meaning that their rotation = (0;0;0;1).
		//
		// When multiple objects are selected cross' rotation is equal to the first 
		// object in selection. See TransformTool::OnSelectionChanged
		if (Settings::SpaceMode().Get() == SpaceMode::Local)
			r = getIsolatedRotation(r, cross->GetWorldRotation());

		cross->SetLocalRotation(r * cross->GetLocalRotation());

		for (auto& target : targets) {
			target->SetWorldPosition(glm::rotate(r, target->GetWorldPosition() - center) + center);
			for (size_t i = 0; i < target->GetVertices().size(); i++)
				target->SetVertice(i, glm::rotate(r, target->GetVertices()[i] - center) + center);

			target->SetWorldRotation(r * target->GetWorldRotation());
		}
	}
};
