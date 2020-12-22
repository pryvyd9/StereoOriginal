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


    static void StyleColorsStereo(ImGuiStyle* dst = nullptr)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImVec4* colors = style->Colors;
        style->WindowRounding = 0.0f;

        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.30f, 0.50f, 1.00f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
        colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
        colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
        colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_HeaderActive];
        colors[ImGuiCol_DockingPreview].w       = 0.7f;

        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

}