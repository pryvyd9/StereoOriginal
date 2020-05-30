#pragma once 

#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "TemplateExtensions.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <map>

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


class Log {
	template<typename T>
	using isOstreamable = decltype(std::declval<std::ostringstream>() << std::declval<T>());

	template<typename T>
	static constexpr bool isOstreamableT = is_detected_v<isOstreamable, T>;

	std::string contextName = "";

	static void Line(const std::string& message) {
		std::cout << message << std::endl;
	}
	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	static std::string ToString(const T& message) {
		std::ostringstream ss;
		ss << message;
		return ss.str();
	}

public:
	template<typename T>
	static const Log For() {
		Log log;
		log.contextName = typeid(T).name();
		return log;
	}

	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	void Error(const T& message) const {
		Error(Log::ToString(message));
	}
	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	void Warning(const T& message) const {
		Warning(Log::ToString(message));
	}
	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	void Information(const T& message) const {
		Information(Log::ToString(message));
	}

	void Error(const std::string& message) const {
		Line("[Error](" + contextName + ") " + message);
	}
	void Warning(const std::string& message) const {
		Line("[Warning](" + contextName + ") " + message);
	}
	void Information(const std::string& message) const {
		Line("[Warning](" + contextName + ") " + message);
	}
};

class Time {
	static std::chrono::steady_clock::time_point* GetBegin() {
		static std::chrono::steady_clock::time_point instance;
		return &instance;
	}

	static size_t* GetDeltaTimeMicroseconds() {
		static size_t instance;
		return &instance;
	}
public:
	static void UpdateFrame() {
		auto end = std::chrono::steady_clock::now();
		*GetDeltaTimeMicroseconds() = std::chrono::duration_cast<std::chrono::microseconds>(end - *GetBegin()).count();
		*GetBegin() = end;
	};

	static float GetFrameRate() {
		return 1 / GetDeltaTime();
	}

	static float GetDeltaTime() {
		return (float)*GetDeltaTimeMicroseconds() / 1e6;
	}
};

template<typename...T>
class Event {
	std::map<size_t, std::function<void(T...)>> handlers;
public:
	size_t AddHandler(std::function<void(T...)> func) const {
		static size_t id = 0;

		(*const_cast<std::map<size_t, std::function<void(T...)>>*>(&handlers))[id] = func;

		return id++;
	}

	void RemoveHandler(size_t v) const {
		(*const_cast<std::map<size_t, std::function<void(T...)>>*>(&handlers)).erase(v);
	}


	void Invoke(T... vs) {
		for (auto h : handlers)
			h.second(vs...);
	}
};

//template<typename T>
//class Event {
//	std::map<size_t, std::function<void(T)>> handlers;
//public:
//	size_t AddHandler(std::function<void(T)> func) const {
//		static size_t id = 0;
//
//		handlers[id] = func;
//
//		return id++;
//	}
//
//	void RemoveHandler(size_t v) const {
//		handlers.erase(v);
//	}
//
//
//	void Invoke(T vs) {
//		for (auto h : handlers)
//			h(vs);
//	}
//};