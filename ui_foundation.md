# UI component foundation — design guide

Goal: a small, extensible component tree (`UiComponent` → `Button`, `Input`,
`Select`, `GridLayout`, …) rendered with ImGui, so your event loop collapses to
essentially one line: `root->Draw();`.

This is a reference. Write the code yourself; nothing here is applied to the repo.

---

## 1. The one idea that makes this clean: a retained tree over an immediate-mode library

ImGui is **immediate-mode** — there are no persistent widget objects; you re-issue
`ImGui::Button(...)` every frame and it returns `true` on the frame it was clicked.

You are building a **retained** model: `UiComponent` objects that persist, hold
configuration (labels, columns, bound data, click handlers) and a child list.

The bridge between the two is `Draw()`:

- You **build the tree once**, before the loop (buttons, layouts, bindings).
- **Each frame** you call `root->Draw()`, which walks the tree and *translates* it
  into immediate-mode ImGui calls.
- User interactions (a click, a text edit) are detected *inside* `Draw()` and
  dispatched to your callbacks immediately — so there is no separate event queue,
  no dispatch phase, no polling in the loop.

Two rules that follow directly, and that beginners always get wrong:

1. **Build the tree once, not every frame.** Rebuilding per frame is wasteful and
   destroys transient widget state (text cursor position, active-drag, etc.).
2. **`Draw()` is not `const`.** Drawing legitimately mutates state (bound values,
   scroll, selection). Don't mark it const.

This is the classic **Composite** pattern: a uniform `UiComponent` interface, with
leaf components (`Button`) and composite components (`GridLayout`) treated the same
by their parent.

---

## 2. The base class `UiComponent`

```cpp
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace avR
{
    /// @brief Base of every UI element. Composite node: may hold children and/or
    ///        render itself. Rendered once per frame via Draw().
    class UiComponent
    {
    public:
        using ClickHandler = std::function<void()>;

        /// @param id stable identifier; also used to disambiguate ImGui widgets
        explicit UiComponent(std::string id = {})
            : m_id(std::move(id))
        {
        }

        // Polymorphic base -> virtual destructor is MANDATORY (see gotcha #1).
        virtual ~UiComponent() = default;

        // Owns children + handlers -> non-copyable. (Movable isn't needed: the tree
        // is held through unique_ptr, so nodes are only ever moved by pointer.)
        UiComponent(const UiComponent &) = delete;
        UiComponent &operator=(const UiComponent &) = delete;

        /// @brief Emit this component (and, for containers, its children) to ImGui.
        ///        Pure virtual: every concrete component must define how it renders.
        virtual void Draw() = 0;

        /// @brief Take ownership of a child; returns a NON-owning pointer so callers
        ///        can keep configuring it. Prefer Emplace() below for typed access.
        UiComponent *AddChild(std::unique_ptr<UiComponent> child)
        {
            m_children.push_back(std::move(child));
            return m_children.back().get();
        }

        /// @brief Construct a child in place and return a typed reference to it.
        ///        auto &btn = grid.Emplace<Button>("OK"); btn.SetOnClick(...);
        template <typename T, typename... Args>
        T &Emplace(Args &&...args)
        {
            static_assert(std::is_base_of_v<UiComponent, T>,
                          "Emplace<T>: T must derive from UiComponent");
            auto child = std::make_unique<T>(std::forward<Args>(args)...);
            T &ref = *child;
            m_children.push_back(std::move(child));
            return ref;
        }

        /// @brief Optional click behavior. Components that support clicks invoke it
        ///        from their Draw() via FireClick().
        UiComponent &SetOnClick(ClickHandler handler)
        {
            m_onClick = std::move(handler);
            return *this; // enables one-liner chaining
        }

        const std::string &Id() const noexcept { return m_id; }

    protected:
        /// @brief Container helper: draw every child in order.
        void DrawChildren()
        {
            for (const auto &child : m_children)
                child->Draw();
        }

        /// @brief Leaf helper: invoke the click handler if one was set.
        void FireClick() const
        {
            if (m_onClick)
                m_onClick();
        }

        std::string m_id;
        std::vector<std::unique_ptr<UiComponent>> m_children;
        ClickHandler m_onClick;
    };
} // namespace avR
```

### Why these choices

- **`virtual ~UiComponent() = default;`** — the moment a class has a virtual
  function and you delete instances through a `UiComponent*` (which the child vector
  does), the destructor must be virtual, or derived destructors won't run → leaks /
  UB. This is non-negotiable for a polymorphic base.
