#pragma once

#include "InfrastructureTypes.hpp"
#include "Key.hpp"

enum class PolylinePenEditingToolMode {
	Immediate,
	Step,
};

enum class CosinePenEditingToolMode {
	Step123,
	Step132,
};

enum class ExtrusionEditingToolMode {
	Immediate,
	Step,
};

//enum class ObjectMode {
//	Object,
//	Vertex,
//};

enum class SpaceMode {
	World,
	Local,
};

enum class TargetMode {
	Object,
	Pivot,
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

enum class NavigationMode {
	Cross,
	Camera,
};

class Settings {
public:
	//StaticProperty(::ObjectMode, ObjectMode)
	StaticProperty(::SpaceMode, SpaceMode)
	StaticProperty(::TargetMode, TargetMode)
	StaticProperty(::MoveCoordinateAction, MoveCoordinateAction)
	StaticProperty(bool, ShouldDetectPosition)

	StaticProperty(bool, ShouldRestrictTargetModeToPivot)
	StaticProperty(::NavigationMode, NavigationMode)

	// Settings
	StaticProperty(std::string, Language)
	StaticProperty(int, StateBufferLength)
	StaticProperty(std::string, LogFileName)
	// Display Pixels Per Centimeter
	StaticProperty(float, PPI)
	StaticProperty(bool, IsAutosaveEnabled)
	StaticProperty(int, AutosavePeriodMinutes)

	StaticProperty(bool, UseDiscreteMovement)
	StaticProperty(float, TranslationStep)
	StaticProperty(float, RotationStep)
	StaticProperty(float, ScalingStep)
	StaticProperty(float, MouseSensivity)

	StaticProperty(glm::vec4, ColorLeft)
	StaticProperty(glm::vec4, ColorRight)
	StaticProperty(glm::vec4, DimmedColorLeft)
	StaticProperty(glm::vec4, DimmedColorRight)

	StaticProperty(float, CustomRenderWindowAlpha)

	StaticProperty(bool, ShouldMoveCrossOnCosinePenModeChange)

	StaticProperty(int, PointRadiusPixel)
	StaticProperty(int, LineThickness)

	StaticProperty(int, CosinePointCount)
	
	StaticProperty(glm::vec2, CameraResolution)
	StaticProperty(glm::vec2, CameraViewAngles)
	StaticProperty(glm::vec2, CameraAngle)

	StaticProperty(float, FaceSizeYMillimeters)
	StaticProperty(glm::vec3, ScreenCenterToCameraDistanceMillimeters)


	// Readonly system fields
	StaticField(int, MinLineThickness)
	StaticField(int, MaxLineThickness)

	static const std::string& Name(void* reference) {
		static std::map<void*, const std::string> v = {
			{&Language,"language"},
			{&StateBufferLength,"stateBufferLength"},
			{&LogFileName,"logFileName"},
			{&PPI,"ppi"},
			{&IsAutosaveEnabled,"isAutosaveEnabled"},
			{&AutosavePeriodMinutes,"autosavePeriodMinutes"},

			{&UseDiscreteMovement,"useDiscreteMovement"},
			{&TranslationStep,"translationStep"},
			{&RotationStep,"rotationStep"},
			{&ScalingStep,"scalingStep"},
			{&MouseSensivity,"mouseSensivity"},

			{&RotationStep,"rotationStep"},
			{&ScalingStep,"scalingStep"},

			{&ColorLeft,"colorLeft"},
			{&ColorRight,"colorRight"},
			{&DimmedColorLeft,"dimmedColorLeft"},
			{&DimmedColorRight,"dimmedColorRight"},
			{&CustomRenderWindowAlpha,"customRenderWindowAlpha"},

			{&ShouldMoveCrossOnCosinePenModeChange,"shouldMoveCrossOnCosinePenModeChange"},

			{&PointRadiusPixel,"pointRadiusPixel"},
			{&LineThickness,"lineThickness"},

			{&CosinePointCount,"cosinePointCount"},

			{&CameraResolution,"cameraResolution"},
			{&CameraViewAngles,"cameraViewAngles"},
			{&CameraAngle,"cameraAngle"},
			{&FaceSizeYMillimeters,"faceSizeYMillimeters"},
			{&ScreenCenterToCameraDistanceMillimeters,"screenCenterToCameraDistanceMillimeters"},
		};

		if (auto a = v.find(reference); a != v.end())
			return a._Ptr->_Myval.second;
		
		throw new std::exception("Name for a reference was not found.");
	}

};

struct ReadOnlyState {
	StaticProperty(glm::vec2, ViewSize)
};
