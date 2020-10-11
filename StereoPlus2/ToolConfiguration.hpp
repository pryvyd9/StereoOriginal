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

class Settings {
public:
	StaticProperty(::ObjectMode, ObjectMode)
	StaticProperty(::SpaceMode, SpaceMode)
	StaticProperty(::MoveCoordinateAction, MoveCoordinateAction)
	StaticProperty(bool, ShouldDetectPosition)

	// Settings
	StaticProperty(float, CrossSpeed)
	StaticProperty(std::string, Language)
	StaticProperty(int, StateBufferLength)
};
