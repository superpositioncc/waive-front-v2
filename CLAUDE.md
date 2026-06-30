# WAIVE-FRONT V2

## What this is

An audio-reactive *visual* tool built on **DPF** (DISTRHO Plugin Framework) — **not** JUCE.
It ships two ways:

- **Plugin** (`WAIVE-FRONT-V2`): VST2 / VST3 / AU (macOS) / JACK. The audio path is a pure
  pass-through (`WaiveFrontPlugin::run()` just `memcpy`s input→output); the plugin exists only to
  host the visuals.
- **Standalone** (`WAIVE-FRONT-STANDALONE`): a non-VST, **no-audio** app that hosts the same
  control panel + viewer via DGL's `ImGuiStandaloneWindow`. Lives on the `standalone` branch.

The visual layer: an OpenGL/shader video compositor (the "Viewer" window) + an ImGui control panel,
fed by file-based media in `~/Documents/WAIVE` (`%USERPROFILE%\Documents\WAIVE` on Windows),
FFmpeg video decoding, and OSC control on UDP port 8000. `palettegen` extracts a 5-colour palette
per frame.

## Architecture / key files

- `src/WaiveFrontController.hpp` — **the shared brain.** All audio-agnostic state + logic: the
  `parameters[]` array, data sources, video loaders, OSC server, automatic-mode engine, and the
  ImGui control-panel rendering (`drawControls`). Used by *both* the plugin UI and the standalone.
- `src/WaiveFrontPluginUI.cpp` — thin DPF `UI` shell; owns the ViewerWindow, delegates to the controller.
- `src/WaiveFrontPlugin.cpp` — DPF DSP (pass-through + parameter plumbing).
- `src/standalone/WaiveFrontStandalone.cpp` — `main()` + `ImGuiStandaloneWindow` for the standalone.
- `src/viewer/ViewerWindow.cpp`, `src/viewer/ViewerWidget.cpp` — the OpenGL viewer (shader render).
- `src/util/MacGL.mm` / `MacGL.h` — macOS-only native helpers (see below). No-op stubs on other OSes.
- `src/video/VideoLoader.cpp` — FFmpeg decode + swscale → RGB + palettegen filter graph.
- `src/data/`, `src/osc/`, `src/shader/`, `src/assets/shaders/` — data model, OSC server, shader plumbing.

**Code convention:** this project `#include`s `.cpp` files into each other (e.g. `ViewerWindow.cpp`
includes `ViewerWidget.cpp`). Only a few `.cpp`/`.c` files are listed as real translation units in
CMake (`DearImGui.cpp`, `DataSources.cpp`, `DataSource.cpp`, `tinyosc.c`, and on macOS `MacGL.mm`);
the rest are pulled in via includes. Match this pattern.

Language standard: **C++17** (`std::filesystem` is used; do not reintroduce `std::experimental`).

## Build system

CMake (min 3.7). On first configure it **auto-downloads** all dependencies into the build dir via a
`download_and_extract` helper: DPF, DPF-Widgets, pugl, tinyosc, nlohmann/json, and (per-OS) FFmpeg /
GLEW / dirent. So you generally don't install those by hand. An internet connection is required for
the first configure.

Build directories (`build`, `build-*`) are gitignored. Use a fresh dir per config, e.g. `build-bundled`.

### Targets
- `WAIVE-FRONT-STANDALONE` — the standalone app.
- `WAIVE-FRONT-V2-vst3` / `-vst2` / `-au` (macOS) / `-jack` — plugin formats.

### Key options
- `-DBUNDLE_FFMPEG=ON` — self-contained build that bundles FFmpeg so end users need nothing
  installed. **Off by default** (open-source build links system/Homebrew FFmpeg on macOS).
- `-DMACOS_CODESIGN_IDENTITY=<hash>` — macOS only; when set, the standalone `.app` is code-signed
  during the build. Empty = skip signing (still builds the `.app`).
