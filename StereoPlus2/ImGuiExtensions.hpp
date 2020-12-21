#pragma once
#include <stack>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_stdlib.h>

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
	static void HelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

    //struct ExampleAppLog
    //{
    //    ImGuiTextBuffer     Buf;
    //    ImGuiTextFilter     Filter;
    //    ImVector<int>       LineOffsets;        // Index to lines offset
    //    bool                ScrollToBottom;

    //    void    Clear() { Buf.clear(); LineOffsets.clear(); }

    //    void    AddLog(const std::string& str)
    //    {
    //        int old_size = Buf.size();
    //        Buf.append(str.c_str());
    //        for (int new_size = Buf.size(); old_size < new_size; old_size++)
    //            if (Buf[old_size] == '\n')
    //                LineOffsets.push_back(old_size);
    //        ScrollToBottom = true;
    //    }

    //    void    Draw(const char* title, bool* p_opened = NULL)
    //    {
    //        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    //        ImGui::Begin(title, p_opened);
    //        if (ImGui::Button("Clear")) Clear();
    //        ImGui::SameLine();
    //        bool copy = ImGui::Button("Copy");
    //        ImGui::SameLine();
    //        Filter.Draw("Filter", -100.0f);
    //        ImGui::Separator();
    //        ImGui::BeginChild("scrolling");
    //        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
    //        if (copy) ImGui::LogToClipboard();

    //        if (Filter.IsActive())
    //        {
    //            const char* buf_begin = Buf.begin();
    //            const char* line = buf_begin;
    //            for (int line_no = 0; line != NULL; line_no++)
    //            {
    //                const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
    //                if (Filter.PassFilter(line, line_end))
    //                    ImGui::TextUnformatted(line, line_end);
    //                line = line_end && line_end[1] ? line_end + 1 : NULL;
    //            }
    //        }
    //        else
    //        {
    //            ImGui::TextUnformatted(Buf.begin());
    //        }

    //        if (ScrollToBottom)
    //            ImGui::SetScrollHere(1.0f);
    //        ScrollToBottom = false;
    //        ImGui::PopStyleVar();
    //        ImGui::EndChild();
    //        ImGui::End();
    //    }
    //};

  //  static bool LogTextMultiline(std::string* str)
  //  {
  //      const char* label = "###logs";
  //      auto r = ImGui::InputTextMultiline(label, str, glm::vec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);

  //      ImGuiWindow* window = GetCurrentWindow();
  //      const ImGuiID id = window->GetID(label);
		//ImGuiContext& g = *GImGui;
		//ImGuiWindow* draw_window = g.CurrentWindow;
		////g.
  //      // We are only allowed to access the state if we are already the active widget.
  //      ImGuiInputTextState* state = GetInputTextState(id);

  //      if (!state->HasSelection())
		//	ImGui::SetScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY()));
		//	//draw_window->Scroll.y = FLT_MAX;

		//return r;
  //  }

}