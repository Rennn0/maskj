# arvis

Cross-platform (Windows / Linux / macOS) C++20 HTTP client / request inspector with a native GUI.

- Type a URL, pick **GET** or **POST**, hit **Send**
- Requests run off the UI thread so the window stays responsive
- Response body is shown in the UI and saved under `responses.dev/`

Built with **libcurl**, **Dear ImGui**, and **GLFW + OpenGL3**.

---

## Prerequisites

### Both platforms

| Tool | Notes |
|------|--------|
| **Git** | Clone the repo and fetch submodules / FetchContent deps |
| **CMake ≥ 3.24** | Presets in `CMakePresets.json` |
| **C++20 compiler** | MSVC (VS 2022) on Windows; GCC/Clang on Linux |
| **vcpkg submodule** | Provides **libcurl** — must be initialized (see below) |
| **Network** | First configure downloads GLFW, ImGui (FetchContent), and builds curl via vcpkg |

### Linux (Ubuntu / Debian)

The Linux presets use the **Ninja** generator — install it or configure will fail.

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  pkg-config \
  curl zip unzip tar \
  libgl1-mesa-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev libxkbcommon-dev
```

| Package group | Why |
|---------------|-----|
| `build-essential` | `g++` and related toolchain |
| `cmake`, `ninja-build` | Configure + build (Ninja is **required** by the linux presets) |
| `git`, `curl`, `zip`, `unzip`, `tar` | Clone, FetchContent, and vcpkg package builds |
| `pkg-config` + X11/Wayland + OpenGL `-dev` packages | GLFW needs these headers/libs at configure/link time |

### Windows

- **Visual Studio 2022** with the **Desktop development with C++** workload
- **CMake** (comes with VS, or install separately ≥ 3.24)
- **Git** for Windows

Open a **Developer PowerShell / x64 Native Tools** prompt, or use a terminal where `cmake` and the MSVC toolchain are on `PATH`.

---

## Clone and init submodules

`external/vcpkg` is a **git submodule**. Without it, configure fails looking for:

`external/vcpkg/scripts/buildsystems/vcpkg.cmake`

```bash
git clone https://github.com/Rennn0/arvis.git
cd arvis

# Required — pulls vcpkg (and any nested submodules)
git submodule update --init --recursive
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

---

## Build

### Linux

```bash
cmake --preset linux-debug      # or: linux-release
cmake --build --preset linux-debug
```

Binary: `build/linux-debug/arvis` (or `build/linux-release/arvis`)

Run:

```bash
./build/linux-debug/arvis
```

### Windows

```powershell
cmake --preset windows
cmake --build --preset windows-debug    # or: windows-release
```

Binary: `build/windows/Debug/arvis.exe` (or `build/windows/Release/arvis.exe`)

Run:

```powershell
.\build\windows\Debug\arvis.exe
```

---

## Notes

- **First configure is slow** — vcpkg builds libcurl; FetchContent clones GLFW and ImGui.
- After editing `CMakeLists.txt` (e.g. adding a `.cpp`), **re-run configure** (`cmake --preset …`), not only `--build`. Otherwise new sources may not be compiled.
- On Windows, if linking fails with `LNK1168` (cannot open `arvis.exe` for writing), close a running instance of the app and rebuild.
- `build/` and `responses.dev/` are git-ignored.
- Project docs for contributors: see [`CLAUDE.md`](CLAUDE.md).
