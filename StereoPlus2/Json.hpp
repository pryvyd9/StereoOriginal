#pragma once
#include <gl\GL.h>
#include <string>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <vector>

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

public:
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
			auto j = (Js::Primitive*) & joa;

			buffer << j->value;
			break;
		}
		case Js::JPrimitiveString:
		{
			auto j = (Js::PrimitiveString*) & joa;

			put(j->value);
			break;
		}
		case Js::JObject:
		{
			auto j = (Js::Object*) & joa;

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
			auto j = (Js::Array*) & joa;

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
	
	std::string getBuffer() {
		return buffer.str();
	}
};

class ijstreams {
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

		while (c = buffer.get(), isString ? (c != '"') : (c != ',' && c != ']' && c != '}'))
			v += c;

		if (c == ']' || c == '}')
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

public:
	
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

	void setBuffer(char* buf) {
		buffer = std::istringstream(buf);
	}
};

class ijstreamw {
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

		while (c = getwc(), isString ? (c != L'"') : (c != L',' && c != L']' && c!= L'}'))
			v += c;

		if (c == L']' || c == L'}')
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

public:
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

class Json {
	static size_t GetFileSize(std::wstring filename) {
		std::ifstream in(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (in)
			return in.tellg();

		return 0;
	}
	static size_t GetFileSizeW(std::wstring filename) {
		std::wifstream in(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (in)
			return in.tellg();

		return 0;
	}

	static wchar_t* Read(std::wifstream& file, size_t size) {
		file.seekg(2, std::ios_base::beg);
		size -= 2;

		auto buffer = new wchar_t[size];
		for (size_t i = 0; i < size; i++) {
			((char*)buffer)[i * 2] = file.get();
			((char*)buffer)[i * 2 + 1] = file.get();
		}

		return buffer;
	}

public:
	static Js::ObjectAbstract* Read(const std::wstring& filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::in);

		auto bufferSize = GetFileSize(filename);
		auto buffer = new char[bufferSize];
		file.read(buffer, bufferSize);

		ijstreams str;
		str.setBuffer(buffer);

		auto json = str.getJson();

		delete[] buffer;
		file.close();

		return json;
	}
	static Jw::ObjectAbstract* ReadW(const std::wstring& filename) {
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

	static void Write(const std::wstring& filename, Js::ObjectAbstract* joa) {
		std::ofstream out(filename);
		ojstreams bs;

		bs.put(*joa);
		out << bs.getBuffer();
		out.close();
	}
};
