#ifndef DOMAIN_FUNCTIONS

#define DOMAIN_FUNCTIONS

#include "Infrastructure.h"
#include "DomainTypes.hpp"
#include "PersistanceInterfaces.hpp"

float* GetVertices(Line line) {
	return (float*)&line;
}

void GetVertices(Line line, float vertices[6]) {
	vertices[0] = line.Start.x;
	vertices[1] = line.Start.y;
	vertices[2] = line.Start.z;
	vertices[3] = line.End.x;
	vertices[4] = line.End.y;
	vertices[5] = line.End.z;
}

#endif // !DOMAIN_FUNCTIONS
