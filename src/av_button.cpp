#include <av_root/av_button.hpp>

namespace avR
{
    AvButton::AvButton(std::string label) : UiComponent(label), m_label(std::move(label))
    {
    }

    AvButton::~AvButton()
    {
        m_label.clear();
    }

    void AvButton::draw()
    {
        ImGui::PushID(this);
        if (ImGui::Button(m_label.c_str()))
        {
            fire_click();
        }
        ImGui::PopID();
    }
} // namespace avR