- **`std::unique_ptr` children** — the parent *owns* its subtree; destroying the root
  tears down everything automatically. No manual `delete`, no ownership ambiguity.
- **`AddChild` returns a non-owning `UiComponent*`; `Emplace<T>` returns `T&`.** You
  keep the interface you asked for (`AddChild`) but also get a type-preserving
  builder so you can configure the concrete type fluently without a downcast.
- **`std::function` click handler on the base** — behavior is *configured per
  instance* ("configured onclick behavior"), which is exactly what `std::function`
  models. Putting it on the base keeps the interface uniform; leaves that don't click
  simply never call `FireClick()`. (Stricter alternative in §7.)
- **Non-copyable** — a node owns unique children and captures state in handlers;
  copying it has no sensible meaning. Deleting copy makes misuse a compile error.
- **`SetOnClick` returns `*this`** — lets you chain configuration in one expression.

---

## 3. Concrete components

### Button (leaf)

```cpp
// button.hpp
#pragma once
#include <av_root/ui_component.hpp>
#include <string>

namespace avR
{
    class Button : public UiComponent
    {
    public:
        explicit Button(std::string label)
            : UiComponent(label), m_label(std::move(label)) {}

        void Draw() override;

    private:
        std::string m_label;
    };
}
```

```cpp
// button.cpp
#include <av_root/button.hpp>
#include "imgui.h"

namespace avR
{
    void Button::Draw()
    {
        ImGui::PushID(this);                 // unique ID even if labels repeat
        if (ImGui::Button(m_label.c_str()))  // true only on the click frame
            FireClick();                     // dispatch immediately — no queue
        ImGui::PopID();
    }
}
```

### Input (leaf, with two-way data binding)

Binding = the component points at a value your app owns and keeps it in sync.

```cpp
// input.hpp
#pragma once
#include <av_root/ui_component.hpp>
#include <functional>
#include <string>

namespace avR
{
    class Input : public UiComponent
    {
    public:
        /// @param bound non-owning pointer to the string this input edits.
        ///              MUST outlive the component (see gotcha #4).
        Input(std::string label, std::string *bound)
            : UiComponent(label), m_label(std::move(label)), m_bound(bound) {}

        Input &OnChange(std::function<void(const std::string &)> cb)
        {
            m_onChange = std::move(cb);
            return *this;
        }

        void Draw() override;

    private:
        std::string m_label;
        std::string *m_bound;                              // two-way bound value
        std::function<void(const std::string &)> m_onChange;
    };
}
```

```cpp
// input.cpp
#include <av_root/input.hpp>
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"   // ImGui::InputText overload for std::string

namespace avR
{
    void Input::Draw()
    {
        ImGui::PushID(this);
        // The std::string overload writes edits straight into *m_bound.
        if (ImGui::InputText(m_label.c_str(), m_bound) && m_onChange)
            m_onChange(*m_bound);
        ImGui::PopID();
    }
}
```

> The `std::string` overload of `InputText` lives in `misc/cpp/imgui_stdlib.cpp`
> in the ImGui repo. Add it to your `imgui` static library in CMake — see §6.

### GridLayout (composite)

```cpp
// grid_layout.hpp
#pragma once
#include <av_root/ui_component.hpp>

namespace avR
{
    class GridLayout : public UiComponent
    {
    public:
        GridLayout(std::string id, int columns)
            : UiComponent(std::move(id)), m_columns(columns) {}

        void Draw() override;

    private:
        int m_columns;
    };
}
```

```cpp
// grid_layout.cpp
#include <av_root/grid_layout.hpp>
#include "imgui.h"

namespace avR
{
    void GridLayout::Draw()
    {
        ImGui::PushID(this);
        if (ImGui::BeginTable(m_id.c_str(), m_columns))
        {
            for (const auto &child : Children()) // small accessor, or make DrawChildren cell-aware
            {
                ImGui::TableNextColumn();
                child->Draw();
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }
}
```

> `GridLayout` needs to place each child in its own cell, so it iterates children
> itself instead of calling the flat `DrawChildren()`. Either expose a
> `const std::vector<...>& Children()` accessor, or add a protected
> `DrawChildrenWith(std::function<void(UiComponent&)>)` hook. A plain vertical
> `Stack` layout, by contrast, can just call `DrawChildren()`.

`Select` and your list components follow the same shape: hold config + a binding,
render with the matching ImGui call (`BeginCombo`/`Selectable`, `BeginListBox`), fire
a callback on change.

---

## 4. How the event loop becomes clean

**Build once, above the loop:**

