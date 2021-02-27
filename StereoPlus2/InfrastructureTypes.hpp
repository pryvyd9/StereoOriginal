#pragma once 

#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "TemplateExtensions.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <map>
#include <glm/vec3.hpp>

#include <fstream>
#include <filesystem>// C++17 standard header file name
namespace fs = std::filesystem;

template<typename T>
int find(const std::vector<T>& source, const T& item) {
	for (size_t i = 0; i < source.size(); i++)
		if (source[i] == item)
			return i;

	return -1;
}
template<typename T>
int find(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	for (size_t i = 0; i < source.size(); i++)
		if (condition(source[i]))
			return i;

	return -1;
}
//template<typename T>
//int find(std::vector<T*>& source, std::function<bool(T*)> condition) {
//	for (size_t i = 0; i < source.size(); i++)
//		if (condition(source[i]))
//			return i;
//
//	return -1;
//}
template<typename T>
int findBack(std::vector<T*>& source, std::function<bool(T*)> condition) {
	if (source.empty())
		return -1;

	for (int i = source.size() - 1; i >= 0; i--)
		if (condition(source[i]))
			return i;

	return -1;
}

template<typename T>
std::vector<int> findAll(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	std::vector<int> indices;
	for (size_t i = 0; i < source.size(); i++)
		if (condition(source[i]))
			indices.push_back(i);

	return indices;
}
template<typename T>
std::vector<int> findAllBack(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	std::vector<int> indices;
	for (size_t i = source.size(); i-- != 0; )
		if (condition(source[i]))
			indices.push_back(i);

	return indices;
}
template<typename T>
bool exists(const std::set<T>& source, const T& item) {
	return source.find(item) != source.end();
}
template<typename T>
bool exists(const std::set<const T>& source, const T& item) {
	return source.find(item) != source.end();
}
template<typename K, typename T>
bool exists(const std::set<K>& source, const T& item, std::function<T(const K&)> selector) {
	for (auto o : source)
		if (selector(o) == item)
			return true;

	return false;
}

template<typename T>
bool exists(const std::vector<T>& source, const T& item) {
	return std::find(source.begin(), source.end(), item) != source.end();
}
template<typename T>
bool exists(const std::list<T>& source, const T& item) {
	return std::find(source.begin(), source.end(), item) != source.end();
}
template<typename K, typename V>
bool keyExists(const std::set<K,V>& source, const K& item) {
	return source.find(item) != source.end();
}

class Path {
	fs::path path;
	std::string pathBuffer;
public:
	const fs::path& get() const {
		return path;
	}
	std::string& getBuffer() {
		return pathBuffer;
	}

	Path() {}
	Path(fs::path n) {
		apply(n);
	}
	void apply() {
		apply(pathBuffer);
	}
	void apply(fs::path n) {
		path = fs::absolute(n);
		pathBuffer = path.u8string();
	}

	bool isSome() {
		return !pathBuffer.empty();
	}

	std::string join(Path path) {
		auto last = pathBuffer[pathBuffer.size() - 1];

		if (last == '/' || last == '\\')
			return pathBuffer + path.getBuffer();

		return pathBuffer + '/' + path.getBuffer();
	}
	fs::path joinPath(const std::string& path) {
		auto last = pathBuffer[pathBuffer.size() - 1];

		if (last == '/' || last == '\\')
			return fs::absolute(pathBuffer + path);

		return fs::absolute(pathBuffer + '/' + path);
	}
};

class Time {
	static const int timeLogSize = 5;

	static std::chrono::steady_clock::time_point* GetBegin() {
		static std::chrono::steady_clock::time_point instance;
		return &instance;
	}
	static size_t* GetDeltaTimeMicroseconds() {
		static size_t instance;
		return &instance;
	}

	static std::vector<size_t>& TimeLog() {
		static std::vector<size_t> v(timeLogSize);
		return v;
	}

