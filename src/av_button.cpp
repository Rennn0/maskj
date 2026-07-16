#include <av_root/av_button.hpp>

namespace avR
{
    AvButton::AvButton(std::string label, Style style)
        : UiComponent(label), m_label(std::move(label)), m_style(style)
    {
    }

    AvButton::~AvButton()
    {
        m_label.clear();
    }

    void AvButton::draw()
    {
        ImGui::PushID(this);

        ImGui::PushStyleColor(ImGuiCol_Button, m_style.background);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_style.hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_style.active);
        ImGui::PushStyleColor(ImGuiCol_Text, m_style.text);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, m_style.rounding);

        if (ImGui::Button(m_label.c_str(), m_style.size))
        {
            log_info("clicked");
            fire_click();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::PopID();
    }
} // namespace avR
