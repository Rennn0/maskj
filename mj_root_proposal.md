# `mj_root/root.hpp` — proposed design

Scope: a small base/utility class `mjR::MjRoot` that provides name-tagged logging
(`logInfo`, `logError`) in the format:

```
[info](name)<msg>
[error](name)<msg>
```

You write the files yourself — this is only a reference.

---

## 1. Issues in the current draft

```cpp
namespace mjR
{
    class MjRoot
    {
        public:
        MjRoot();
        MjRoot(const char* name);
        ~MjRoot();

        private:
        const char* m_name = nullptr;
    };
}
```

1. **No `#pragma once`** (or include guard). This header will be included by many
   translation units → redefinition errors. Must be added.
2. **`const char* m_name` is a dangling-pointer trap.** It stores a *borrowed*
   pointer; if you construct from a temporary (`MjRoot("NetworkManager")` built
   from a `std::string`, or any string that outlives its scope), `m_name` dangles.
   A logger must *own* its name → use `std::string`.
3. **Two constructors (`MjRoot()` + `MjRoot(const char*)`) are redundant.** Collapse
   to one; use a default argument if you want a "nameless" fallback.
4. **The single-arg constructor should be `explicit`** to prevent accidental
   implicit conversion of a string into an `MjRoot`.
5. **The actual feature (`logInfo` / `logError`) is missing.** They should be
   `const` — logging doesn't mutate the object.
6. **`~MjRoot()` is declared but does nothing meaningful.** Let the compiler default
   it (see the virtual-destructor note in §5).

---

## 2. Recommended header (header-only)

Header-only is the right call for something this small: no `.cpp`, no CMake change,
and the trivial methods inline cleanly.

```cpp
#pragma once

#include <iostream>
#include <string>
#include <string_view>

namespace mjR
{
    /// @brief Common utility base providing simple, name-tagged logging.
    ///        Every line is formatted as: [level](name)<message>
    class MjRoot
    {
    public:
        /// @param name identifier shown in every log line from this instance
        explicit MjRoot(std::string name = "mjRoot")
            : m_name(std::move(name))
        {
        }

        /// @brief log an informational message -> stdout
        void logInfo(std::string_view msg) const
        {
            log(std::cout, "info", msg);
        }

        /// @brief log an error message -> stderr
        void logError(std::string_view msg) const
        {
            log(std::cerr, "error", msg);
        }

        /// @brief the name this instance tags its log lines with
        const std::string &name() const noexcept { return m_name; }

    private:
        /// @brief single formatting point for both levels (DRY)
        void log(std::ostream &out, std::string_view level,
                 std::string_view msg) const
        {
            out << '[' << level << "](" << m_name << ")<" << msg << ">\n";
        }

        std::string m_name;
    };
} // namespace mjR
```

---

## 3. Why these choices (OOP / C++ best practices)

- **`std::string m_name` + `std::move`** — the class owns its name; no lifetime
  coupling to the caller. `std::move` avoids a copy when constructing.
- **`explicit` single constructor with a default argument** — one clear way to
  build it, no implicit conversions, and still a no-arg option.
- **`std::string_view` parameters** — accepts `const char*`, `std::string`, and
  literals with zero allocation; the logger only reads the message.
- **`const` member functions** — logging is a read-only operation on the object;
  marking it `const` lets `const MjRoot` instances log and documents intent.
- **One private `log()` helper (DRY)** — `logInfo`/`logError` differ only in the
  level string and the target stream. Centralizing the format means the template
  `[level](name)<msg>` is defined in exactly one place.
- **`std::cerr` for errors, `std::cout` for info** — errors go to the error stream
  (unbuffered, not swallowed if stdout is redirected). This is a small upgrade over
  the current `NetworkManager`, which prints everything to `std::cout`.
- **`name()` accessor is `noexcept` and returns `const&`** — cheap, non-throwing,
  no copy.

---

## 4. How other classes should use it — prefer **composition** over inheritance

You said "all other classes will instantiate this root." That phrasing =
composition (has-a), which is also the better design here: a logger is a *utility a
class uses*, not a *type a class is*. Composition keeps public interfaces clean, adds
no vtable, and avoids fragile-base-class coupling.

Each class holds an `MjRoot` member initialized with its own name:

```cpp
// network_manager.hpp
#include <mj_root/root.hpp>

class NetworkManager
{
    // ...
private:
    mjR::MjRoot m_log{"NetworkManager"};   // <- instantiated per your wording
};
```

```cpp
// network_manager.cpp — replaces the current print_info / print_error helpers
m_log.logInfo("network manager ctr");
m_log.logError(curl_easy_strerror(res));
```

This lets you delete `NetworkManager::print_info` / `print_error` entirely and route
all logging through `m_log`.

### Inheritance alternative (only if you truly want an "is-a")

If you later decide subclasses *are* `MjRoot`s, use **`protected` inheritance** so the
logging API is available to the subclass but not leaked to the class's public users:

```cpp
class NetworkManager : protected mjR::MjRoot
{
public:
    NetworkManager() : mjR::MjRoot("NetworkManager") { logInfo("ctr"); }
};
```

If you go this route **and** ever delete a derived object through a `MjRoot*`, the
base needs a `virtual ~MjRoot() = default;`. For composition (recommended) you do
**not** want a virtual destructor — it would add a vtable pointer for no benefit.

---

## 5. Notes / optional extras

- **Thread safety:** your GUI already logs from a worker thread *and* the main
  thread. `std::cout` writes can interleave. If that bothers you, guard the write:

  ```cpp
  #include <mutex>
  // inside log():
  static std::mutex s_mtx;
  std::lock_guard<std::mutex> lk(s_mtx);
  out << '[' << level << "](" << m_name << ")<" << msg << ">\n";
  ```

- **Flushing:** `\n` doesn't flush `std::cout`, but `std::cerr` is unbuffered, so
  errors appear immediately. Use `std::endl` instead of `'\n'` only if you want info
  lines flushed eagerly too (slower).

- **If you prefer a split `.cpp`** (e.g. to keep `<iostream>` out of the header and
  cut compile time): move the method bodies to `src/root.cpp`, leave declarations in
  the header, and add `src/root.cpp` to the `maskj` sources in `CMakeLists.txt`. The
  existing `target_include_directories(... include)` already covers the header path.
  For this class size, header-only is simpler and fine.
```