	static void UpdateTimeLog(size_t v) {
		static int i = 0;
		TimeLog()[i] = v;
		i++;
		if (i >= timeLogSize)
			i = 0;
	}
public:
	static void UpdateFrame() {
		auto end = std::chrono::steady_clock::now();
		*GetDeltaTimeMicroseconds() = std::chrono::duration_cast<std::chrono::microseconds>(end - *GetBegin()).count();
		*GetBegin() = end;

		UpdateTimeLog(*GetDeltaTimeMicroseconds());
	}
	static int GetFrameRate() {
		return round(1 / GetDeltaTime());
	}
	static int GetAverageFrameRate() {
		return round(1 / GetAverageDeltaTime());
	}
	static float GetDeltaTime() {
		return (float)*GetDeltaTimeMicroseconds() / 1e6;
	}
	static float GetAverageDeltaTime() {
		size_t t = 0;
		for (auto a : TimeLog())
			t += a;

		return (float)t / 1e6 / timeLogSize;
	}
	static size_t GetTime() {
		return std::chrono::time_point_cast<std::chrono::milliseconds>(*GetBegin()).time_since_epoch().count();
	}
	static std::string GetTimeFormatted(const std::string& format = "%Y-%m-%d %H:%M:%S") {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);

#pragma warning(push)
#pragma warning(disable: 4996)
		std::tm tm = *std::gmtime(&now_c);
#pragma warning(pop)
		
		std::stringstream ss;
		ss << std::put_time(&tm, format.c_str());
		return ss.str();
	}

};

class Log {
public:
	struct Context {
		std::string contextName = "";
		std::string logFileName = "";
		std::string message = "";
		std::string level = "";
	};
private:
	template<typename T>
	using isOstreamable = decltype(std::declval<std::ostringstream>() << std::declval<T>());

	template<typename T>
	static constexpr bool isOstreamableT = is_detected_v<isOstreamable, T>;


	Context context;
	std::function<void(const Context&)> sink = Sink();
	
	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	static std::string ToString(const T& message) {
		std::ostringstream ss;
		ss << message;
		return ss.str();
	}
	static std::string ToString(const glm::vec3& message) {
		std::ostringstream ss;
		ss << "(" << message.x << ";" << message.y << ";" << message.z << ")";
		return ss.str();
	}

public:
	template<typename T>
	static const Log For(std::string logFileName = LogFileName(), std::function<void(const Context&)> sink = Sink()) {
		Log log;
		log.context.contextName = typeid(T).name();
		log.context.logFileName = logFileName;
		log.sink = sink;
		return log;
	}

	template<typename... T>
	void Error(const T&... message) const {
		Error((Log::ToString(message) + ...));
	}
	template<typename... T>
	void Warning(const T&... message) const {
		Warning((Log::ToString(message) + ...));
	}
	template<typename... T>
	void Information(const T&... message) const {
		Information((Log::ToString(message) + ...));
	}


	void Error(const std::string& message) const {
		Context c = context;
		c.level = "Error";
		c.message = message;
		sink(c);
	}
	void Warning(const std::string& message) const {
		Context c = context;
		c.level = "Warning";
		c.message = message;
		sink(c);
	}
	void Information(const std::string& message) const {
		Context c = context;
		c.level = "Information";
		c.message = message;
		sink(c);
	}

	static std::string& LogFileName() {
		static std::string v = "defaultLog";
		return v;
	}
	static std::function<void(const Context&)>& Sink() {
		static std::function<void(const Context&)> v = [](const Context&) {};
		return v;
	}

	static void FileSink(const Context& v) {
		static Path logDirectory("logs/");
		static std::string startTime = Time::GetTimeFormatted("%Y%m%d%H%M%S");
		if (!fs::is_directory(logDirectory.get()) || !fs::exists(logDirectory.get()))
			fs::create_directory(logDirectory.get());

		std::stringstream ss;
		ss << Time::GetTimeFormatted() << "[" << v.level << "]" << "(" << v.contextName << ")" << v.message << std::endl;
		auto log = ss.str();

		std::ofstream f(logDirectory.joinPath(v.logFileName + startTime), std::ios_base::app);
		f << log;
		f.close();
	}
	static void ConsoleSink(const Context& v) {
		std::cout << "[" << v.level << "]" << "(" << v.contextName << ")" << v.message << std::endl;
	}
};


