#pragma once
#include "FileManager.hpp"
#include "Localization.hpp"

class SettingsLoader {
	static std::map<std::string, std::string>& settings() {
		static std::map<std::string, std::string> v;
		return v;
	}

	static void LoadJson(const std::string& key, Js::ObjectAbstract* joa) {
		switch (joa->GetType()) {
		case Js::JPrimitiveString:
		{
			auto v = (Js::PrimitiveString*)joa;

			settings()[key] = v->value;
			break;
		}
		case Js::JPrimitive:
		{
			auto v = (Js::Primitive*)joa;

			settings()[key] = v->value;
			break;
		}
		case Js::JObject:
		{
			for (auto [k, v] : ((Js::Object*)joa)->objects)
				LoadJson(key + ":" + k, v);

			break;
		}
		default:
			Log::For<LocaleProvider>().Error("Unsupported Json object was passed.");
			break;
		}
	}
	static void LoadSettings(std::string name) {
		Js::Object* json = nullptr;
		try {
			json = (Js::Object*)Json::Read(name);

			for (auto [k, v] : json->objects)
				LoadJson(k, v);

			delete json;
		}
		catch (std::exception & e) {
			Log::For<LocaleProvider>().Error("Settings could not be loaded.");

			if (json)
				delete json;

			throw;
		}
	}

	template<typename T>
	static void Load(const std::string& name, std::function<void(T)> setter) {
		auto node = settings().find(name);
		if (node == settings().end()) {
			Log::For<Settings>().Error("Failed to load setting: ", name);
			return;
		}

		std::stringstream ss;
		ss << node._Ptr->_Myval.second;
		T v;
		ss >> v;

		setter(v);
	}
	template<typename T>
	static void Load(const std::string& name, Property<T>& (*selector)()) {
		Load(name, std::function([&](T v) { selector().Set(v); }));
	}

	template<typename T>
	static void Insert(Js::Object& jso, const std::string& name, Property<T>&(*selector)()) {
		jso.objects.insert({ name, JsonConvert::serialize(selector().Get()) });
	}

public:
	static void Load() {
		LoadSettings("settings.json");

		Load("language", &Settings::Language);
		Load("stateBufferLength", &Settings::StateBufferLength);

		Load("transitionStep", &Settings::TransitionStep);
		Load("rotationStep", &Settings::RotationStep);
		Load("scaleStep", &Settings::ScaleStep);
	}
	static void Save() {
		Js::Object json;

		Insert(json, "language", &Settings::Language);
		Insert(json, "stateBufferLength", &Settings::StateBufferLength);

		Insert(json, "transitionStep", &Settings::TransitionStep);
		Insert(json, "rotationStep", &Settings::RotationStep);
		Insert(json, "scaleStep", &Settings::ScaleStep);

		Json::Write("settings.json", &json);
	}
};
