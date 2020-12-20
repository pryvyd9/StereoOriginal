#pragma once

#include "InfrastructureTypes.hpp"
#include "Key.hpp"

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
	StaticProperty(std::string, Language)
	StaticProperty(int, StateBufferLength)
	
	StaticProperty(bool, UseDiscreteMovement)
	StaticProperty(float, TransitionStep)
	StaticProperty(float, RotationStep)
	StaticProperty(float, ScaleStep)

	StaticProperty(Key::Combination, TransformToolShortcut)
	StaticProperty(Key::Combination, PenToolShortcut)
	StaticProperty(Key::Combination, ExtrusionToolShortcut)
	StaticProperty(Key::Combination, RenderViewportToFile)
	StaticProperty(Key::Combination, RenderAdvancedToFile)

	StaticProperty(Key::Combination, SwitchUseDiscreteMovement)

	static const std::string& Name(void* reference) {
		static std::map<void*, const std::string> v = {
			{&Language,"language"},
			{&StateBufferLength,"stateBufferLength"},

			{&UseDiscreteMovement,"useDiscreteMovement"},
			{&TransitionStep,"transitionStep"},
			{&RotationStep,"rotationStep"},
			{&ScaleStep,"scaleStep"},
		};

		if (auto a = v.find(reference); a != v.end())
			return a._Ptr->_Myval.second;

		throw new std::exception("Name for a reference was not found.");
	}

};
