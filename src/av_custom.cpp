#include <av_root/av_custom.hpp>

namespace avR
{
    AvCustom::AvCustom(std::string id, DrawFn drawFn)
        : UiComponent(std::move(id)), m_drawFn(std::move(drawFn))
    {
    }

    void AvCustom::draw()
    {
        if (!m_drawFn)
            return;

        ImGui::PushID(this);
        m_drawFn();
        ImGui::PopID();
    }
} // namespace avR
