#include "content_browser_panel.hpp"

namespace ignite {

    ContentBrowserPanel::ContentBrowserPanel(const char *windowTitle)
        : IPanel(windowTitle)
    {
    }

    void ContentBrowserPanel::OnGuiRender()
    {
        ImGui::Begin("Content Browser");

        ImGuiWindow *window = ImGui::GetCurrentWindow();
        ImVec2 regionSize = ImGui::GetContentRegionAvail();

        const ImVec2 navbarBtSize = ImVec2(40.0f, 30.0f);
        const ImVec2 navbarSize = ImVec2(regionSize.x, 45.0f);
        // Navigation bar
        ImGui::BeginChild("##NAV_BUTTON_BAR", navbarSize, ImGuiChildFlags_Borders);

        ImGui::Button("<-", navbarBtSize);
        ImGui::SameLine();
        ImGui::Button("->", navbarBtSize);
        ImGui::SameLine();
        ImGui::Button("R", navbarBtSize);

        ImGui::EndChild();


        // Files
        ImGui::BeginChild("##FILE_LISTS");

        ImGui::EndChild();

        ImGui::End();
    }

}