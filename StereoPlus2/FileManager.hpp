#pragma once

#include "DomainTypes.hpp"
#include <string>
#include <iostream>
#include "TemplateExtensions.hpp"

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

			put(o->children.size());
			for (auto c : o->children)
				put(*c);

			break;
		}
		case MeshT:
		{
			auto o = (Mesh*)&so;

			put(so.GetType());
			put(o->Name.size());
			put(o->Name);

			put(o->GetVertices().size());
			for (auto p : o->GetVertices())
				put(p);

			put(o->GetLinearConnections().size());
			for (auto c : o->GetLinearConnections())
				for (auto p : c)
					put(p);

			put(o->children.size());
			for (auto c : o->children)
				put(*c);

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

	template<typename T>
	void read(T* dest) {
		auto size = get<size_t>();
		*dest = get<T>(size);
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
		scene->Insert((SceneObject*)o);
		return o;
	}
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
			auto o = start<GroupObject>();
			read(&o->Name);
			readChildren(o);
			return o;
		}
		case StereoPolyLineT:
		{
			auto o = start<StereoPolyLine>();
			read(&o->Name);
			readArray(std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
			readChildren(o);
			return o;
		}
		case MeshT:
		{
			auto o = start<Mesh>();
			read(&o->Name);
			readArray(std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
			readArray(std::function([&o](size_t a, size_t b) { o->Connect(a, b); }));
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

enum JType {
	JObject,
	JArray,
	JPrimitive,
	JPrimitiveString,
};
struct JsonObjectAbstract {
	virtual JType GetType() const = 0;
};
struct JsonObject : JsonObjectAbstract {
	std::unordered_map<std::string, JsonObjectAbstract*> objects;
	
	virtual JType GetType() const {
		return JObject;
	}
	~JsonObject() {
		for (auto o : objects) {
			delete o.second;
		}
	}
};
struct JsonArray : JsonObjectAbstract {
	std::vector<JsonObjectAbstract*> objects;

	virtual JType GetType() const {
		return JArray;
	}

	~JsonArray() {
		for (auto o : objects) {
			delete o;
		}
	}
};
struct JsonPrimitive : JsonObjectAbstract {
	std::string value;

	virtual JType GetType() const {
		return JPrimitive;
	}
};
struct JsonPrimitiveString : JsonObjectAbstract {
	std::string value;

	virtual JType GetType() const {
		return JPrimitiveString;
	}
};


class ojstream {
	std::stringstream buffer;

	template<typename T>
	static std::string toString(const T& t) {
		std::stringstream temp;
		temp << t;
		return temp.str();
	}

	template<typename T>
	void insert(JsonObject* jo, std::string name, const T& v) {
		jo->objects.insert({ name, serialize(v) });
	}
public:
	template<typename T>
	JsonObjectAbstract* serialize(const T& o) {
		auto j = new JsonPrimitive();
		j->value = toString(o);
		return j;
	}
	template<>
	JsonObjectAbstract* serialize(const std::string& o) {
		auto j = new JsonPrimitiveString();
		j->value = o;
		return j;
	}
	template<>
	JsonObjectAbstract* serialize(const glm::vec3& o) {
		auto j = new JsonArray();
		j->objects = { serialize(o.x), serialize(o.y), serialize(o.z) };
		return j;
	}
	template<>
	JsonObjectAbstract* serialize(const glm::fquat& o) {
		auto j = new JsonArray();
		j->objects = { serialize(o.x), serialize(o.y), serialize(o.z), serialize(o.w) };
		return j;
	}
	template<typename T>
	JsonObjectAbstract* serialize(const std::vector<T*>& v) {
		auto j = new JsonArray();
		for (auto a : v)
			j->objects.push_back(serialize(*a));
		return j;
	}
	template<typename T>
	JsonObjectAbstract* serialize(const std::vector<T>& v) {
		auto j = new JsonArray();
		for (auto a : v)
			j->objects.push_back(serialize(a));
		return j;
	}
	template<typename T, size_t S>
	JsonObjectAbstract* serialize(const std::array<T, S>& v) {
		auto j = new JsonArray();
		for (auto a : v)
			j->objects.push_back(serialize(a));
		return j;
	}
	template<>
	JsonObjectAbstract* serialize(const SceneObject& so) {
		auto jo = new JsonObject();
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


	template<typename T>
	void put(const T& val) {
		throw std::exception("Unsupported Type found while writing file.");
	}
	template<>
	void put(const std::string& val) {
		buffer << '"' << val << '"';
	}
	template<>
	void put(const JsonObjectAbstract& joa) {
		switch (joa.GetType()) {
		case JPrimitive:
		{
			auto j = (JsonPrimitive*)&joa;

			buffer << j->value;
			break;
		}
		case JPrimitiveString:
		{
			auto j = (JsonPrimitiveString*)&joa;

			put(j->value);
			break;
		}
		case JObject:
		{
			auto j = (JsonObject*)&joa;

			buffer << "{";
			bool isFirst = true;
			for (auto pair : j->objects) {
				if (!isFirst)
					buffer << ",";
				put(pair.first);
				buffer << ":";
				put(*pair.second);
				isFirst = false;
			}
			buffer << "}";
			break;
		}
		case JArray:
		{
			auto j = (JsonArray*)&joa;

			buffer << "[";
			if (j->objects.size() > 0)
				put(*j->objects[0]);
			for (size_t i = 1; i < j->objects.size(); i++) {
				buffer << ',';
				put(*j->objects[i]);
			}
			buffer << ']';
			break;
		}
		}
	}
	template<>
	void put(const SceneObject& so) {
		auto o = serialize(so);
		put(*o);
		delete o;
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
	// Skips comma if exists.
	void skipComma() {
		if (buffer.get() == ',')
			return;

		buffer.seekg(-1, std::ios_base::cur);
	}


	bool isString() {
		bool is = false;
		
		if (buffer.get() == '"')
			is = true;
		
		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isObject() {
		bool is = false;

		if (buffer.get() == '{')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isObjectEnd() {
		bool is = false;

		if (buffer.get() == '}')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isArray() {
		bool is = false;

		if (buffer.get() == '[')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isArrayEnd() {
		bool is = false;

		if (buffer.get() == ']')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isArrayEmpty() {
		bool is = false;

		skip();//[
		if (buffer.get() == ']')
			is = true;

		buffer.seekg(-2, std::ios_base::cur);

		return is;
	}

	std::string readName() {
		std::string v;
		char c;

		skip();//"
		while ((c = buffer.get()) != '"')
			v += c;

		skip();//:
		return v;
	}
	JsonObjectAbstract* readValue() {
		std::string v;
		char c;
		bool isString = (c = buffer.get()) == '"';

		if (!isString)
			v += c;
		
		while (c = buffer.get(), isString ? (c != '"') : (c != ',' && c != ']'))
			v += c;

		if (c == ']')
			buffer.seekg(-1, std::ios_base::cur);
		
		skipComma();

		if (isString) {
			auto o = new JsonPrimitiveString();
			o->value = v;
			return o;
		}

		auto o = new JsonPrimitive();
		o->value = v;
		return o;
	}

	template<typename T>
	T* start() {
		auto t = new T();
		scene->Insert(t);
		return t;
	}

	template<typename T>
	void get(JsonObject* joa, std::string name, T& dest) {
		dest = get<T>(joa->objects[name]);
	}
	template<typename T>
	void get(JsonObject* joa, std::string name, std::function<void(T)> dest) {
		dest(get<T>(joa->objects[name]));
	}
	template<typename...T>
	void getArray(JsonObject* jo, std::string name, std::function<void(T...)> f) {
		auto j = (JsonArray*)jo->objects[name];
		for (auto o : j->objects)
			f(get<T>(o)...);
	}
	void getArray(JsonObject* jo, std::string name, std::function<void(size_t, size_t)> f) {
		auto j = (JsonArray*)jo->objects[name];
		for (auto o : j->objects) {
			auto v = get<size_t, 2>(o);
			f(v[0], v[1]);
		}
	}

	void getChildren(JsonObject* j, std::string name, SceneObject* parent) {
		getArray(j, name, std::function([&parent](SceneObject* v) {
			v->SetParent(parent);
			}));
	}
public:
	Scene* scene;

	template<typename T>
	T get(std::string str) {
		std::stringstream ss;
		ss << str;
		T val;
		ss >> val;
		return val;
	}
	template<>
	ObjectType get(std::string str) {
		return (ObjectType)get<int>(str);
	}
	template<>
	std::string get(std::string str) {
		return str;
	}
	template<typename T>
	T get(JsonObjectAbstract* joa) {
		auto j = (JsonPrimitive*)joa;
		return get<T>(j->value);
	}
	template<>
	std::string get(JsonObjectAbstract* joa) {
		auto j = (JsonPrimitiveString*)joa;
		return get<std::string>(j->value);
	}
	template<>
	glm::vec3 get(JsonObjectAbstract* joa) {
		auto j = (JsonArray*)joa;
		glm::vec3 v;
		for (size_t i = 0; i < j->objects.size(); i++) {
			((float*)&v)[i] = get<float>(j->objects[i]);
		}
		return v;
	}
	template<>
	glm::fquat get(JsonObjectAbstract* joa) {
		auto j = (JsonArray*)joa;
		glm::fquat v;
		for (size_t i = 0; i < j->objects.size(); i++) {
			((float*)&v)[i] = get<float>(j->objects[i]);
		}
		return v;
	}
	template<>
	std::vector<SceneObject*> get(JsonObjectAbstract* joa) {
		auto j = (JsonArray*)joa;
		std::vector<SceneObject*> v;
		for (auto p : j->objects)
			v.push_back(get<SceneObject*>(p));
		return v;
	}
	template<typename T, size_t S>
	std::array<T, S> get(JsonObjectAbstract* joa) {
		auto j = (JsonArray*)joa;
		std::array<T, S> v;
		for (size_t i = 0; i < S; i++) {
			v[i] = get<T>(j->objects[i]);
		}
		return v;
	}
	template<>
	SceneObject* get(JsonObjectAbstract* joa) {
		auto j = (JsonObject*)joa;
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

	JsonObjectAbstract* getJson() {
		if (isObject()) {
			auto o = new JsonObject();
			skip();//{
			while (!isObjectEnd())//}
				o->objects.insert({ readName(), getJson() });
			skip();//}
			skipComma();
			return o;
		}
		if (isArray()) {
			auto o = new JsonArray();
			skip();//[
			while (!isArrayEnd())//]
				o->objects.push_back(getJson());
			skip();//]
			skipComma();
			return o;
		}
		return readValue();
	}

	SceneObject* read() {
		auto j = getJson();
		auto so = get<SceneObject*>(j);
		delete j;
		return so;
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

		bs.put(*inScene->root);

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

		auto o = str.read();
		inScene->root = o;
		o->SetParent(nullptr);

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
		inScene->root = o;
		o->SetParent(nullptr);

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