```cpp
std::string urlBuffer = "https://api.xati.org/health";

auto root = std::make_unique<avR::GridLayout>("main", 2);

root->Emplace<avR::Input>("URL", &urlBuffer);          // bound to urlBuffer

auto &send = root->Emplace<avR::Button>("Send");
send.SetOnClick([&]{ startRequest(urlBuffer); });      // captures live state

auto &clear = root->Emplace<avR::Button>("Clear");
clear.SetOnClick([&]{ urlBuffer.clear(); });
```

**The loop shrinks to this:**

```cpp
while (!glfwWindowShouldClose(window))
{
    glfwPollEvents();
    // ... collect async results, NewFrame ...

    ImGui::Begin("arvis", /* flags */);
    root->Draw();          // <-- entire UI + all click/change dispatch, one call
    ImGui::End();

    // ... Render / SwapBuffers ...
}
```

Everything that used to be inline `if (ImGui::Button(...)) { ... }` spaghetti now
lives in the tree as data + handlers. Adding a widget is `Emplace<X>(...)` + a
callback; it never touches the loop. That is the extensibility you're after.

---

## 5. Where things live (namespaces / files)

You put `UiComponent` in `av_root` / namespace `avR`, so keep the whole component
family there for consistency:

```
scripts/new_class.ps1 av_root Button      -Namespace avR
scripts/new_class.ps1 av_root Input       -Namespace avR
scripts/new_class.ps1 av_root GridLayout  -Namespace avR
```

(If you later decide the widgets belong under the UI module instead, move them to
`av_ui` / `avUi` — but pick one and be consistent; mixing is what bit you before.)

---

## 6. CMake: enable `std::string` inputs

`Input` uses the `std::string` overload of `ImGui::InputText`, which is not in
ImGui's core. Add its source to your existing `imgui` static library target:

```cmake
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp   # <-- add this
)
```

The `#include "misc/cpp/imgui_stdlib.h"` works because your `imgui` target already
adds `${imgui_SOURCE_DIR}` to its public include dirs.

---

## 7. Gotchas & best practices (the checklist)

1. **Virtual destructor** on `UiComponent` — mandatory for a polymorphic base
   deleted through base pointers. Already in the header above; don't drop it.
2. **ImGui ID collisions.** ImGui derives widget identity from the label string. Two
   buttons labelled "OK", or repeated rows in a list, will fight over the same ID
   (clicks/edits go to the wrong one). `ImGui::PushID(this)` / `PopID()` around each
   component's draw gives every instance a unique scope — cheapest robust fix. For
   list items, `PushID(index)` per row.
3. **Build the tree once.** Not in the loop. (Immediate-mode state lives in ImGui
   keyed by ID; your tree only holds config + bindings.)
4. **Binding & capture lifetimes.** `Input`'s bound `std::string*` and any variable a
   handler captures by reference must outlive the component tree. Prefer capturing
   things that live as long as the window (members, or values owned by the same scope
   as `root`). A dangling bind/capture is a use-after-free that ImGui will happily
   trigger on the next click.
5. **Keep handlers thin.** A click handler should call into a controller/service, not
   contain business logic. Components render; they don't own app behavior. This keeps
   the tree reusable and testable.
6. **`Draw()` is non-const and runs every frame.** Don't do expensive work in it
   (allocations, parsing). Precompute in the setup phase; cache in members.
7. **Leaves and `AddChild`.** In this transparent design a `Button` technically
   inherits `AddChild`. Just don't call it on leaves. If you want that to be a
   *compile-time* error instead of a silent no-op, use the split in the note below.

### Optional: stricter container/leaf split (Interface Segregation)

If you'd rather leaves *cannot* accept children at all, move `AddChild`/`Emplace`/
`m_children`/`DrawChildren` out of `UiComponent` into an intermediate
`UiContainer : UiComponent`, and derive only layouts (`GridLayout`, `Stack`) from
`UiContainer`; leaves (`Button`, `Input`) derive straight from `UiComponent`. Cleaner
typing, at the cost of two base types instead of one. For a small app the transparent
version in §2 is usually the better trade — start there, split later only if leaf
misuse actually happens.

### Optional: a `Window` component

Right now the loop does `ImGui::Begin/End` around `root->Draw()`. If you want windows
to be part of the tree too, make a `Window : UiComponent` whose `Draw()` wraps
`ImGui::Begin(...)` / `DrawChildren()` / `ImGui::End()`. Then even the `Begin/End`
leaves the loop and the loop truly becomes just `root->Draw();`.
```
