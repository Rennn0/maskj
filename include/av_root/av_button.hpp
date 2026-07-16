#pragma once

#include <string>
#include <av_root/ui_component.hpp>

namespace avR
{
    class AvButton : public UiComponent
    {
    public:
        explicit AvButton(std::string label);
        ~AvButton();

        void draw() override;

    private:
        std::string m_label;
    };
} // namespace avR
