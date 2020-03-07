#pragma once

#include "DomainTypes.hpp"

#include <iostream>


class obstream {
	std::vector<char> buffer;
public:
	template<typename T>
	void put(const T& val) {
		for (size_t i = 0; i < sizeof(T); i++)
			buffer.push_back(((char*)&val)[i]);
	}

	template<>
	void put<std::string>(const std::string& val) {
		for (size_t i = 0; i < val.size(); i++)
			buffer.push_back(val[i]);
	}

	template<>
	void put<SceneObject>(const SceneObject& so) {

		switch (so.GetType())
		{
		case Group:
		{
			auto o = (GroupObject*)&so;

			put(so.GetType());
			put(o->Name.size());
			put(o->Name);
			put(o->Children.size());

			for (auto c : o->Children)
				put(*c);

			break;
		}
		case StereoPolyLineT:
		{
			auto o = (StereoPolyLine*)&so;

			put(so.GetType());
			put(o->Name.size());
			put(o->Name);
			put(o->Points.size());

			for (auto p : o->Points)
				put(p);

			break;
		}
		case StereoLineT:
		{
			auto o = (StereoLine*)&so;

			put(so.GetType());
			put(o->Name.size());
			put(o->Name);

			put(o->Start);
			put(o->End);

			break;
		}
		default:
			break;
		}
	}

	const char* getBuffer() {
		return buffer.data();
	}

	size_t getSize() {
		return buffer.size();
	}
};

class ibstream {
	char* buffer = nullptr;
	size_t pos = 0;
public:
	Scene* scene;

	template<typename T>
	T get(size_t size = sizeof(T)) {
		T val;

		for (size_t i = 0; i < size; i++, pos++)
			((char*)&val)[i] = buffer[pos];

		return val;
	}

	template<>
	std::string get<std::string>(size_t size) {
		std::string val;

		for (size_t i = 0; i < size; i++, pos++)
			val += buffer[pos];

		return val;
	}

	template<>
	SceneObject* get<SceneObject*>(size_t _) {
		auto type = get<ObjectType>();

		switch (type)
		{
		case Group:
		{
			auto o = new GroupObject();
			scene->Insert(o);

			auto nameSize = get<size_t>();
			o->Name = get<std::string>(nameSize);

			auto childCount = get<size_t>();

			for (size_t i = 0; i < childCount; i++)
				o->Children.push_back(get<SceneObject*>());

			return o;
		}
		case StereoPolyLineT:
		{
			auto o = new StereoPolyLine();
			scene->Insert(o);

			auto nameSize = get<size_t>();
			o->Name = get<std::string>(nameSize);
			
			auto pointCount = get<size_t>();
			for (size_t i = 0; i < pointCount; i++)
			{
				o->Points.push_back(get<glm::vec3>());
			}

			return o;
		}
		case StereoLineT:
		{
			auto o = new StereoLine();
			scene->Insert(o);

			auto nameSize = get<size_t>();
			o->Name = get<std::string>(nameSize);

			o->Start = get<glm::vec3>();
			o->End = get<glm::vec3>();

			return o;
		}
		default:
			return nullptr;
		}
	}

	ibstream& setBuffer(char* buf){
		buffer = buf;
		pos = 0;

		return *this;
	}
};




std::ofstream& operator<<(std::ofstream& os, glm::vec3& dt)
{
	os << dt.x << dt.y << dt.z;

	return os;
}

std::ofstream& operator<<(std::ofstream& os, SceneObject* dt)
{
	switch (dt->GetType())
	{
	case Group:
	{
		auto o = (GroupObject*)dt;

		os << o->Children.size();

		for (auto c : o->Children)
			os << c;

		break;
	}
	case StereoPolyLineT:
	{
		auto o = (StereoPolyLine*)dt;

		os << o->Name << o->Points.size();

		for (auto p : o->Points)
			os << p;

		break;
	}
	default:
		break;
	}

	return os;
}


std::ifstream& operator>>(std::ifstream& is, glm::vec3& dt)
{
	is >> dt.x >> dt.y >> dt.z;

	return is;
}

std::ifstream& operator>>(std::ifstream& is, SceneObject* dt)
{
	switch (dt->GetType())
	{
	case Group:
	{
		for (auto o : ((GroupObject*)dt)->Children)
			is >> o;

		break;
	}
	case StereoPolyLineT:
	{
		auto o = (StereoPolyLine*)dt;
		int pointCount;
		is >> o->Name >> pointCount;

		for (size_t i = 0; i < pointCount; i++)
		{
			glm::vec3 v;
			is >> v;
			o->Points.push_back(v);
		}

		break;
	}
	default:
		break;
	}

	return is;
}

class FileManager {
public:

	//static bool Load(std::string filename, Scene* outScene) {
	//	std::ifstream in(filename, std::ios::binary, std::ios::in);

	//	in >> outScene->root;

	//	return true;
	//}

	//static bool SaveJson(std::string filename, Scene* inScene) {
	//	std::ofstream out(filename, std::ios::binary | std::ios::out);

	//	out << inScene->root;

	//	return true;
	//}

	//static stringstream& Serialize(stringstream& ss, glm::vec3& obj) {
	//	ss << obj.x << obj.y << obj.z;

	//	return ss;
	//}

	//static stringstream& Serialize(stringstream& ss, SceneObject* obj) {
	//	switch (obj->GetType())
	//	{
	//	case Group:
	//	{
	//		auto o = (GroupObject*)obj;

	//		ss << o->Children.size();

	//		for (auto c : o->Children)
	//			Serialize(ss, c);

	//		break;
	//	}
	//	case StereoPolyLineT:
	//	{
	//		auto o = (StereoPolyLine*)obj;

	//		ss << o->Name.size() << o->Name << o->Points.size();

	//		for (auto p : o->Points)
	//			Serialize(ss, p);

	//		break;
	//	}
	//	default:
	//		break;
	//	}

	//	return ss;
	//}



	//static binarystream& Serialize(binarystream& ss, SceneObject* obj) {
	//	switch (obj->GetType())
	//	{
	//	case Group:
	//	{
	//		auto o = (GroupObject*)obj;

	//		ss << o->Children.size();

	//		for (auto c : o->Children)
	//			Serialize(ss, c);

	//		break;
	//	}
	//	case StereoPolyLineT:
	//	{
	//		auto o = (StereoPolyLine*)obj;

	//		ss << o->Name.size() << o->Name << o->Points.size();

	//		for (auto p : o->Points)
	//			Serialize(ss, p);

	//		break;
	//	}
	//	default:
	//		break;
	//	}

	//	return ss;
	//}

	static size_t GetFileSize(std::string filename) {
		std::ifstream in(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (in)
			return in.tellg();

		return 0;
	}

	static bool LoadBinary(std::string filename, Scene* inScene) {
		std::ifstream file(filename, std::ios::binary | std::ios::in);

		auto bufferSize = GetFileSize(filename);

		auto buffer = new char[bufferSize];
		file.read(buffer, bufferSize);

		ibstream str;
		str.setBuffer(buffer);
		str.scene = inScene;

		auto o = str.get<SceneObject*>();
		inScene->root = (GroupObject*)o;

		file.close();

		return true;
	}

	static bool SaveBinary(std::string filename, Scene* inScene) {
		std::ofstream file(filename, std::ios::binary | std::ios::out);

		auto bs = obstream();
		bs.put(*(SceneObject*)inScene->root);

		file.write(bs.getBuffer(), bs.getSize());

		file.close();

		return true;
	}
};