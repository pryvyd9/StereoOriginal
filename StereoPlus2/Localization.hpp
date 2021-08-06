#pragma once
#include "Json.hpp"
#include "InfrastructureTypes.hpp"
#include <stringapiset.h>
#include "FileManager.hpp"

namespace Locale {
	const std::string UA = "ua";
	const std::string EN = "en";
};
static class LocaleProvider {
	// Convert a wide Unicode string to an UTF8 string
	// Windows specific
	static std::string utf8_encode(const std::wstring& wstr) {
		if (wstr.empty()) return std::string();
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
		return strTo;
	}

	// Convert an UTF8 string to a wide Unicode String
	// Windows specific
	static std::wstring utf8_decode(const std::string& str) {
		if (str.empty()) return std::wstring();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
		return wstrTo;
	}

	static std::map<std::string, std::string>& localizations() {
		static std::map<std::string, std::string> v;
		return v;
	}

	static void LoadJson(const std::string& key, Jw::ObjectAbstract* joa) {
		switch (joa->GetType()) {
		case Jw::JPrimitiveString:
		{
			auto v = (Jw::PrimitiveString*)joa;

			localizations()[key] = utf8_encode(v->value);
			break;
		}
		case Jw::JObject:
		{
			for (auto [k, v] : ((Jw::Object*)joa)->objects)
				LoadJson(key + ":" + utf8_encode(k), v);

			break;
		}
		default:
			Log::For<LocaleProvider>().Error("Unsupported Json object was passed.");
			break;
		}
	}

	static void LoadLanguage(std::string name) {
		Jw::Object* json = nullptr;
		try {
			json = (Jw::Object*)FileManager::LoadLocaleFile("locales/" + name + ".json");

			for (auto [k, v] : json->objects)
				LoadJson(utf8_encode(k), v);

			delete json;
		}
		catch (std::exception & e) {
			Log::For<LocaleProvider>().Error("Locale could not be loaded.");

			if (json)
				delete json;

			throw;
		}
	}

public:
	static const std::string& Get(const std::string& name) {
		if (auto v = localizations().find(name); v != localizations().end())
			return v._Ptr->_Myval.second;

		Log::For<LocaleProvider>("localeProviderLog").Warning("Localization for ", name, " was not found.");
		return name;
	}
	static const char* GetC(const std::string& name) {
		return Get(name).c_str();
	}

	static bool Init() {
		if (Settings::Language().Get().empty()) {
			Log::For<LocaleProvider>().Error("Language not assigned.");
			return false;
		}

		Settings::Language().OnChanged().AddHandler([](std::string name) { LoadLanguage(name); });

		LoadLanguage(Settings::Language().Get());

		return true;
	}
};
