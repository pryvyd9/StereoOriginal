#pragma once 

#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "TemplateExtensions.hpp"
#include <sstream>
#include <string>

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
