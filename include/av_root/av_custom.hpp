#pragma once

#include <functional>
#include <string>
#include <av_root/ui_component.hpp>

namespace avR
{
    /// @brief Leaf component whose rendering is delegated to a caller-supplied
    ///        callback. Escape hatch for app-specific, data-driven views (lists,
    ///        detail panes, ...) that don't warrant a dedicated component class.
    ///        The callback runs every frame inside this component's ID scope.
    class AvCustom : public UiComponent
    {
    public:
        using DrawFn = std::function<void()>;

        AvCustom(std::string id, DrawFn drawFn);

    protected:
        void render() override;

    private:
        DrawFn drawFn;
    };
} // namespace avR
