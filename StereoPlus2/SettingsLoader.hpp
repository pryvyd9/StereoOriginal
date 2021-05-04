#pragma once
#include "FileManager.hpp"
#include "Localization.hpp"

class SettingsLoader {
	static std::map<std::string, std::string>& settings() {
		static std::map<std::string, std::string> v;
		return v;
	}

	static const Log& Logger() {
		static Log v = Log::For<SettingsLoader>("settingsLoaderLog");
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
		case Js::JArray:
		{
			for (size_t i = 0; i < ((Js::Array*)joa)->objects.size(); i++) {
				std::stringstream ss;
				ss << i;
				LoadJson(key + ":" + ss.str(), ((Js::Array*)joa)->objects[i]);
			}

			break;
		}
		default:
			Logger().Error("Unsupported Json object was passed.");
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
			Logger().Error("Settings could not be loaded.");

			if (json)
				delete json;

			throw;
		}
	}

	template<typename T>
	static void Load(const std::string& name, std::function<void(T)> setter) {
		auto node = settings().find(name);
		if (node == settings().end()) {
			Logger().Error("Failed to load setting: ", name);
			return;
		}

		std::stringstream ss;
		ss << node._Ptr->_Myval.second;
		T v;
		ss >> v;

		setter(v);
	}
	template<>
	static void Load(const std::string& name, std::function<void(glm::vec4)> setter) {
		static const int size = sizeof(glm::vec4) / sizeof(float);

		std::string nodes[size];

		for (size_t i = 0; i < size; i++) {
			std::stringstream ss;
			ss << i;
			auto node = settings().find(name + ":" + ss.str());
			if (node == settings().end()) {
				Logger().Error("Failed to load setting: ", name, ":", i);
				return;
			}
			nodes[i] = node._Ptr->_Myval.second;
		}
		
		glm::vec4 v;
		for (size_t i = 0; i < size; i++) {
			std::stringstream ss;
			ss << nodes[i];
			ss >> ((float*)&v)[i];
		}

		setter(v);
	}

	template<typename T>
	static void Load(const std::string& name, Property<T>& (*selector)()) {
		Load(name, std::function([&](T v) { selector() = v; }));
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
		Load(&Settings::LogFileName);
		Load(&Settings::PPI);

		Load(&Settings::UseDiscreteMovement);
		Load(&Settings::TranslationStep);
		Load(&Settings::RotationStep);
		Load(&Settings::ScalingStep);
		Load(&Settings::MouseSensivity);

		Load(&Settings::ColorLeft);
		Load(&Settings::ColorRight);
		Load(&Settings::DimmedColorLeft);
		Load(&Settings::DimmedColorRight);
		Load(&Settings::CustomRenderWindowAlpha);

		Load(&Settings::ShouldMoveCrossOnSinePenModeChange);
	}
	static void Save() {
		Js::Object json;

		Insert(json, &Settings::Language);
		Insert(json, &Settings::StateBufferLength);
		Insert(json, &Settings::LogFileName);
		Insert(json, &Settings::PPI);

		Insert(json, &Settings::UseDiscreteMovement);
		Insert(json, &Settings::TranslationStep);
		Insert(json, &Settings::RotationStep);
		Insert(json, &Settings::ScalingStep);
		Insert(json, &Settings::MouseSensivity);

		Insert(json, &Settings::ColorLeft);
		Insert(json, &Settings::ColorRight);
		Insert(json, &Settings::DimmedColorLeft);
		Insert(json, &Settings::DimmedColorRight);
		Insert(json, &Settings::CustomRenderWindowAlpha);

		Insert(json, &Settings::ShouldMoveCrossOnSinePenModeChange);

		Json::Write("settings.json", &json);
	}
};
