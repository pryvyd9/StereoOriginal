#pragma once

#include "DomainUtils.hpp"
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

// Json
template<typename T>
struct J {
	enum JType {
		JObject,
		JArray,
		JPrimitive,
		JPrimitiveString,
	};
	struct ObjectAbstract {
		virtual JType GetType() const = 0;
	};
	struct Object : ObjectAbstract {
		std::unordered_map<T, ObjectAbstract*> objects;

		virtual JType GetType() const {
			return JObject;
		}
		~Object() {
			for (auto o : objects) {
				delete o.second;
			}
		}
	};
	struct Array : ObjectAbstract {
		std::vector<ObjectAbstract*> objects;

		virtual JType GetType() const {
			return JArray;
		}

		~Array() {
			for (auto o : objects) {
				delete o;
			}
		}
	};
	struct Primitive : ObjectAbstract {
		T value;

		virtual JType GetType() const {
			return JPrimitive;
		}
	};
	struct PrimitiveString : ObjectAbstract {
		T value;

		virtual JType GetType() const {
			return JPrimitiveString;
		}
	};
};

struct Js : J<std::string> {};
struct Jw : J<std::wstring> {};


class ojstreams {
	std::stringstream buffer;

	template<typename T>
	static std::string toString(const T& t) {
		std::stringstream temp;
		temp << t;
		return temp.str();
	}