class Command {
	static std::list<Command*>& GetQueue() {
		static auto queue = std::list<Command*>();
		return queue;
	}
protected:
	bool isReady = false;
	virtual bool Execute() = 0;
public:
	Command() {
		GetQueue().push_back(this);
	}
	static bool ExecuteAll() {
		std::list<Command*> deleteQueue;
		for (auto command : GetQueue())
			if (command->isReady) {
				if (!command->Execute())
					return false;

				deleteQueue.push_back(command);
			}

		for (auto command : deleteQueue) {
			GetQueue().remove(command);
			delete command;
		}

		return true;
	}
};

class FuncCommand : Command {
protected:
	virtual bool Execute() {
		func();

		return true;
	};
public:
	FuncCommand() {
		isReady = true;
	}

	std::function<void()> func;
};

template<typename...T>
class IEvent {
protected:
	std::map<size_t, std::function<void(T...)>> handlers;
public:
	size_t AddHandler(std::function<void(T...)> func) {
		static size_t id = 0;

		(new FuncCommand())->func = [&, id = id, f = func] {
			handlers[id] = f;
		};

		return id++;
	}
	void RemoveHandler(size_t v) {
		(new FuncCommand())->func = [&, v = v] {
			handlers.erase(v);
		};
	}

	size_t operator += (std::function<void(T...)> func) {
		return AddHandler(func);
	}
	void operator -= (size_t v) {
		RemoveHandler(v);
	}

};

template<typename...T>
class Event : public IEvent<T...> {
public:
	void Invoke(T... vs) {
		for (auto h : IEvent<T...>::handlers)
			h.second(vs...);
	}
};


//template<typename T>
//class AbstractProperty {
//protected:
//	template<typename T>
//	class Node {
//		T value;
//		Event<T> changed;
//	public:
//		const T& Get() const {
//			return value;
//		}
//		T& Get() {
//			return value;
//		}
//		void Set(const T& v) {
//			auto& old = value;
//			value = v;
//			if (old != v)
//				changed.Invoke(v);
//		}
//		IEvent<T>& OnChanged() const {
//			return *(IEvent<T>*) & changed;
//		}
//	};
//
//	std::shared_ptr<Node<T>> node = std::make_shared<Node<T>>();
//	T& Get() {
//		return node->Get();
//	}
//	void Set(const T& v) {
//		node->Set(v);
//	}
//	AbstractProperty<T>& operator=(const T& v) {
//		Set(v);
//		return *this;
//	}
//	void Bind(const AbstractProperty<T>& p) {
//		p.OnChanged().AddHandler([&](const T& o) { this->Set(o); });
//	}
//	void BindAndApply(const AbstractProperty<T>& p) {
//		Set(p.Get());
//		p.OnChanged().AddHandler([&](const T& o) { this->Set(o); });
//	}
//	void BindTwoWay(const AbstractProperty<T>& p) {
//		node = p.node;
//	}
//public:
//	IEvent<T>& OnChanged() const {
//		return node->OnChanged();
//	}
//};
//
//template<typename T>
//class Property : public AbstractProperty<T> {
//public:
//	T& Get() const {
//		return AbstractProperty<T>::Get();
//	}
//	void Set(const T& v) {
//		AbstractProperty<T>::Set(v);
//	}
//	Property<T>& operator=(const T& v) {
//		Set(v);
//		return *this;
//	}
//	void Bind(const Property<T>& p) {
//		AbstractProperty<T>::Bind(p);
//	}
//	void BindAndApply(const Property<T>& p) {
//		AbstractProperty<T>::BindAndApply(p);
//	}
//	void BindTwoWay(const Property<T>& p) {
//		AbstractProperty<T>::BindTwoWay(p);
//	}
//};
////template<typename T>
////class AbstractReadonlyProperty : public AbstractProperty<T> {
////protected:
////	T& Get() const {
////		return AbstractProperty<T>::Get();
////	}
////	void BindAndApply(const AbstractReadonlyProperty<T>& p) {
////		// Since it's readonly it doesn't matter if it's the same node.
////		AbstractProperty<T>::BindTwoWay(p);
////	}
////	T& operator->() const {
////		return Get();
////	}
////};
//
//template<typename T>
//class ReadonlyProperty : public AbstractProperty<T> {
//public:
//	ReadonlyProperty() {}
//	ReadonlyProperty(const T& o) {
//		AbstractProperty<T>::Set(o);
//	}
//	T& Get() const {
//		return AbstractProperty<T>::Get();
//	}
//	void BindAndApply(const ReadonlyProperty<T>& p) {
//		// Since it's readonly it doesn't matter if it's the same node.
//		AbstractProperty<T>::BindTwoWay(p);
//	}
//	void BindAndApply(const Property<T>& p) {
//		// Since it's readonly it doesn't matter if it's the same node.
//		AbstractProperty<T>::BindTwoWay(p);
//	}
//	T& operator->() const {
//		return Get();
//	}
//};
//
//template<typename T>
//class ReadonlyProperty<T*> : public AbstractProperty<T*> {
//public:
//	ReadonlyProperty() {}
//	ReadonlyProperty(const T* o) {
//		AbstractProperty<T*>::node->Set(*o);
//	}
//	T* Get() const {
//		return AbstractProperty<T*>::node->Get();
//		//return &AbstractProperty<T*>::Get();
//	}
//	void BindAndApply(const ReadonlyProperty<T*>& p) {
//		// Since it's readonly it doesn't matter if it's the same node.
//		AbstractProperty<T*>::BindTwoWay(p);
//	}
//	void BindAndApply(const Property<T*>& p) {
//		// Since it's readonly it doesn't matter if it's the same node.
//		AbstractProperty<T*>::BindTwoWay(p);
//	}
//	T* operator->() const {
//		return Get();
//	}
//};

