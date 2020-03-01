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
			auto size = ((StereoPolyLine*)obj)->Points.size();
			return size > 0 ? size - 1 : 0;
		}
		case MeshT:
		case LineMeshT:
		{
			auto size = ((LineMesh*)obj)->lines.size();
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

			for (size_t i = 0; i < polyLine->Points.size() - 1; i++)
			{
				objs[i].Start = polyLine->Points[i];
				objs[i].End = polyLine->Points[i + 1];
			}
			break;
		}
		case MeshT:
		case LineMeshT:
			auto lineMesh = (LineMesh*)obj;

			for (size_t i = 0; i < lineMesh->lines.size(); i++)
			{
				objs[i].Start = (*lineMesh->GetVertices())[lineMesh->lines[i][0]];
				objs[i].End = (*lineMesh->GetVertices())[lineMesh->lines[i][1]];
			}
			break;
		}
	}


};

//class Convert {
//public:
//	template<typename T>
//	static FreeMesh* ToMesh(SceneObject* obj) {
//	}
//
//	template<>
//	static FreeMesh* ToMesh<StereoPolyLine>(SceneObject* obj) {
//		auto o = (StereoPolyLine*)obj;
//		auto n = new FreeMesh();
//
//		n->vertices = std::vector<glm::vec3>(o->Points);
//		
//		for (size_t i = 0; i < o->Points.size() - 1; i++)
//		{
//			n->lines.push_back({ i, i + 1 });
//		}
//
//		return n;
//	}
//
//};
