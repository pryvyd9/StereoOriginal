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
	static void Load(Property<T>& (*selector)()) {
		Load(Settings::Name(selector), selector);
	}

	template<typename T>
	static void Insert(Js::Object& jso, const std::string& name, Property<T>&(*selector)()) {
		jso.objects.insert({ name, JsonConvert::serialize(selector().Get()) });
	}

	template<typename T>
	static void Insert(Js::Object& jso, Property<T>& (*selector)()) {
		jso.objects.insert({ Settings::Name(selector), JsonConvert::serialize(selector().Get()) });
	}

public:
	
	static void Load() {
		LoadSettings("settings.json");
		
		Load(&Settings::Language);
		Load(&Settings::StateBufferLength);

		Load(&Settings::UseDiscreteMovement);
		Load(&Settings::TransitionStep);
		Load(&Settings::RotationStep);
		Load(&Settings::ScaleStep);
	}
	static void Save() {
		Js::Object json;

		Insert(json, &Settings::Language);
		Insert(json, &Settings::StateBufferLength);

		Insert(json, &Settings::UseDiscreteMovement);
		Insert(json, &Settings::TransitionStep);
		Insert(json, &Settings::RotationStep);
		Insert(json, &Settings::ScaleStep);

		Json::Write("settings.json", &json);
	}
};
