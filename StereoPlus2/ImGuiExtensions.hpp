#pragma once
#include <stack>
#include <imgui/imgui.h>
#include <imgui\imgui_internal.h>

namespace ImGui::Extensions {

	static std::stack<bool>& GetIsActive() {
		static std::stack<bool> val;
		return val;
	}
	static bool PushActive(bool isActive) {
		ImGui::Extensions::GetIsActive().push(isActive);
		if (!isActive) {
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		return true;
	}
	static void PopActive() {
		auto isActive = ImGui::Extensions::GetIsActive().top();
		ImGui::Extensions::GetIsActive().pop();

		if (!isActive) {
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}
	}
}