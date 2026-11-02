# Building OBS Clock

## Current status

The supplied 1.0.4 Windows x64 DLL was cross-compiled with MinGW-w64 GCC 13 against
OBS Studio 33.0.1 headers. The project's C++ source and resource content match
that build. The repository CMake file and PowerShell helper have been cleaned up
for a Windows development checkout; native Visual Studio compilation has not
been executed here. Treat the MSVC steps below as setup instructions to validate.

## Dependencies

- A C++17 toolchain targeting Windows x64: Visual Studio C++ tools or MinGW-w64.
- CMake 3.22 or later for the project-based build.
- OBS 33 development headers, including the configured `obsconfig.h`.
- The matching Windows OBS import library. Use an MSVC-compatible `.lib` with
  Visual Studio, or a MinGW-compatible `.a` with MinGW. They are not interchangeable
  setup inputs to assume without checking the chosen toolchain.
- Windows SDK resources and GDI/User libraries, supplied by the Windows toolchain.

OBS's ordinary application installation is not a complete development SDK.
Obtain/build the development files from the official OBS Project sources and
instructions. This package does not include an OBS SDK or vendor binaries.

Official source reference: https://github.com/obsproject/obs-studio/tree/33.0.1

No separate Qt dependency is used by this plugin. GDI text rendering and Windows
zone enumeration are currently platform-specific adapters in `src/windows_clock.hpp`.

## Visual Studio project generation

In a PowerShell terminal with Visual Studio C++ tools and CMake available:

```powershell
./tools/build_windows.ps1 `
  -ObsIncludeDir "C:/obs-sdk/include/obs" `
  -ObsImportLibrary "C:/obs-sdk/lib/obs.lib" `
  -ObsGeneratedIncludeDir "C:/obs-sdk/generated"
```

Replace these example paths with actual SDK paths. If `obsconfig.h` is already
alongside the other OBS headers, omit `ObsGeneratedIncludeDir`. Alternatively use
an SDK's `libobs` CMake package via `CMAKE_PREFIX_PATH` and configure CMake directly.

The helper configures an x64 Visual Studio build, compiles Release, runs the time
unit tests, and stages `staging/OBS-Clock/OBS-Clock.dll`. It does not install into OBS
or publish a release. Close OBS before manually replacing an installed DLL.

## MinGW cross-build

`scripts/build-windows.sh` records the direct MinGW route. It requires:

- `OBS_SOURCE_DIR`: official OBS 33 source checkout.
- `OBS_CONFIG_DIR`: directory with configured `obsconfig.h`.
- `OBS_DLL`: official matching Windows x64 `obs.dll` for generating the import library.
- `gendef`, MinGW-w64 `dlltool`, `windres`, and C++ compiler on PATH.

Run from the project root:

```bash
bash scripts/build-windows.sh
```

The helper writes `build-windows/OBS-Clock.dll`. Dependencies should be OBS itself
and normal Windows system libraries; inspect imports before distribution.

## Embedded assets

The generated `src/embedded_assets.hpp` is already included. To rebuild it after
changing `data/` files, use Python 3:

```bash
python scripts/embed_assets.py
```

Regenerating the design artwork is separate from compiling the plugin. Those
scripts require Pillow, NumPy/SciPy for glyph extraction, and the exact font paths
shown in the scripts. Preserve the original assets unless a change is intended.

## Portable math checks

On a machine with a host C++17 compiler:

```bash
g++ -std=c++17 -Wall -Wextra tests/time_test.cpp -o clock-time-tests
./clock-time-tests
```

These tests cover time/countdown logic and formatting, not OBS runtime loading,
Windows time-zone conversion, or rendering. See TESTING.md for acceptance cases.

## 1.1.0 cross-platform candidate

The earlier Windows 1.0.4 notes above describe the accepted baseline. This
candidate adds Qt Core/Gui for macOS and Linux; Windows retains GDI and does not
add a Qt dependency. Use ci/native_build.py on each native OS or the supplied
Build All Platforms workflow. Dependencies and checksums are pinned in
ci/dependencies.json. The script uses official OBS 33 host libraries with the
matching source headers; it does not rebuild or package OBS itself.

Explicit SDK CMake options work on all platforms: OBS_INCLUDE_DIR,
OBS_GENERATED_INCLUDE_DIR, and OBS_IMPORT_LIBRARY (the platform library path).
For a complete installed SDK, libobs 33's CMake package can be used instead.

The local Qt 6.4 test is a compatibility check for our adapter only. Production
cloud macOS builds use OBS's matching Qt; Ubuntu26 uses its own Qt. Never package
local Qt libraries alongside OBS or assume old OBS libraries are runtime tests.
