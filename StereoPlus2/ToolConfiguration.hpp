#pragma once

#include "InfrastructureTypes.hpp"

enum class PointPenEditingToolMode {
	Immediate,
	Step,
};

enum class ExtrusionEditingToolMode {
	Immediate,
	Step,
};

enum class ObjectMode {
	Object,
	Vertex,
};

enum class SpaceMode {
	World,
	Local,
};

enum class TransformToolMode {
	Translate,
	Scale,
	Rotate,
};

enum class MoveCoordinateAction {
	Adapt,
	None,
};


//class GlobalToolConfiguration {
//	static ObjectMode& objectMode() {
//		static ObjectMode v;
//		return v;
//	}
//	static SpaceMode& spaceMode() {
//		static SpaceMode v;
//		return v;
//	}
//	static MoveCoordinateAction& moveCoordinateAction() {
//		static MoveCoordinateAction v;
//		return v;
//	}
//
//	static Event<ObjectMode>& objectModeChanged() {
//		static Event<ObjectMode> v;
//		return v;
//	}
//	static Event<SpaceMode>& spaceModeChanged() {
//		static Event<SpaceMode> v;
//		return v;
//	}
//	static Event<MoveCoordinateAction>& moveCoordinateActionChanged() {
//		static Event<MoveCoordinateAction> v;
//		return v;
//	}
//public:
//	static const ObjectMode& GetCoordinateMode() {
//		return objectMode();
//	}
//	static const SpaceMode& GetSpaceMode() {
//		return spaceMode();
//	}
//	static const MoveCoordinateAction& GetMoveCoordinateAction() {
//		return moveCoordinateAction();
//	}
//	
//	static void SetObjectMode(const ObjectMode& v) {
//		auto a = objectMode();
//		objectMode() = v;
//		if (a != v)
//			objectModeChanged().Invoke(v);
//	}
//	static void SetSpaceMode(const SpaceMode& v) {
//		auto a = spaceMode();
//		spaceMode() = v;
//		if (a != v)
//			spaceModeChanged().Invoke(v);
//	}
//	static void SetMoveCoordinateAction(const MoveCoordinateAction& v) {
//		auto a = moveCoordinateAction();
//		moveCoordinateAction() = v;
//		if (a != v)
//			moveCoordinateActionChanged().Invoke(v);
//	}
//
//	static IEvent<ObjectMode>& OnObjectModeChanged() {
//		return objectModeChanged();
//	}
//	static IEvent<SpaceMode>& OnSpaceModeChanged() {
//		return spaceModeChanged();
//	}
//	static IEvent<MoveCoordinateAction>& OnMoveCoordinateActionChanged() {
//		return moveCoordinateActionChanged();
//	}
//};

class GlobalToolConfiguration {
public:
	static Property<ObjectMode>& ObjectMode() {
		static Property<::ObjectMode> v;
		return v;
	}
	static Property<SpaceMode>& SpaceMode() {
		static Property<::SpaceMode> v;
		return v;
	}
	static Property<MoveCoordinateAction>& MoveCoordinateAction() {
		static Property<::MoveCoordinateAction> v;
		return v;
	}
};
