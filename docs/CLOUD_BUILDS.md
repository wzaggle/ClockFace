# Native cloud builds for OBS Clock

The workflow follows the SimpleCamPTZ pattern: one committed source revision,
native compiler jobs, tests, installer packaging, and a final revision check.
Open Actions, select Build All Platforms, then Run workflow. Infrastructure
pushes also start a validation run. Download each successful packages artifact,
or the combined OBS-Clock-AllPlatforms artifact after all four jobs pass.
Artifacts are retained for seven days. No public release is created.

## Platform details

Windows uses the pinned official OBS 33.0.1 headers and a generated import
library for the official host DLL. It keeps the existing Windows/GDI backend.
Inno Setup supports standard ProgramData and portable OBS destinations.

Both macOS architectures use pinned OBS 33.0.1 host libraries and the matching
OBS Qt dependency archive. Packages install into the current user's Library.
The workflow verifies bundle architecture, ad-hoc signing, and actual PKG
installation. Packages are not Developer ID signed or notarized.

Linux targets Ubuntu 26.04, because OBS 33's official Linux release dropped
Ubuntu 24.04. It uses the official 33.0.1 package library and distribution Qt.
The DEB is for native OBS, not Flatpak or Snap. Flatpak still needs a separate
extension manifest and runtime build/test pass; its user workflow is described
in the guide for eventual release packaging.

## Validation limits

The native cloud builds and tests have passed on all four targets. Mac PKG
installation is checked on each native runner.
CI compilation and packaging do not establish OBS rendering/load behavior.
After downloading, exercise the full acceptance checklist on each platform.
Keep the accepted 1.0.4 Windows package until the new candidate is accepted.

Source references:
https://github.com/wzaggle/SimpleCamDirect/blob/main/docs/CLOUD_BUILDS.md
https://github.com/obsproject/obs-studio/releases/tag/33.0.1

OBS 33 native Linux packages install under /usr/lib/x86_64-linux-gnu/obs-modules/plugins, with a matching data directory under /usr/share/obs/obs-modules/plugins. User manual installations use XDG_DATA_HOME/obs-studio/plugins/OBS-Clock/OBS-Clock.so (default ~/.local/share). This follows frontend/plugin-manager/PluginModuleLoader_Linux.cpp in the pinned OBS 33 source.

Final matching installer build: https://github.com/wzaggle/OBS-Clock/actions/runs/41390335998

Source: 48a9b234b549e12d3a02d0ac704cf380634e0c94
