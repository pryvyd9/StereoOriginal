#pragma once

#include "DomainUtils.hpp"
#include <string>
#include <iostream>
#include "Json.hpp"

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
	template<typename TString>
	void put(const TString& val) {
		for (size_t i = 0; i < sizeof(TString); i++)
			buffer.push_back(((char*)&val)[i]);
	}
	template<>
	void put<std::string>(const std::string& val) {
		put(val.size());
		for (size_t i = 0; i < val.size(); i++)
			buffer.push_back(val[i]);
	}
	template<>
	void put<SceneObject>(const SceneObject& so) {
		if (so.GetType() == CrossT)
			return;

		put(so.GetType());
		put(so.Name);
		put(so.GetLocalPosition());
		put(so.GetLocalRotation());

		switch (so.GetType())
		{
		case Group:
			break;
		case StereoPolyLineT:
		{
			auto o = (StereoPolyLine*)&so;

			put(o->GetVertices().size());
			for (auto p : o->GetVertices())
				put(p);

			break;
		}
		case MeshT:
		{
			auto o = (Mesh*)&so;

			put(o->GetVertices().size());
			for (auto p : o->GetVertices())
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

		put(so.children.size());
		for (auto c : so.children)
			put(*c);
	}

	const char* getBuffer() {
		return buffer.data();
	}

	size_t getSize() {
		return buffer.size();
	}
};

class ibstream {
	bool isRoot = true;
	const Log log = Log::For<ibstream>();
	char* buffer = nullptr;
	size_t pos = 0;

	template<typename T>
	void read(T* dest) {
		*dest = get<T>();
	}
	template<typename T>
	void read(std::function<void(T)> f) {
		f(get<T>());
	}
	template<>
	void read(std::string* dest) {
		auto size = get<size_t>();
		*dest = get<std::string>(size);
	}
	void readChildren(SceneObject* parent) {
		readArray(std::function([&parent](SceneObject* v) {
			v->SetParent(parent);
			}));
	}
	template<typename...T>
	void readArray(std::function<void(T...)> f) {
		auto count = get<size_t>();
		for (size_t i = 0; i < count; i++)
			f(get<T>()...);
	}

	template<typename T>
	T* start() {
		auto o = new T();
		if (isRoot)
			isRoot = false;
		else
			objects.push_back(o);
		return o;
	}
public:
	std::vector<SceneObject*> objects;

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
			auto o = start<GroupObject>();
			read(&o->Name);
			read(std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
			read(std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
			readChildren(o);
			return o;
		}
		case StereoPolyLineT:
		{
			auto o = start<StereoPolyLine>();
			read(&o->Name);
			read(std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
			read(std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
			readArray(std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
			readChildren(o);
			return o;
		}
		case MeshT:
		{
			auto o = start<Mesh>();
			read(&o->Name);
			read(std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
			read(std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
			readArray(std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
			readArray(std::function([&o](GLuint a, GLuint b) { o->Connect(a, b); }));
			readChildren(o);
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

class JsonConvert {

	static bool& isRoot() {
		static bool v;
		return v;
	}

	template<typename T>
	static T* start() {
		auto t = new T();
		if (isRoot())
			isRoot() = false;
		else
			objects().push_back(t);
		return t;
	}

	template<typename T>
	static void get(Js::Object* joa, std::string name, T& dest) {
		dest = get<T>(joa->objects[name]);
	}
	template<typename T>
	static void get(Js::Object* joa, std::string name, std::function<void(T)> dest) {
		dest(get<T>(joa->objects[name]));
	}
	template<typename...T>
	static void getArray(Js::Object* jo, std::string name, std::function<void(T...)> f) {
		auto j = (Js::Array*)jo->objects[name];
		for (auto o : j->objects)
			f(get<T>(o)...);
	}
	static void getArray(Js::Object* jo, std::string name, std::function<void(size_t, size_t)> f) {
		auto j = (Js::Array*)jo->objects[name];
		for (auto o : j->objects) {
			auto v = get<size_t, 2>(o);
			f(v[0], v[1]);
		}
	}

	static void getChildren(Js::Object* j, std::string name, SceneObject* parent) {
		getArray(j, name, std::function([&parent](SceneObject* v) {
			v->SetParent(parent);
			}));
	}


	static std::stringstream& buffer() {
		static std::stringstream v;
		return v;
	}

	template<typename T>
	static std::string toString(const T& t) {
		std::stringstream temp;
		temp << t;
		return temp.str();
	}

	template<typename T>
	static void insert(Js::Object* jo, std::string name, const T& v) {
		jo->objects.insert({ name, serialize(v) });
	}


public:
	static std::vector<SceneObject*>& objects() {
		static std::vector<SceneObject*> v;
		return v;
	}

	template<typename T>
	static T get(std::string str) {
		std::stringstream ss;
		ss << str;
		T val;
		ss >> val;
		return val;
	}
	template<>
	static ObjectType get(std::string str) {
		return (ObjectType)get<int>(str);
	}

	template<typename T>
	static T get(Js::ObjectAbstract* joa) {
		auto j = (Js::Primitive*)joa;
		return get<T>(j->value);
	}
	template<>
	static std::string get(Js::ObjectAbstract* joa) {
		auto j = (Js::PrimitiveString*)joa;
		return get<std::string>(j->value);
	}
	template<>
	static glm::vec3 get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		glm::vec3 v;
		for (size_t i = 0; i < j->objects.size(); i++)
			((float*)&v)[i] = get<float>(j->objects[i]);
		return v;
	}
	template<>
	static glm::fquat get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		glm::fquat v;
		for (size_t i = 0; i < j->objects.size(); i++)
			((float*)&v)[i] = get<float>(j->objects[i]);
		return v;
	}
	template<>
	static std::vector<SceneObject*> get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		std::vector<SceneObject*> v;
		for (auto p : j->objects)
			v.push_back(get<SceneObject*>(p));
		return v;
	}
	template<typename T, size_t S>
	static std::array<T, S> get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		std::array<T, S> v;
		for (size_t i = 0; i < S; i++) {
			v[i] = get<T>(j->objects[i]);
		}
		return v;
	}
	template<>
	static SceneObject* get(Js::ObjectAbstract* joa) {
		auto j = (Js::Object*)joa;
		auto type = get<ObjectType>(j->objects["type"]);
		switch (type) {
		case Group:
		{
			auto o = start<GroupObject>();
			get(j, "name", o->Name);
			get(j, "localPosition", std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
			get(j, "localRotation", std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
			getChildren(j, "children", o);
			return o;
		}
		case StereoPolyLineT:
		{
			auto o = start<StereoPolyLine>();
			get(j, "name", o->Name);
			get(j, "localPosition", std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
			get(j, "localRotation", std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
			getChildren(j, "children", o);
			getArray(j, "vertices", std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
			return o;
		}
		case MeshT:
		{
			auto o = start<Mesh>();
			get(j, "name", o->Name);
			get(j, "localPosition", std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
			get(j, "localRotation", std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
			getChildren(j, "children", o);
			getArray(j, "vertices", std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
			getArray(j, "connections", std::function([&o](size_t a, size_t b) { o->Connect(a, b); }));
			return o;
		}
		case CameraT:
			break;
		case CrossT:
			break;
		}

		throw std::exception("Unsupported Scene Object Type found while reading file.");
	}

	template<typename T>
	static Js::ObjectAbstract* serialize(const T& o) {
		auto j = new Js::Primitive();
		j->value = toString(o);
		return j;
	}
	template<>
	static Js::ObjectAbstract* serialize(const std::string& o) {
		auto j = new Js::PrimitiveString();
		j->value = o;
		return j;
	}
	template<>
	static Js::ObjectAbstract* serialize(const glm::vec3& o) {
		auto j = new Js::Array();
		j->objects = { serialize(o.x), serialize(o.y), serialize(o.z) };
		return j;
	}
	template<>
	static Js::ObjectAbstract* serialize(const glm::fquat& o) {
		auto j = new Js::Array();
		j->objects = { serialize(o.x), serialize(o.y), serialize(o.z), serialize(o.w) };
		return j;
	}
	template<typename T>
	static Js::ObjectAbstract* serialize(const std::vector<T*>& v) {
		auto j = new Js::Array();
		for (auto a : v)
			if (auto r = serialize(*a); r != nullptr)
				j->objects.push_back(r);
		return j;
	}
	template<typename T>
	static Js::ObjectAbstract* serialize(const std::vector<T>& v) {
		auto j = new Js::Array();
		for (auto a : v)
			j->objects.push_back(serialize(a));
		return j;
	}
	template<typename T, GLuint S>
	static Js::ObjectAbstract* serialize(const std::array<T, S>& v) {
		auto j = new Js::Array();
		for (auto a : v)
			j->objects.push_back(serialize(a));
		return j;
	}
	template<>
	static Js::ObjectAbstract* serialize(const SceneObject& so) {
		if (so.GetType() == CrossT)
			return nullptr;

		auto jo = new Js::Object();
		insert(jo, "type", so.GetType());
		insert(jo, "name", so.Name);
		insert(jo, "localPosition", so.GetLocalPosition());
		insert(jo, "localRotation", so.GetLocalRotation());
		insert(jo, "children", so.children);

		switch (so.GetType()) {
		case Group:
			break;
		case StereoPolyLineT:
		{
			auto o = (StereoPolyLine*)&so;
			insert(jo, "vertices", o->GetVertices());
			break;
		}
		case MeshT:
		{
			auto o = (Mesh*)&so;
			insert(jo, "vertices", o->GetVertices());
			insert(jo, "connections", o->GetLinearConnections());
			break;
		}
		}

		return jo;
	}



	static void Reset() {
		isRoot() = true;
		objects().clear(); 
		buffer().clear();
	}

	static std::string getBuffer() {
		return buffer().str();
	}
};

class FileManager {
	// How to check if file is Unicode
	// Encoding     BOM (hex)     BOM (dec)
	// ----------------------------------
	// UTF - 8      EF BB BF      239 187 191
	// UTF - 16     (BE)FE FF     254 255
	// UTF - 16     (LE)FF FE     255 254

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
		if (!inScene->root.Get().HasValue())
			Fail("InScene Root was null");

		auto json = JsonConvert::serialize(*inScene->root.Get().Get());
		Json::Write(filename, json);
		delete json;
	}
	static void LoadJson(std::string filename, Scene* inScene) {
		auto json = Json::Read(filename);
		JsonConvert::Reset();
		auto root = JsonConvert::get<SceneObject*>(json);
		
		inScene->root = root;

		std::vector<PON> newObjects;
		for (auto o : JsonConvert::objects())
			newObjects.push_back(o);

		inScene->objects = newObjects;
	}

	static void SaveBinary(std::string filename, Scene* inScene) {
		std::ofstream file(filename, std::ios::binary | std::ios::out);

		auto bs = obstream();
		bs.put(*inScene->root.Get().Get());

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

		auto o = str.get<SceneObject*>();
		inScene->root = o;

		std::vector<PON> newObjects;
		for (auto o : str.objects)
			newObjects.push_back(o);

		inScene->objects = newObjects;

		delete[] buffer;
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

	static Jw::ObjectAbstract* LoadLocaleFile(const std::string& filename) {
		return Json::ReadW(filename);
	}
};