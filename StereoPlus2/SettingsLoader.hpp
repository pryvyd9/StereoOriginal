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

public:
	static void Load() {
		LoadSettings("settings.json");
		Load("crossSpeed", std::function([](float v) { Settings::CrossSpeed().Set(v); }));
		Load("language", std::function([](std::string v) { Settings::Language().Set(v); }));
		Load("stateBufferLength", std::function([](int v) { Settings::StateBufferLength().Set(v); }));
	}
	static void Save() {
		auto json = new Js::Object();

		json->objects.insert({ "crossSpeed", JsonConvert::serialize(Settings::CrossSpeed().Get()) });
		json->objects.insert({ "language", JsonConvert::serialize(Settings::Language().Get()) });
		json->objects.insert({ "stateBufferLength", JsonConvert::serialize(Settings::StateBufferLength().Get()) });

		Json::Write("settings.json", json);
		delete json;
	}
};
