#pragma once

#include <string>
#include <av_root/ui_component.hpp>

namespace avR
{
    class AvButton : public UiComponent
    {
    public:
        /// @brief Visual tunables. Defaults mirror the ImGui dark theme so an
        ///        unstyled button looks like before.
        struct Style
        {
            ImVec2 size = ImVec2(0.0f, 0.0f);                      ///< (0,0) => auto-fit label
            ImVec4 background = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            ImVec4 hovered = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            ImVec4 active = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            ImVec4 text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            float rounding = 0.0f;                                 ///< corner rounding in px
        };

        explicit AvButton(std::string label, Style style = {});
        ~AvButton();

        /// @brief Mutable access for tweaks: btn->style().rounding = 6.f;
        Style &style() noexcept { return m_style; }

        void draw() override;

    private:
        std::string m_label;
        Style m_style;
    };
} // namespace avR