template<typename T>
class Property {
	template<typename T>
	class Node {
		T value;
		Event<T> changed;
	public:
		const T& Get() const {
			return value;
		}
		T& Get() {
			return value;
		}
		void Set(const T& v) {
			value = v;
				changed.Invoke(v);
		}
		IEvent<T>& OnChanged() const {
			return *(IEvent<T>*) & changed;
		}
	};

	std::shared_ptr<Node<T>> node = std::make_shared<Node<T>>();
public:
	Property() {}
	Property(T o) {
		Set(o);
	}

	const T& Get() const {
		return node->Get();
	}
	T& Get() {
		return node->Get();
	}
	void Set(const T& v) {
		node->Set(v);
	}
	IEvent<T>& OnChanged() const {
		return node->OnChanged();
	}
	void Bind(const Property<T>& p) {
		p.OnChanged().AddHandler([&](const T& o) { this->Set(o); });
	}
	void BindAndApply(const Property<T>& p) {
		Set(p.Get());
		p.OnChanged().AddHandler([&](const T& o) { this->Set(o); });
	}
	void BindTwoWay(const Property<T>& p) {
		node = p.node;
	}

	Property<T>& operator=(const T& v) {
		Set(v);
		return *this;
	}
	const T* operator->() const {
		return &Get();
	}
};

template<typename T>
class ReadonlyProperty : Property<T> {
public:
	ReadonlyProperty() {}
	ReadonlyProperty(T o) {
		Property<T>::Set(o);
	}
	void BindAndApply(const Property<T>& p) {
		Property<T>::BindAndApply(p);
	}
	void BindAndApply(const ReadonlyProperty<T>& p) {
		Property<T>::BindAndApply(p);
	}
	void Bind(const Property<T>& p) {
		Property<T>::Bind(p);
	}
	const T& Get() const {
		return Property<T>::Get();
	}
	T& Get() {
		return Property<T>::Get();
	}
	T operator->() const {
		return Get();
	}
};

#define StaticProperty(type,name) \
static Property<type>& name() {\
	static Property<type> v;\
	return v;\
}