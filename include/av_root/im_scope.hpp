#pragma once

#include "imgui.h"

namespace avR
{
    /// @brief RAII guard for an ImGui ID scope. Pushes on construction, pops on
    ///        destruction, so a component can never leak an unbalanced PushID —
    ///        the exact bug that makes sibling widgets with identical labels
    ///        steal each other's clicks.
    class ScopedId
    {
    public:
        explicit ScopedId(const void *ptr) { ImGui::PushID(ptr); }
        explicit ScopedId(int i) { ImGui::PushID(i); }
        ~ScopedId() { ImGui::PopID(); }

        ScopedId(const ScopedId &) = delete;
        ScopedId &operator=(const ScopedId &) = delete;
    };

    /// @brief RAII guard for ImGui style pushes. Colors and style-vars live on
    ///        two independent ImGui stacks; this counts how many of each were
    ///        pushed and pops exactly that many in the destructor. Callers never
    ///        hand-count PopStyleColor(n) / PopStyleVar(n) again — the single
    ///        most error-prone part of writing an immediate-mode widget.
    ///
    /// The push/pop pairs must nest, and this guarantees they do: styles pushed
    /// through the guard are unwound when the guard leaves scope, after the
    /// widget(s) drawn under them.
    ///
    /// Usage:
    ///     ScopedStyle style;
    ///     style.color(ImGuiCol_Button, bg)
    ///          .var(ImGuiStyleVar_FrameRounding, 4.0f);
    ///     ImGui::Button("ok");   // pops happen automatically at end of scope
    class ScopedStyle
    {
    public:
        ScopedStyle() = default;
        ~ScopedStyle()
        {
            // Order between the two stacks is irrelevant — they are independent.
            if (this->colors)
                ImGui::PopStyleColor(this->colors);
            if (this->vars)
                ImGui::PopStyleVar(this->vars);
        }

        ScopedStyle(const ScopedStyle &) = delete;
        ScopedStyle &operator=(const ScopedStyle &) = delete;

        /// @brief Push a color; chainable.
        ScopedStyle &color(ImGuiCol idx, const ImVec4 &value)
        {
            ImGui::PushStyleColor(idx, value);
            ++this->colors;
            return *this;
        }

        /// @brief Push a 2-component style var (e.g. FramePadding); chainable.
        ScopedStyle &var(ImGuiStyleVar idx, const ImVec2 &value)
        {
            ImGui::PushStyleVar(idx, value);
            ++this->vars;
            return *this;
        }

        /// @brief Push a scalar style var (e.g. FrameRounding); chainable.
        ScopedStyle &var(ImGuiStyleVar idx, float value)
        {
            ImGui::PushStyleVar(idx, value);
            ++this->vars;
            return *this;
        }

    private:
        int colors = 0;
        int vars = 0;
    };
} // namespace avR