- `-DBUNDLE_DATA_PATH=<path>` — bundle a WAIVE data folder into the standalone so it ships with its
  media. macOS → copied into `Contents/Resources/WAIVE` (before signing, so it's sealed);
  Windows → copied to `WAIVE/` next to the exe. At runtime the app uses the bundled data if present
  and valid, else falls back to `~/Documents/WAIVE` (`%USERPROFILE%\Documents\WAIVE`). The folder can
  be large (the real data set is ~1.6 GB), which makes the bundle and notarization upload large.

## Building — macOS

Default (links system FFmpeg; needs `brew install ffmpeg`):
```bash
cmake -B build -S .
cmake --build build --target WAIVE-FRONT-STANDALONE -j$(sysctl -n hw.ncpu)
```

Self-contained + data-bundled + signed (full shipping build):
```bash
cmake -B build-bundled -S . -DBUNDLE_FFMPEG=ON \
  -DBUNDLE_DATA_PATH="$HOME/Documents/WAIVE" \
  -DMACOS_CODESIGN_IDENTITY=8C3E9AB199FC9F68039D3931BE49D66E0F362C1B
cmake --build build-bundled --target WAIVE-FRONT-STANDALONE -j10
```
(Omit `BUNDLE_DATA_PATH`/`MACOS_CODESIGN_IDENTITY` for a fast dev build — copying + deep-signing the
~1.6 GB data takes a few minutes each build.)

- On macOS the standalone is always built as a real **`.app` bundle** (`MACOSX_BUNDLE`). A post-build
  step (`cmake/PackageMacOSApp.cmake`) installs the icon (`assets/Icon.icns`), the correct
  `assets/Info-Standalone.plist`, and (if an identity is set) code-signs with
  `assets/entitlements.plist` + hardened runtime — logging each step as `-- [WAIVE-FRONT] ...`.
- Output: `build-bundled/WAIVE-FRONT-STANDALONE.app`. Run the inner binary at
  `…/Contents/MacOS/WAIVE-FRONT-STANDALONE`, or `open` the `.app`.
- The signing identity hash above is this project's *Developer ID Application: Superposition
  (GZ5M47VRK6)*. Find available identities with `security find-identity -p basic -v`.

## Building — Windows

Prereqs: **Git for Windows** + **Visual Studio 2022** with the *Desktop development with C++*
workload (provides MSVC + CMake + Ninja). FFmpeg/GLEW/dirent are auto-downloaded (no manual install).

From a **Developer PowerShell for VS 2022** (so `cmake`/MSVC are on PATH):
```powershell
git checkout standalone
cmake -B build -S . -DBUNDLE_FFMPEG=ON
cmake --build build --config Release --target WAIVE-FRONT-STANDALONE
```
- The VS generator is multi-config → `--config Release` is required.
- Output: `build\Release\WAIVE-FRONT-STANDALONE.exe`. With `BUNDLE_FFMPEG`, the FFmpeg DLLs are
  copied next to the exe by a post-build step. Ship the exe **together with those DLLs**.
- No `.app`/codesigning on Windows — just the exe + DLLs.
- The compile has not been verified on Windows; if MSVC errors appear, the likely spots are the
  `.cpp`-include translation units, WinSock (`Ws2_32`, `_WINSOCKAPI_`), or the dirent shim.

## FFmpeg bundling details

- **macOS, `BUNDLE_FFMPEG=ON`**: builds a **minimal, decode-only FFmpeg 7.1 from source**, linked
  **statically** (`cmake/BundleFFmpeg.cmake`). Components: `mov` demux; h264/hevc/mpeg4/mpeg2/mjpeg/
  vp8/vp9/prores decoders; swscale; `buffer`/`palettegen`/`buffersink` filters. No encoders →
  LGPL-clean (no GPL x264/x265), no external deps. The binary is fully self-contained (nothing to
  bundle, no rpath surgery). First build compiles FFmpeg (a few minutes; cached after).
- **Windows, `BUNDLE_FFMPEG=ON`**: downloads the prebuilt **LGPL shared** FFmpeg (BtbN) and copies
  the DLLs next to the exe. (A from-source minimal Windows build would need MSYS2 — not set up.)
- **Off (default)**: macOS links system/Homebrew FFmpeg; Windows uses the prebuilt **GPL shared** build.

## Automatic ("demo") mode

In `WaiveFrontController.hpp`. Self-running mode (a UI toggle, **on by default**) that drives the
visuals and structural choices. Five driven parameters (`AutoParam`): Blur, Focus, Space, Zoom,
Background. Each has its own **driver** — `DRIVER_LFO` (sine), `DRIVER_PULSE` (rhythmic envelope),
`DRIVER_NOISE` (smoothed random) — and switches driver randomly over time. Each parameter has its
own **Speed** (clock scale) and **Chaos** (amplitude) slider, defaulting to 0.75 / 0.35. Chaos pulls
focus/space toward the range centre, and blur/zoom/background toward 0. It also randomly changes
categories/items and toggles layers on a steady clock. Seeded by `beginAutomatic()` (called in
`init()` since it starts enabled).

## Native helpers (`src/util/MacGL.h` / `MacGL.mm`)

`MacGL.h` has three implementations: a real one for **macOS** (`MacGL.mm`), inline **Windows** stubs
(`#elif defined(_WIN32)`), and no-op stubs for anything else. The same function names are used cross-
platform so the shared code doesn't need `#ifdef`s. Functions:
- `waiveUpdateGLDrawable()` — macOS only does work: on window resize, force the `NSOpenGLView` to
  fill the wrapper and call `[NSOpenGLContext update]`; otherwise the GL drawable stays frozen at its
  initial size (cropped render) because the standalone drives rendering manually (one window repaints
  another), bypassing AppKit's display pass. Called from both windows' `onReshape`.
- `waiveSetCursorHidden()` — `[NSCursor hide]`/`unhide` on macOS, `ShowCursor` on Windows (balanced).
  The viewer hides the cursor on motion over it; the control window shows it again on motion over itself.
- `waiveToggleFullscreen()` — borderless full-screen toggle (not native macOS fullscreen, to avoid a
  title bar under the hidden cursor); Windows uses a borderless `SetWindowPos` equivalent.
- `waiveGetBundledDataPath()` — returns the bundled WAIVE data folder: `Contents/Resources/WAIVE`
  (macOS, via `NSBundle`) or `<exe dir>/WAIVE` (Windows).

Closing either standalone window quits the app via an app-level idle watcher (`QuitOnWindowClose`) —
note **`Window::onClose()` is never dispatched on macOS** in DGL, so don't rely on it.

## Keyboard shortcuts (standalone)

- **F** — toggle fullscreen on the **Viewer** window (macOS + Windows).
- **Cmd/Super+Q** — quit the app, from either the Viewer or the control window.

## Shipping (macOS notarization)

See README "Shipping on MacOS". The standalone `.app` is packaged/signed by the build itself — do
**not** run the README's manual icon/plist/sign steps on it, and never copy `assets/Info.plist` (the
*plugin's* plist; its `CFBundleExecutable` is `WAIVE-FRONT-V2`) into the standalone bundle — that
breaks the signature and fails notarization with "invalid Info.plist". The standalone uses
`assets/Info-Standalone.plist`. To archive for notarization use `ditto -c -k --keepParent` (not `zip`),
then `xcrun notarytool submit … --team-id GZ5M47VRK6 --wait`.
