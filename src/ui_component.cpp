#include <av_root/ui_component.hpp>

namespace avR
{
    UiComponent::UiComponent(std::string id) : AvRoot("hjuh") ,m_id(std::move(id))
    {
    }

    UiComponent::~UiComponent()
    {
    }

    UiComponent *UiComponent::add_child(std::unique_ptr<UiComponent> child)
    {
        m_children.push_back(child);
        return m_children.back().get();
    }

    UiComponent &UiComponent::set_on_click(std::function<void()> handler)
    {
        m_onClick = std::move(handler);
        return *this;
    }

    const std::string &UiComponent::get_id() const noexcept
    {
        return m_id;
    }

    void UiComponent::fire_click() const
    {
        if (m_onClick)
            m_onClick();
    }

    void UiComponent::draw_children()
    {
        for (const std::unique_ptr<UiComponent> &child : m_children)
            child->draw();
    }
} // namespace avR
