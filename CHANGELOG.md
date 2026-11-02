# Changelog

## 1.1.1 ClockFace candidate

- Rename the plugin, source display name, binaries, bundles, installers and repository to ClockFace.
- Change the internal source ID to `clockface_source`; recreate test sources after installation.
- Rebuild Windows x64, macOS ARM64, macOS Intel and native Ubuntu 26.04 packages.
- Refresh the Word guide and all current installation instructions; clarify Flatpak is not supplied.
- Windows setup removes the named sibling OBS-Clock test DLL. The Linux package replaces obs-clock.


## 1.1.0 candidate

- Added macOS/Linux Qt time-zone and text rendering backend.
- Preserved the Windows GDI backend and existing source/settings IDs.
- Added four native GitHub Actions builds and platform installer packaging.
- Expanded installation documentation across Windows, macOS, Linux and Flatpak.
- Native Windows, Intel Mac, Apple silicon Mac and Ubuntu builds/tests passed.
- Generated and checked the matching installer set from source 48a9b234b549e12d3a02d0ac704cf380634e0c94.
- Live OBS acceptance remains pending.

# Change history

## 1.0.4

Moved the Opacity property before Show message and the remaining checkboxes.
Scheduling and source-routing behavior did not change.

## 1.0.3

Changed the display name to OBS Clock and the filename/folder to OBS-Clock.
Organized the two-tier message controls and embedded the new Windows clock icon.
Preserved the internal source identifier and saved setting keys.

## 1.0.2

Changed the countdown to MM:SS and added routing to an existing OBS text source.
The clock no longer reserves an inline countdown row in external mode.

## 1.0.1

Added the final-hour daily message schedule, conditional background-image picker,
and conditional color controls.

## 1.0

Named the plugin OBSClock and added independent number and tick-mark colors.

## Earlier development

Converted the supplied Lua clock to a native Windows source; adapted image loading
to OBS 33; corrected cross-runtime FILE handling; added opacity, independent
markings, new styles, custom backgrounds, digital/date labels, and Windows zones.

### Documentation revision

Expanded installation guidance to Windows portable, macOS Apple silicon and Intel, native Linux, and Flatpak. Explicitly identifies planned builds and preserves Windows-only current release status. No plugin binary changes.