	template<typename T>
	void insert(Js::Object* jo, std::string name, const T& v) {
		jo->objects.insert({ name, serialize(v) });
	}
public:
	template<typename T>
	Js::ObjectAbstract* serialize(const T& o) {
		auto j = new Js::Primitive();
		j->value = toString(o);
		return j;
	}
	template<>
	Js::ObjectAbstract* serialize(const std::string& o) {
		auto j = new Js::PrimitiveString();
		j->value = o;
		return j;
	}
	template<>
	Js::ObjectAbstract* serialize(const glm::vec3& o) {
		auto j = new Js::Array();
		j->objects = { serialize(o.x), serialize(o.y), serialize(o.z) };
		return j;
	}
	template<>
	Js::ObjectAbstract* serialize(const glm::fquat& o) {
		auto j = new Js::Array();
		j->objects = { serialize(o.x), serialize(o.y), serialize(o.z), serialize(o.w) };
		return j;
	}
	template<typename T>
	Js::ObjectAbstract* serialize(const std::vector<T*>& v) {
		auto j = new Js::Array();
		for (auto a : v)
			if (auto r = serialize(*a); r != nullptr)
				j->objects.push_back(r);
		return j;
	}
	template<typename T>
	Js::ObjectAbstract* serialize(const std::vector<T>& v) {
		auto j = new Js::Array();
		for (auto a : v)
			j->objects.push_back(serialize(a));
		return j;
	}
	template<typename T, GLuint S>
	Js::ObjectAbstract* serialize(const std::array<T, S>& v) {
		auto j = new Js::Array();
		for (auto a : v)
			j->objects.push_back(serialize(a));
		return j;
	}
	template<>
	Js::ObjectAbstract* serialize(const SceneObject& so) {
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


	template<typename T>
	void put(const T& val) {
		throw std::exception("Unsupported Type found while writing file.");
	}
	template<>
	void put(const std::string& val) {
		buffer << '"' << val << '"';
	}
	template<>
	void put(const Js::ObjectAbstract& joa) {
		switch (joa.GetType()) {
		case Js::JPrimitive:
		{
			auto j = (Js::Primitive*)&joa;

			buffer << j->value;
			break;
		}
		case Js::JPrimitiveString:
		{
			auto j = (Js::PrimitiveString*)&joa;

			put(j->value);
			break;
		}
		case Js::JObject:
		{
			auto j = (Js::Object*)&joa;

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
		case Js::JArray:
		{
			auto j = (Js::Array*)&joa;

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

class ijstreams {
	bool isRoot = true;

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
	void skipWhiteSpace() {
		if (isWhiteSpace(buffer.get()))
			skipWhiteSpace();
		else
			buffer.seekg(-1, std::ios_base::cur);
	}
	void skipColon() {
		if (buffer.get() == L':')
			return;

		buffer.seekg(-1, std::ios_base::cur);
	}

	bool isWhiteSpace(const char& c) {
		switch (c) {
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return true;
		default:
			return false;
		}
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

		skipWhiteSpace();
		skip();//"
		while ((c = buffer.get()) != '"')
			v += c;

		skipWhiteSpace();
		skipColon();//:
		return v;
	}
	Js::ObjectAbstract* readValue() {
		skipWhiteSpace();
		std::string v;
		char c;
		bool isString = (c = buffer.get()) == '"';

		if (!isString)
			v += c;

		while (c = buffer.get(), isString ? (c != '"') : (c != ',' && c != ']'))
			v += c;

		if (c == ']')
			buffer.seekg(-1, std::ios_base::cur);

		skipWhiteSpace();
		skipComma();

		if (isString) {
			auto o = new Js::PrimitiveString();
			o->value = v;
			return o;
		}

		auto o = new Js::Primitive();
		o->value = v;
		return o;
	}

	template<typename T>
	T* start() {
		auto t = new T();
		if (isRoot)
			isRoot = false;
		else
			objects.push_back(t);
		return t;
	}

	template<typename T>
	void get(Js::Object* joa, std::string name, T& dest) {
		dest = get<T>(joa->objects[name]);
	}
	template<typename T>
	void get(Js::Object* joa, std::string name, std::function<void(T)> dest) {
		dest(get<T>(joa->objects[name]));
	}
	template<typename...T>
	void getArray(Js::Object* jo, std::string name, std::function<void(T...)> f) {
		auto j = (Js::Array*)jo->objects[name];
		for (auto o : j->objects)
			f(get<T>(o)...);
	}
	void getArray(Js::Object* jo, std::string name, std::function<void(size_t, size_t)> f) {
		auto j = (Js::Array*)jo->objects[name];
		for (auto o : j->objects) {
			auto v = get<size_t, 2>(o);
			f(v[0], v[1]);
		}
	}

	void getChildren(Js::Object* j, std::string name, SceneObject* parent) {
		getArray(j, name, std::function([&parent](SceneObject* v) {
			v->SetParent(parent);
			}));
	}
public:
	std::vector<SceneObject*> objects;

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
	T get(Js::ObjectAbstract* joa) {
		auto j = (Js::Primitive*)joa;
		return get<T>(j->value);
	}
	template<>
	std::string get(Js::ObjectAbstract* joa) {
		auto j = (Js::PrimitiveString*)joa;
		return get<std::string>(j->value);
	}
	template<>
	glm::vec3 get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		glm::vec3 v;
		for (size_t i = 0; i < j->objects.size(); i++)
			((float*)&v)[i] = get<float>(j->objects[i]);
		return v;
	}
	template<>
	glm::fquat get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		glm::fquat v;
		for (size_t i = 0; i < j->objects.size(); i++)
			((float*)&v)[i] = get<float>(j->objects[i]);
		return v;
	}
	template<>
	std::vector<SceneObject*> get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		std::vector<SceneObject*> v;
		for (auto p : j->objects)
			v.push_back(get<SceneObject*>(p));
		return v;
	}
	template<typename T, size_t S>
	std::array<T, S> get(Js::ObjectAbstract* joa) {
		auto j = (Js::Array*)joa;
		std::array<T, S> v;
		for (size_t i = 0; i < S; i++) {
			v[i] = get<T>(j->objects[i]);
		}
		return v;
	}
	template<>
	SceneObject* get(Js::ObjectAbstract* joa) {
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

	Js::ObjectAbstract* getJson() {
		skipWhiteSpace();
		skipColon();
		skipWhiteSpace();
		if (isObject()) {
			auto o = new Js::Object();
			skipWhiteSpace();
			skip();//{
			while (skipWhiteSpace(), !isObjectEnd())//}
				o->objects.insert({ readName(), getJson() });
			skip();//}
			skipWhiteSpace();
			skipComma();
			return o;
		}
		if (isArray()) {
			auto o = new Js::Array();
			skipWhiteSpace();
			skip();//[
			while (skipWhiteSpace(), !isArrayEnd())//]
				o->objects.push_back(getJson());
			skip();//]
			skipWhiteSpace();
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


class ijstreamw {
	bool isRoot = true;

	std::wistringstream buffer;

	wchar_t getwc() {
		wchar_t v;
		buffer.readsome(&v, 1);
		//((char*)&v)[1] = buffer.get();
		//((char*)&v)[0] = buffer.get();
		return v;
	}


	void skipName() {
		while (getwc() != L':');
	}
	void skip() {
		auto a = getwc();
	}
	// Skips comma if exists.
	void skipComma() {
		if (getwc() == L',')
			return;

		buffer.seekg(-1, std::ios_base::cur);
	}
	void skipWhiteSpace() {
		auto c = getwc();
		switch (c) {
		case L' ':
		case L'\n':
		case L'\r':
		case L'\t':
			return skipWhiteSpace();
		default:
			buffer.seekg(-1, std::ios_base::cur);
			break;
		}
	}
	void skipColon() {
		if (getwc() == L':')
			return;

		buffer.seekg(-1, std::ios_base::cur);
	}

	bool isString() {
		bool is = false;

		if (getwc() == L'"')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isObject() {
		bool is = false;

		if (getwc() == L'{')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isObjectEnd() {
		bool is = false;

		if (getwc() == L'}')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isArray() {
		bool is = false;

		if (getwc() == L'[')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isArrayEnd() {
		bool is = false;

		if (getwc() == L']')
			is = true;

		buffer.seekg(-1, std::ios_base::cur);

		return is;
	}
	bool isArrayEmpty() {
		bool is = false;

		skip();//[
		if (getwc() == L']')
			is = true;

		buffer.seekg(-2, std::ios_base::cur);

		return is;
	}

	std::wstring readName() {
		std::wstring v;
		wchar_t c;

		skipWhiteSpace();
		skip();//"
		while ((c = getwc()) != L'"')
			v += c;

		skipWhiteSpace();
		skipColon();
		return v;
	}
	Jw::ObjectAbstract* readValue() {
		skipWhiteSpace();
		std::wstring v;
		wchar_t c;
		bool isString = (c = getwc()) == L'"';

		if (!isString)
			v += c;

		while (c = getwc(), isString ? (c != L'"') : (c != L',' && c != L']'))
			v += c;

		if (c == L']')
			buffer.seekg(-1, std::ios_base::cur);

		skipWhiteSpace();
		skipComma();

		if (isString) {
			auto o = new Jw::PrimitiveString();
			o->value = v;
			return o;
		}

		auto o = new Jw::Primitive();
		o->value = v;
		return o;
	}

	template<typename TString>
	TString* start() {
		auto t = new TString();
		if (isRoot)
			isRoot = false;
		else
			objects.push_back(t);
		return t;
	}

	template<typename T>
	void get(Jw::Object* joa, std::wstring name, T& dest) {
		dest = get<T>(joa->objects[name]);
	}
	template<typename T>
	void get(Jw::Object* joa, std::wstring name, std::function<void(T)> dest) {
		dest(get<T>(joa->objects[name]));
	}
	template<typename...T>
	void getArray(Jw::Object* jo, std::wstring name, std::function<void(T...)> f) {
		auto j = (Jw::Array*)jo->objects[name];
		for (auto o : j->objects)
			f(get<T>(o)...);
	}
	void getArray(Jw::Object* jo, std::wstring name, std::function<void(size_t, size_t)> f) {
		auto j = (Jw::Array*)jo->objects[name];
		for (auto o : j->objects) {
			auto v = get<size_t, 2>(o);
			f(v[0], v[1]);
		}
	}

	void getChildren(Jw::Object* j, std::wstring name, SceneObject* parent) {
		getArray(j, name, std::function([&parent](SceneObject* v) {
			v->SetParent(parent);
			}));
	}
public:
	std::vector<SceneObject*> objects;

	template<typename T>
	using isOstreamable = decltype(std::declval<std::wstringstream>() >> std::declval<T>());

	template<typename T>
	static constexpr bool isOstreamableT = is_detected_v<isOstreamable, T>;

	template<typename T>
	T get(std::wstring str) {
		Log::For<T>().Error("unsupported type");
		throw new std::exception("unsupported type");
	}

	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	T get(std::wstring str) {
		std::wstringstream ss;
		ss << str;
		T val;
		ss >> val;
		return val;
	}
	template<>
	ObjectType get(std::wstring str) {
		return (ObjectType)get<int>(str);
	}
	template<>
	std::wstring get(std::wstring str) {
		return str;
	}
	template<typename T>
	T get(Jw::ObjectAbstract* joa) {
		auto j = (Jw::Primitive*)joa;
		return get<T>(j->value);
	}
	template<>
	std::wstring get(Jw::ObjectAbstract* joa) {
		auto j = (Jw::PrimitiveString*)joa;
		return get<std::wstring>(j->value);
	}
	template<>
	glm::vec3 get(Jw::ObjectAbstract* joa) {
		auto j = (Jw::Array*)joa;
		glm::vec3 v;
		for (size_t i = 0; i < j->objects.size(); i++)
			((float*)&v)[i] = get<float>(j->objects[i]);
		return v;
	}
	template<>
	glm::fquat get(Jw::ObjectAbstract* joa) {
		auto j = (Jw::Array*)joa;
		glm::fquat v;
		for (size_t i = 0; i < j->objects.size(); i++)
			((float*)&v)[i] = get<float>(j->objects[i]);
		return v;
	}
	template<>
	std::vector<SceneObject*> get(Jw::ObjectAbstract* joa) {
		auto j = (Jw::Array*)joa;
		std::vector<SceneObject*> v;
		for (auto p : j->objects)
			v.push_back(get<SceneObject*>(p));
		return v;
	}
	template<typename T, size_t S>
	std::array<T, S> get(Jw::ObjectAbstract* joa) {
		auto j = (Jw::Array*)joa;
		std::array<T, S> v;
		for (size_t i = 0; i < S; i++) {
			v[i] = get<T>(j->objects[i]);
		}
		return v;
	}
	
	Jw::ObjectAbstract* getJson() {
		skipWhiteSpace();
		skipColon();
		skipWhiteSpace();
		if (isObject()) {
			auto o = new Jw::Object();
			skipWhiteSpace();
			skip();//{
			while (skipWhiteSpace(), !isObjectEnd())//}
				o->objects.insert({ readName(), getJson() });
			skip();//}
			skipWhiteSpace();
			skipComma();
			return o;
		}
		if (isArray()) {
			auto o = new Jw::Array();
			skipWhiteSpace();
			skip();//[
			while (skipWhiteSpace(), !isArrayEnd())//]
				o->objects.push_back(getJson());
			skip();//]
			skipWhiteSpace();
			skipComma();
			return o;
		}
		return readValue();
	}

	void setBuffer(wchar_t* buf) {
		buffer = std::wistringstream(buf);
	}
};

//class JsonConvert {
//	static bool& isRoot() {
//		static bool v;
//		return v;
//	}
//
//	template<typename T>
//	static T* start() {
//		auto t = new T();
//		if (isRoot())
//			isRoot() = false;
//		else
//			objects().push_back(t);
//		return t;
//	}
//
//	template<typename T>
//	static void get(Js::Object* joa, std::string name, T& dest) {
//		dest = get<T>(joa->objects[name]);
//	}
//	template<typename T>
//	static void get(Js::Object* joa, std::string name, std::function<void(T)> dest) {
//		dest(get<T>(joa->objects[name]));
//	}
//	template<typename...T>
//	static void getArray(Js::Object* jo, std::string name, std::function<void(T...)> f) {
//		auto j = (Js::Array*)jo->objects[name];
//		for (auto o : j->objects)
//			f(get<T>(o)...);
//	}
//	static void getArray(Js::Object* jo, std::string name, std::function<void(size_t, size_t)> f) {
//		auto j = (Js::Array*)jo->objects[name];
//		for (auto o : j->objects) {
//			auto v = get<size_t, 2>(o);
//			f(v[0], v[1]);
//		}
//	}
//
//	static void getChildren(Js::Object* j, std::string name, SceneObject* parent) {
//		getArray(j, name, std::function([&parent](SceneObject* v) {
//			v->SetParent(parent);
//			}));
//	}
//
//public:
//	static std::vector<SceneObject*>& objects() {
//		static std::vector<SceneObject*> v;
//		return v;
//	}
//
//	template<typename T>
//	static T get(std::string str) {
//		std::stringstream ss;
//		ss << str;
//		T val;
//		ss >> val;
//		return val;
//	}
//	template<>
//	static ObjectType get(std::string str) {
//		return (ObjectType)get<int>(str);
//	}
//
//	template<typename T>
//	static T get(Js::ObjectAbstract* joa) {
//		auto j = (Js::Primitive*)joa;
//		return get<T>(j->value);
//	}
//	template<>
//	static std::string get(Js::ObjectAbstract* joa) {
//		auto j = (Js::PrimitiveString*)joa;
//		return get<std::string>(j->value);
//	}
//	template<>
//	static glm::vec3 get(Js::ObjectAbstract* joa) {
//		auto j = (Js::Array*)joa;
//		glm::vec3 v;
//		for (size_t i = 0; i < j->objects.size(); i++)
//			((float*)&v)[i] = get<float>(j->objects[i]);
//		return v;
//	}
//	template<>
//	static glm::fquat get(Js::ObjectAbstract* joa) {
//		auto j = (Js::Array*)joa;
//		glm::fquat v;
//		for (size_t i = 0; i < j->objects.size(); i++)
//			((float*)&v)[i] = get<float>(j->objects[i]);
//		return v;
//	}
//	template<>
//	static std::vector<SceneObject*> get(Js::ObjectAbstract* joa) {
//		auto j = (Js::Array*)joa;
//		std::vector<SceneObject*> v;
//		for (auto p : j->objects)
//			v.push_back(get<SceneObject*>(p));
//		return v;
//	}
//	template<typename T, size_t S>
//	static std::array<T, S> get(Js::ObjectAbstract* joa) {
//		auto j = (Js::Array*)joa;
//		std::array<TString, S> v;
//		for (size_t i = 0; i < S; i++) {
//			v[i] = get<TString>(j->objects[i]);
//		}
//		return v;
//	}
//	template<>
//	static SceneObject* get(Js::ObjectAbstract* joa) {
//		auto j = (Js::Object*)joa;
//		auto type = get<ObjectType>(j->objects["type"]);
//		switch (type) {
//		case Group:
//		{
//			auto o = start<GroupObject>();
//			get(j, "name", o->Name);
//			get(j, "localPosition", std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
//			get(j, "localRotation", std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
//			getChildren(j, "children", o);
//			return o;
//		}
//		case StereoPolyLineT:
//		{
//			auto o = start<StereoPolyLine>();
//			get(j, "name", o->Name);
//			get(j, "localPosition", std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
//			get(j, "localRotation", std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
//			getChildren(j, "children", o);
//			getArray(j, "vertices", std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
//			return o;
//		}
//		case MeshT:
//		{
//			auto o = start<Mesh>();
//			get(j, "name", o->Name);
//			get(j, "localPosition", std::function([&o](glm::vec3 v) { o->SetLocalPosition(v); }));
//			get(j, "localRotation", std::function([&o](glm::fquat v) { o->SetLocalRotation(v); }));
//			getChildren(j, "children", o);
//			getArray(j, "vertices", std::function([&o](glm::vec3 v) { o->AddVertice(v); }));
//			getArray(j, "connections", std::function([&o](size_t a, size_t b) { o->Connect(a, b); }));
//			return o;
//		}
//		case CameraT:
//			break;
//		case CrossT:
//			break;
//		}
//
//		throw std::exception("Unsupported Scene Object Type found while reading file.");
//	}
//
//	static void Reset() {
//		isRoot() = true;
//		objects().clear();
//	}
//};

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
	static size_t GetFileSizeW(std::string filename) {
		std::wifstream in(filename, std::ios::in | std::ios::ate);

		if (in)
			return in.tellg();

		return 0;
	}

	static wchar_t* Read(std::wifstream& file, size_t size) {
		file.seekg(2, std::ios_base::beg);
		size-=2;

		auto buffer = new wchar_t[size];
		for (size_t i = 0; i < size; i++) {
			((char*)buffer)[i*2] = file.get();
			((char*)buffer)[i*2 + 1] = file.get();
		}

		return buffer;
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

		std::ofstream out(filename);

		ojstreams bs;

		bs.put(*inScene->root.Get().Get());

		out << bs.getBuffer();

		out.close();
	}
	static void LoadJson(std::string filename, Scene* inScene) {
		std::ifstream file(filename, std::ios::binary | std::ios::in);

		auto bufferSize = GetFileSize(filename);

		auto buffer = new char[bufferSize];
		file.read(buffer, bufferSize);

		ijstreams str;
		str.setBuffer(buffer);

		auto o = str.read();
		inScene->root = o;

		std::vector<PON> newObjects;
		for (auto o : str.objects)
			newObjects.push_back(o);

		inScene->objects = newObjects;
		
		delete[] buffer;
		file.close();
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
		std::wifstream file(filename, std::ios::binary | std::ios::in);
		
		auto bufferSize = GetFileSizeW(filename);
		auto buffer = Read(file, bufferSize);

		ijstreamw str;
		str.setBuffer(buffer);

		auto json = str.getJson();

		delete[] buffer;
		file.close();

		return json;
	}
};