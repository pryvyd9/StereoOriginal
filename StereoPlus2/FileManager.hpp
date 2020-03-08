#pragma once

#include "DomainTypes.hpp"
#include <string>
#include <iostream>


namespace FileType {
	const std::string Json = "json";
	const std::string Sp2 = "sp2";
};

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
			throw std::exception("Unsupported Scene Object Type found while writing file.");
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
				o->Points.push_back(get<glm::vec3>());

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
			throw std::exception("Unsupported Scene Object Type found while reading file.");
		}
	}

	ibstream& setBuffer(char* buf){
		buffer = buf;
		pos = 0;

		return *this;
	}
};

class ojstream {
	std::stringstream buffer;
public:
	template<typename T>
	void put(const T& val) {
		throw std::exception("Unsupported Type found while writing file.");
	}


	template<>
	void put<size_t>(const size_t& val) {
		buffer << val;
	}
	template<>
	void put<ObjectType>(const ObjectType& val) {
		buffer << val;
	}
	template<>
	void put<glm::vec3>(const glm::vec3& val) {
		buffer << '[' << val.x << ',' << val.y << ',' << val.z << ']';
	}

	template<>
	void put<std::string>(const std::string& val) {
		buffer << '"' << val << '"';
	}

	template<>
	void put<SceneObject>(const SceneObject& so) {
		buffer << '{';

		switch (so.GetType())
		{
		case Group:
		{
			auto o = (GroupObject*)&so;
			
			buffer << "\"type\":";
			put(so.GetType());

			buffer << ",\"name\":";
			put(o->Name);
			
			buffer << ",\"children\":[";
			put(*o->Children[0]);
			for (size_t i = 1; i < o->Children.size(); i++)
			{
				buffer << ',';
				put(*o->Children[i]);
			}
			buffer << ']';

			break;
		}
		case StereoPolyLineT:
		{
			auto o = (StereoPolyLine*)&so;

			buffer << "\"type\":";
			put(so.GetType());

			buffer << ",\"name\":";
			put(o->Name);

			buffer << ",\"points\":[";
			put(o->Points[0]);
			for (size_t i = 1; i < o->Points.size(); i++)
			{
				buffer << ',';
				put(o->Points[i]);
			}
			buffer << ']';

			break;
		}
		case StereoLineT:
		{
			auto o = (StereoLine*)&so;

			buffer << "\"type\":";
			put(so.GetType());

			buffer << ",\"name\":";
			put(o->Name);

			buffer << ",\"start\":";
			put(o->Start);

			buffer << ",\"end\":";
			put(o->End);

			break;
		}
		default:
			throw std::exception("Unsupported Scene Object Type found while writing file.");
		}

		buffer << '}';
	}

	std::string getBuffer() {
		return buffer.str();
	}
};

class ijstream {
	std::istringstream buffer;

	void skipName() {
		while (buffer.get() != ':');
	}
	void skip() {
		char a = buffer.get();
	}
public:
	Scene* scene;

	template<typename T>
	T get() {
		throw std::exception("Unsupported Type found while writing file.");
	}

	template<>
	ObjectType get<ObjectType>() {
		ObjectType val;
		buffer >> *(int*)&val;
		return (ObjectType)val;
	}

	template<>
	glm::vec3 get<glm::vec3>() {
		glm::vec3 val;


		skip();//[

		buffer >> val.x;
		skip();//,

		buffer >> val.y;
		skip();//,

		buffer >> val.z;
		skip();//]

		return val;
	}

	template<>
	std::string get<std::string>() {
		std::string val;
		char c;

		skip();//"
		while ((c = buffer.get()) != '"')
			val += c;

		return val;
	}


	template<>
	SceneObject* get<SceneObject*>() {
		skip();//{
		skipName();
		auto type = get<ObjectType>();
		skip();//,

		switch (type)
		{
		case Group:
		{
			auto o = new GroupObject();
			scene->Insert(o);

			skipName();
			o->Name = get<std::string>();
			skip();//,

			skipName();
			while (buffer.get() != ']')//[]
				o->Children.push_back(get<SceneObject*>());

			skip();//}

			return o;
		}
		case StereoPolyLineT:
		{
			auto o = new StereoPolyLine();
			scene->Insert(o);

			skipName();
			o->Name = get<std::string>();
			skip();//,

			skipName();
			while (buffer.get() != ']')//[]
				o->Points.push_back(get<glm::vec3>());

			skip();//}

			return o;
		}
		case StereoLineT:
		{
			auto o = new StereoLine();
			scene->Insert(o);

			skipName();
			o->Name = get<std::string>();
			skip();//,

			skipName();
			o->Start = get<glm::vec3>();
			skip();//,

			skipName();
			o->End = get<glm::vec3>();

			skip();//}

			return o;
		}
		default:
			throw std::exception("Unsupported Scene Object Type found while reading file.");
		}
	}


	void setBuffer(char* buf) {
		buffer = std::istringstream(buf);
	}

};


class FileManager {
	static size_t GetFileSize(std::string filename) {
		std::ifstream in(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (in)
			return in.tellg();

		return 0;
	}

public:
	static bool SaveJson(std::string filename, Scene* inScene) {
		std::ofstream out(filename);

		ojstream bs;

		bs.put(*(SceneObject*)inScene->root);

		out << bs.getBuffer();

		out.close();

		return true;
	}

	static bool LoadJson(std::string filename, Scene* inScene) {
		std::ifstream file(filename, std::ios::binary | std::ios::in);

		auto bufferSize = GetFileSize(filename);

		auto buffer = new char[bufferSize];
		file.read(buffer, bufferSize);

		ijstream str;
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

	//static bool Load(std::string filename, Scene* inScene, std::string type) {
	//	if (type == FileType::Json)
	//	{
	//		return LoadJson(
	//	}
	//}
};