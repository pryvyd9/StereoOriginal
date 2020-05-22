#pragma once

#include "DomainTypes.hpp"
#include<array>

class LineConverter {
public:
	static size_t GetLineCount(SceneObject* obj) {
		switch (obj->GetType()) {
		case StereoLineT:
			return 1;
		case StereoPolyLineT:
		{
			auto size = ((StereoPolyLine*)obj)->GetVertices().size();
			return size > 0 ? size - 1 : 0;
		}
		case MeshT:
		{
			auto size = ((Mesh*)obj)->lines.size();
			return size > 0 ? size : 0;
		}
		default:
			return 0;
		}
	}

	static void Convert(SceneObject* obj, StereoLine* objs) {
		switch (obj->GetType()) {
		case StereoLineT:
			*objs = *((StereoLine*)obj);
			break;
		case StereoPolyLineT:
		{
			auto polyLine = (StereoPolyLine*)obj;

			for (size_t i = 0; i < polyLine->GetVertices().size() - 1; i++)
			{
				objs[i].Start = polyLine->GetVertices()[i];
				objs[i].End = polyLine->GetVertices()[i + 1];
			}
			break;
		}
		case MeshT:
			auto lineMesh = (Mesh*)obj;

			for (size_t i = 0; i < lineMesh->lines.size(); i++)
			{
				objs[i].Start = lineMesh->GetVertices()[lineMesh->lines[i][0]];
				objs[i].End = lineMesh->GetVertices()[lineMesh->lines[i][1]];
			}
			break;
		}
	}


};
