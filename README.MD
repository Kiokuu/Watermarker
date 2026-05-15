# Watermarker

> A proof-of-concept tool for embedding watermarks into DirectX 11 executables at the binary level - without source code access or recompilation.

Final-year bachelor's thesis project (2024). Built to explore PE format manipulation, DLL injection, and DirectX 11 render hooking as a cohesive system.

---

## Example (Before&After)
[Example](/BeforeAfter.png)

## How it works

The project is split into two components that work together:

**Binder** parses a target `.exe` at the binary level, rewrites its PE import table to add a new DLL dependency (`Watermark.dll`), and writes the patched binary to `output.exe`. No source code or recompilation of the target is required - only the compiled executable.

**Watermark** is the injected DLL. On load, it hooks `IDXGISwapChain::Present` and `ResizeBuffers` via MinHook, intercepts each frame, and renders the configured watermark text using a custom Direct3D 11 pipeline before passing control back to the original call.

```
[Binder.exe]                         [output.exe at runtime]
  │                                       │
  ├─ Parse PE headers                     ├─ Windows loads Watermark.dll (injected import)
  ├─ Rewrite IMAGE_IMPORT_DESCRIPTOR      ├─ Watermark hooks IDXGISwapChain::Present
  ├─ Inject Watermark.dll import entry    ├─ Each frame: render watermark overlay
  └─ Write patched binary → output.exe   └─ Call original Present()
```

---

## Technical highlights

- **PE format manipulation** - Parses `IMAGE_DOS_HEADER`, `IMAGE_NT_HEADERS`, `IMAGE_SECTION_HEADER`, and `IMAGE_IMPORT_DESCRIPTOR` structures by hand. Rewrites the import table and recalculates section sizes, virtual addresses, and file alignment to produce a valid executable.
- **DirectX 11 render hook** - Hooks `IDXGISwapChain::Present` and `ResizeBuffers` using MinHook. Manages a D3D11 state block to save and restore render state around the watermark draw call, avoiding corruption of the host application's render pipeline.
- **Custom HLSL shader pipeline** - Watermark text is rasterised with CImg and uploaded as a texture; a minimal vertex/pixel shader pair renders it as a screen-space overlay each frame.
- **ImGui tooling UI** - Binder exposes a DirectX 11–backed ImGui interface with a file picker (ImGuiFileDialog) for selecting the target executable and entering watermark text.
- **x64 code generation** - Zasm is available for assembly-level patching (used in earlier reassembly-based approach, preserved in `.old/`).

---

## Project structure

```
.
├── Binder/               # GUI tool - parses PE and rewrites import table
│   ├── ExecutableFile    # PE parser and import table rewriter
│   ├── GUI/              # ImGui wrapper (handler + draw)
│   ├── ImageSection.h    # PE section abstraction
│   ├── ImportedModule.h  # Imported DLL/function abstraction
│   └── ExportedModule.h  # Exported function abstraction
│
├── Watermark/            # Injected DLL - hooks Present, renders overlay
│   ├── D3D11RenderHook   # IDXGISwapChain::Present and ResizeBuffers hooks
│   ├── D3D11StateBlock   # Saves/restores D3D11 render state
│   ├── D3D11WatermarkShader.h  # Embedded HLSL shader source
│   └── Vertex.h          # Vertex layout for overlay quad
│
├── DX11TestApplication/  # Public-domain DX11 app for testing (see attribution)
├── Licenses/             # Third-party library licences
├── .old/                 # Earlier reassembly-based approach (reference only)
├── CMakeLists.txt
├── CMakePresets.json
└── Doxyfile
```

---

## Building

### Prerequisites

| Requirement | Version |
|---|---|
| CMake | 3.19 or later |
| Visual Studio | 2022 recommended |
| Windows 10 SDK | Any recent version |
| DirectX 11 | Included with Windows SDK |

### Steps

```bash
# 1. Clone with submodules (ImGui, ImGuiFileDialog, CImg)
git clone --recurse-submodules https://github.com/<you>/Watermarker.git

# 2. Open in Visual Studio
#    Build menu → Build All

# Binaries are output to:
#    bin\AMD64\RelWithDebInfo\
```

MinHook and Zasm are fetched automatically by CMake at configure time via `FetchContent`.

---

## Usage

1. Run `Binder.exe`
2. Select the target DirectX 11 executable using the file picker
3. Enter the watermark text
4. Click **Watermark** - `output.exe` is written alongside the original
5. Run `output.exe` (or replace the original) - the watermark renders as an overlay

The included `DX11TestApplication.exe` (built alongside the project) is a ready-made test target.

---

## Known issues

This was a final-year thesis project and has not been maintained since submission. Known areas for improvement:
- **Memory leaks** - some resources are not correctly released, `D3D11RenderHook` and `ExecutableFile` destructors need auditing
- **Error handling** - several failure paths are silent or use unguarded exceptions
- **Usability** - due to this being a final-year project, many corners were cut in favour of time. The UI is primitive.

These are documented honestly rather than fixed, as the repository is kept as a reference.

---

## Original Concept

The orignal concept for this program was to provide a proof-of-concept for easy post-compilation protection of executables, as a form of DRM.
Originally, my approach was to completely recompile the executable and weave in the watermark code, in addition to further anti-tamper measures,
however, the scope was reduced due to time restraints and complexity restraints. There are remnants of this approach preserved in the `.old` folder.


## Libraries

| Library | Use | Licence |
|---|---|---|
| [ImGui](https://github.com/ocornut/imgui) | Binder GUI | MIT |
| [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog) | File picker in Binder | MIT |
| [MinHook](https://github.com/TsudaKageyu/minhook) | IDXGISwapChain hooking | BSD 2-Clause |
| [CImg](https://github.com/GreycLab/CImg) | Watermark text rasterisation | CeCILL / LGPL |
| [Zasm](https://github.com/zyantific/zasm) | x86/x64 assembly generation | MIT |

Licence files are in [`Licenses/`](Licenses/).

---

## Attribution

`DX11TestApplication` is based on [BeginnerDirect3D11](https://github.com/kevinmoran/BeginnerDirect3D11/) by Kevin Moran, released into the public domain.
