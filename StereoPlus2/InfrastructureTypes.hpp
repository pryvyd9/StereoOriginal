#pragma once 

#include <stdlib.h>
#include <chrono>
#include <iostream>

class Log {
	std::string contextName = "";

	static void Line(const std::string& message) {
		std::cout << message << std::endl;
	}
public:
	template<typename T>
	static const Log For() {
		Log log;
		log.contextName = typeid(T).name();
		return log;
	}

	void Error(const std::string& message) const {
		Line("[Error](" + contextName + ") " + message);
	}

	void Warning(const std::string& message) const {
		Line("[Warning](" + contextName + ") " + message);
	}

	void Information(const std::string& message) const {
		Line("[Information](" + contextName + ") " + message);
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
