#pragma once
#include "DomainTypes.hpp"



struct SceneConfiguration {
	float whiteZ = 0;
	float whiteZPrecision = 0.1;

	StereoCamera camera;
	GLFWwindow* window;
};