# Building OBS Clock 1.1.0

## Native GitHub builds

Open this repository's Actions tab, select **Build All Platforms**, and choose
**Run workflow** on main. Infrastructure changes also trigger a validation run.
The four jobs build one source commit and run tests before packaging:

| Target | Runner | Package |
| --- | --- | --- |
| Windows x64 | Windows 2025 with Visual Studio | Setup EXE and manual ZIP |
| macOS Apple silicon | macOS 26 arm64 | Per-user PKG and manual bundle archive |
| macOS Intel | macOS 26 Intel | Per-user PKG and manual bundle archive |
| Linux x64 | Ubuntu 26.04 | Native DEB |

The final combined artifact requires every native job to succeed and records the
same revision in all four BUILD-INFO files. A compilation or automated test pass
is separate from loading and visually testing the plugin in OBS.

## Dependencies and SDK preparation

`ci/dependencies.json` pins official OBS 33.0.1 source and host-library packages
by SHA-256. The build uses their development headers and host libraries, without
rebuilding OBS or including OBS binaries in the plugin installer.

- Windows retains GDI text rendering and native Windows time-zone enumeration.
  Visual Studio C++ tools, the Windows SDK, CMake, Python and Inno Setup 6 are needed.
- macOS and Linux use Qt Core/Gui for text and time zones. macOS uses OBS's matching
  Qt and prebuilt dependency archives, including SIMDe headers. Linux uses the
  Ubuntu development packages listed in the workflow, including libsimde-dev.
- macOS test bundles are ad-hoc signed. Public Developer ID signing and notarization
  require separately configured certificates and credentials.
- Linux packages target native Ubuntu 26.04 OBS 33. Flatpak is a separate extension
  package, not the native DEB. See CLOUD_BUILDS.md for current packaging scope.

Run `python ci/native_build.py --platform windows`, `macos-arm64`, `macos-x86_64`,
or `linux` on the corresponding native operating system. The script downloads
verified dependencies, builds, tests, stages and packages. On macOS it also checks
architecture/signatures and exercises the per-user PKG installation on the runner.

## Local CMake builds

For a complete installed OBS 33 SDK, use its libobs CMake package through
CMAKE_PREFIX_PATH. Alternatively configure explicit paths:

```text
cmake -S . -B build -DOBS_INCLUDE_DIR=/path/to/obs/libobs -DOBS_GENERATED_INCLUDE_DIR=/path/to/generated -DOBS_IMPORT_LIBRARY=/path/to/host/library
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix staging
```

OBS_IMPORT_LIBRARY is an MSVC-compatible .lib on Windows, the OBS framework
binary on macOS, or libobs.so on Linux. OBS_GENERATED_INCLUDE_DIR holds the
configured obsconfig.h. OBS_EXTRA_INCLUDE_DIR can point to extra dependency
headers such as SIMDe. Match all development inputs to the OBS host version and
architecture. Do not ship the local OBS/Qt SDK directories inside the plugin.

Windows developers can use `tools/build_windows.ps1` with the same explicit SDK
paths. The separate `scripts/build-windows.sh` records the MinGW route. Neither
helper publishes a release or installs into the user's OBS directory automatically.

## Artwork and documentation

CMake generates src/embedded_assets.hpp from tracked files under data using
scripts/embed_assets.py. The generated header is ignored by Git. Do not edit its
byte arrays or regenerate the design artwork for a small code change.

Edit USER.md and run tools/build_user_guide.py to rebuild the Word guide. Render
and inspect the resulting document before including it in a release. PDF export
is a separate documentation step.

## Portable tests

A standalone C++17 compiler can run the time suite without OBS or Qt:

```sh
g++ -std=c++17 -Wall -Wextra tests/time_test.cpp -o clock-time-tests
./clock-time-tests
```

The non-Windows platform suite additionally tests Qt time-zone conversion,
daylight saving, text coverage, transparency and straight-alpha color handling.
