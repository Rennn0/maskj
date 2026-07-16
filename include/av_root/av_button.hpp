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
            ImVec2 size = ImVec2(0.0f, 0.0f); ///< (0,0) => auto-fit label
            ImVec2 padding = ImVec2(12.0f, 6.0f); ///< frame padding around label
            ImVec4 background = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            ImVec4 hovered = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            ImVec4 active = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            ImVec4 text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            float rounding = 0.0f; ///< corner rounding in px
            float border = 0.0f;   ///< frame border thickness (0 = none)
        };

        explicit AvButton(std::string label, Style style);
        ~AvButton();

        ImVec2 preferred_size() const override;

    protected:
        void render() override;

    private:
        std::string label;
        Style style;
    };
} // namespace avR
