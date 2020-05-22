#pragma once

#include "DomainTypes.hpp"
#include <string>
#include <iostream>


class FileException : public std::exception {
public:
	FileException(const char* message) : std::exception(message) {

	}
};

namespace FileType {
	const std::string Json = "json";
	const std::string So2 = "so2";
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
			put(o->children.size());

			for (auto c : o->children)
				put(*c);

			break;
		}
		case StereoPolyLineT:
		{
			auto o = (StereoPolyLine*)&so;

			put(so.GetType());
			put(o->Name.size());
			put(o->Name);
			put(o->GetVertices().size());

			for (auto p : o->GetVertices())
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
		case MeshT:
		{
			auto o = (LineMesh*)&so;

			put(so.GetType());
			put(o->Name.size());
			put(o->Name);

			put(o->GetVertices()->size());
			for (auto p : *o->GetVertices())
				put(p);

			put(o->GetLinearConnections().size());
			for (auto c : o->GetLinearConnections())
				for (auto p : c)
					put(p);

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
	const Log log = Log::For<ibstream>();
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
				o->children.push_back(get<SceneObject*>());

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
				o->AddVertice(get<glm::vec3>());

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
		case MeshT:
		{
			auto o = new LineMesh();
			scene->Insert(o);

			auto nameSize = get<size_t>();
			o->Name = get<std::string>(nameSize);

			auto verticeCount = get<size_t>();
			for (size_t i = 0; i < verticeCount; i++)
				o->AddVertice(get<glm::vec3>());

			auto linearConnectionCount = get<size_t>();
			for (size_t i = 0; i < linearConnectionCount; i++)
				o->Connect(get<size_t>(), get<size_t>());

			return o;
		}
		default:
			log.Error("Unsupported Scene Object Type found while reading file.");
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
	void put<std::array<size_t, 2>>(const std::array<size_t, 2>& val) {
		buffer << "{\"a\":" << val[0] << ",\"b\":" << val[1] << '}';
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
			if (o->children.size() > 0)
				put(*o->children[0]);
			for (size_t i = 1; i < o->children.size(); i++)
			{
				buffer << ',';
				put(*o->children[i]);
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
			if(o->GetVertices().size() > 0)
				put(o->GetVertices()[0]);
			for (size_t i = 1; i < o->GetVertices().size(); i++)
			{
				buffer << ',';
				put(o->GetVertices()[i]);
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
		case MeshT:
		{
			auto o = (LineMesh*)&so;

			buffer << "\"type\":";
			put(so.GetType());

			buffer << ",\"name\":";
			put(o->Name);

			buffer << ",\"vertices\":[";
			put((*o->GetVertices())[0]);
			for (size_t i = 1; i < o->GetVertices()->size(); i++)
			{
				buffer << ',';
				put((*o->GetVertices())[i]);
			}
			buffer << ']';

			buffer << ",\"linearConnections\":[";
			put(o->GetLinearConnections()[0]);
			for (size_t i = 1; i < o->GetLinearConnections().size(); i++)
			{
				buffer << ',';
				put(o->GetLinearConnections()[i]);
			}
			buffer << ']';

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

	bool isArrayEmpty() {
		bool isEmpty = false;

		skip();//[
		if (buffer.get() == ']')
			isEmpty = true;

		buffer.seekg(-2, std::ios_base::cur);

		return isEmpty;
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
	std::array<size_t, 2> get<std::array<size_t, 2>>() {
		std::array<size_t, 2> val;

		skip();//{
		skipName();
		buffer >> val[0];
		skip();//,
		skipName();
		buffer >> val[1];
		skip();//}

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
			if (!isArrayEmpty())
				while (buffer.get() != ']')//[]
					o->children.push_back(get<SceneObject*>());
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
			if (!isArrayEmpty())
				while (buffer.get() != ']')//[]
					o->AddVertice(get<glm::vec3>());
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
		case MeshT:
		{
			auto o = new LineMesh();
			scene->Insert(o);

			skipName();
			o->Name = get<std::string>();
			skip();//,

			skipName();
			if (!isArrayEmpty())
				while (buffer.get() != ']')//[]
					o->AddVertice(get<glm::vec3>());

			skipName();
			if (!isArrayEmpty())
				while (buffer.get() != ']') {//[]
					auto connection = get<std::array<size_t, 2>>();
					o->Connect(connection[0], connection[1]);
				}

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
	static Log& GetLog() {
		static Log log = Log::For<FileManager>();
		return log;
	}

	static size_t GetFileSize(std::string filename) {
		std::ifstream in(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (in)
			return in.tellg();

		return 0;
	}

	static void Fail(const char* msg) {
		GetLog().Error(msg);
		throw new FileException(msg);
	}

	static void SaveJson(std::string filename, Scene* inScene) {
		if (inScene == nullptr)
			Fail("InScene was null");
		if (inScene->root == nullptr)
			Fail("InScene Root was null");

		std::ofstream out(filename);

		ojstream bs;

		bs.put(*(SceneObject*)inScene->root);

		out << bs.getBuffer();

		out.close();
	}

	static void LoadJson(std::string filename, Scene* inScene) {
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
	}


	static void SaveBinary(std::string filename, Scene* inScene) {
		std::ofstream file(filename, std::ios::binary | std::ios::out);

		auto bs = obstream();
		bs.put(*(SceneObject*)inScene->root);

		file.write(bs.getBuffer(), bs.getSize());

		file.close();
	}

	static void LoadBinary(std::string filename, Scene* inScene) {
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
	}

	static std::string GetFixedExtension(std::string& filename) {
		int dotPosition = filename.find_last_of('.');

		if (dotPosition < 0) {
			if (GetDefaultFileExtension().empty())
				Fail("File extension empty");
			else {
				dotPosition = filename.size();
				filename += '.' + GetDefaultFileExtension();
			}
		}

		auto extension = filename.substr(dotPosition + 1);

		if (extension.empty()) {
			if (GetDefaultFileExtension().empty())
				Fail("File extension empty");
			else {
				filename += GetDefaultFileExtension();
			}
		}

		return extension;
	}
public:

	static std::string& GetDefaultFileExtension() {
		static auto val = FileType::So2;
		return val;
	}

	static void Load(std::string filename, Scene* inScene) {
		auto extension = GetFixedExtension(filename);

		if (extension == FileType::Json)
			LoadJson(filename, inScene);
		else if (extension == FileType::So2)
			LoadBinary(filename, inScene);
		else
			Fail("File extension not supported");
	}

	static void Save(std::string filename, Scene* inScene) {
		auto extension = GetFixedExtension(filename);

		if (extension == FileType::Json)
			SaveJson(filename, inScene);
		else if (extension == FileType::So2)
			SaveBinary(filename, inScene);
		else
			Fail("File extension not supported");
	}


};