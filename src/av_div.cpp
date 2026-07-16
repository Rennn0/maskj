#include <av_root/av_div.hpp>

#include <cstddef>

namespace avR
{
    AvDiv::AvDiv(std::string id, Config config)
        : UiComponent(std::move(id)), m_config(config)
    {
    }

    AvDiv &AvDiv::configure(const Config &config)
    {
        m_config = config;
        return *this;
    }

    void AvDiv::set_layout_size(const ImVec2 &size)
    {
        m_layoutSize = size;
        m_hasLayoutSize = true;
    }

    ImVec2 AvDiv::preferred_size() const
    {
        return m_config.size;
    }

    void AvDiv::draw()
    {
        ImGui::PushID(this);

        // Apply configurable spacing/padding for the region's scope.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, m_config.padding);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(m_config.spacing, m_config.spacing));

        const bool hasBackground = m_config.background.w > 0.0f;
        if (hasBackground)
            ImGui::PushStyleColor(ImGuiCol_ChildBg, m_config.background);

        const ImGuiChildFlags childFlags =
            m_config.border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;

        // A resizable parent may have imposed a size on us this frame; otherwise
        // fall back to the configured size ((0,0) => fill available space).
        const ImVec2 size = m_hasLayoutSize ? m_layoutSize : m_config.size;

        if (ImGui::BeginChild(get_id().c_str(), size, childFlags))
        {
            layout_children();
        }
        ImGui::EndChild();

        if (hasBackground)
            ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        ImGui::PopID();

        // The imposed size lasts a single frame; the parent re-applies it each draw.
        m_hasLayoutSize = false;
    }

    void AvDiv::layout_children()
    {
        const auto &kids = children();
        if (kids.empty())
            return;

        const bool horizontal = (m_config.direction == Direction::Horizontal);

        // --- simple (non-resizable) flow ------------------------------------
        if (!m_config.resizable)
        {
            bool first = true;
            for (const std::unique_ptr<UiComponent> &child : kids)
            {
                if (horizontal && !first)
                    ImGui::SameLine();
                child->draw();
                first = false;
            }
            return;
        }

        // --- resizable: each leading child gets a stored main-axis extent plus
        //     a draggable splitter on its trailing edge; the last child fills.
        //     Seed extents once from each child's preferred size.
        if (m_extents.size() != kids.size())
        {
            m_extents.assign(kids.size(), 0.0f);
            for (std::size_t i = 0; i < kids.size(); ++i)
            {
                const ImVec2 pref = kids[i]->preferred_size();
                m_extents[i] = horizontal ? pref.x : pref.y;
            }
        }

        bool first = true;
        const auto gap = [&]()
        {
            if (horizontal && !first)
                ImGui::SameLine();
            first = false;
        };

        for (std::size_t i = 0; i < kids.size(); ++i)
        {
            const bool last = (i + 1 == kids.size());

            gap();
            if (last)
            {
                kids[i]->set_layout_size(ImVec2(0.0f, 0.0f)); // fill remaining
                kids[i]->draw();
                continue;
            }

            // Leading child: pin its main-axis extent, let the cross axis fill.
            kids[i]->set_layout_size(horizontal ? ImVec2(m_extents[i], 0.0f)
                                                : ImVec2(0.0f, m_extents[i]));
            kids[i]->draw();

            // Draggable splitter on the trailing edge.
            gap();
            ImGui::PushID(static_cast<int>(i));

            const ImVec2 region = ImGui::GetContentRegionAvail();
            const ImVec2 handle = horizontal
                                      ? ImVec2(m_config.splitter_thickness, region.y)
                                      : ImVec2(region.x, m_config.splitter_thickness);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.20f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
            ImGui::Button("##splitter", handle);
            ImGui::PopStyleColor(3);

            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                ImGui::SetMouseCursor(horizontal ? ImGuiMouseCursor_ResizeEW
                                                 : ImGuiMouseCursor_ResizeNS);

            if (ImGui::IsItemActive())
                m_extents[i] += horizontal ? ImGui::GetIO().MouseDelta.x
                                           : ImGui::GetIO().MouseDelta.y;

            // Clamp to the configured min/max.
            if (m_extents[i] < m_config.resize_min)
                m_extents[i] = m_config.resize_min;
            if (m_config.resize_max > 0.0f && m_extents[i] > m_config.resize_max)
                m_extents[i] = m_config.resize_max;

            ImGui::PopID();
        }
    }
} // namespace avR
