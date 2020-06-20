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
	static Property<bool>& ShouldDetectPosition() {
		static Property<bool> v;
		return v;
	}
};
