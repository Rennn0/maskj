#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <av_root/root.hpp>

namespace avR
{
    /// @brief Base of every UI element. Composite node: may hold children and/or
    ///        render itself. Rendered once per frame via Draw().
    class UiComponent : protected AvRoot
    {
    public:
        UiComponent(const UiComponent &) = delete;
        UiComponent &operator=(const UiComponent &) = delete;
        virtual ~UiComponent() = default;

        explicit UiComponent(std::string id = {});

        using AvRoot::log_error;
        using AvRoot::log_info;

        virtual void draw() = 0;

        /// @brief Layout negotiation used by resizable containers: a parent may
        ///        impose a size for the current frame (set_layout_size) and query
        ///        a child's preferred size. Leaf components ignore both.
        virtual void set_layout_size(const ImVec2 &size) { (void)size; }
        virtual ImVec2 preferred_size() const { return ImVec2(0.0f, 0.0f); }

        UiComponent *add_child(std::unique_ptr<UiComponent> child);
        UiComponent &set_on_click(std::function<void()> handler);
        const std::string &get_id() const noexcept;

    protected:
        void fire_click() const;
        void draw_children();

        /// @brief Read access to the child list for layout components that need to
        ///        position children themselves (e.g. horizontal rows).
        const std::vector<std::unique_ptr<UiComponent>> &children() const noexcept
        {
            return m_children;
        }

    private:
        std::string m_id;
        std::vector<std::unique_ptr<UiComponent>> m_children;
        std::function<void()> m_onClick;
    };
} // namespace avR
