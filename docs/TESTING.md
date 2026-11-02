# Testing OBS Clock 1.0.4

## Recorded evidence

- User confirmed that the 0.1.2 Windows plugin loaded after the runtime FILE fix.
- Subsequent user feedback addressed visible controls and styling; the 1.0.4
  placement correction was accepted as perfect in the conversation.
- Windows x64 compilation passed. Source comparison for 1.0.4 showed only version
  labels and the Opacity-property position changed from 1.0.3.
- Portable tests have passed for hand angles, tick modes, 12/24-hour formats,
  dates, valid/invalid end times, one-hour/midnight countdown boundaries, MM:SS,
  and inline/external text layout heights.
- OBS imports, module exports, icon resources, and property creation order were
  inspected. Compile/import checks are not a live OBS execution test.

## Acceptance scenarios to record

1. Load/restart OBS 33 with just one installed clock DLL; check no duplicate source
   registration or new Simple Clock / OBS Clock initialization errors.
2. Try all faces and hand sets; verify center pivots, color contrast, second-hand
   movements, opacity endpoints, tick-only/numbers-only/both/neither, and Neon glow.
3. Test custom backgrounds, transparency, picker visibility, and path retention.
4. Check every color picker is hidden with Override colors off and that saved
   choices return when it is enabled again.
5. Test digital/date settings, source heights, local versus UTC versus a Windows
   zone, date changes around midnight, and independent clock sources.
6. Show message off: no inline or external message, and conditional controls hidden.
7. Show message on: valid End time, Message and routing option available. During
   the final hour, countdown uses MM:SS and clears exactly at End time. Test a
   target across midnight. A blank Message should show only the countdown.
8. External mode: choose a dedicated Text source, verify text changes each second,
   and verify font/color/transforms/filters remain unchanged. Rename the source,
   switch targets, disable the feature, and remove the publisher. Confirm cleanup
   does not erase a manual edit that no longer matches the clock's last output.
9. Verify external output updates with the clock not visible but still present in
   the scene collection. Confirm no inline message row is reserved in that mode.
10. Save/reopen the scene collection, restart OBS, and close OBS normally. Record
    daily recurrence and daylight-saving behavior separately if exercised.

The message follows wall-clock time; a repeated autumn hour may repeat its interval.
An external text source's background can remain visible when its text is blank,
because the plugin changes text content rather than source visibility.

## Report a result

Record OBS version, Windows architecture, plugin version/SHA-256, test steps,
expected behavior, actual behavior, and relevant current OBS log lines. Keep
private logs and environment details out of public issues unless reviewed.

## 1.1.0 candidate evidence

- Linux compile check passed with OBS 33.0.1 headers and host OBS 30 linker library.
  This is not an OBS 33 runtime validation and is not a release artifact.
- Portable math suite passed.
- Qt backend suite passed: UTC, New York winter/DST, zone enumeration, text
  dimensions, visible text, transparent background, exact straight RGB and empty text.
- Windows source cross-compilation passed with the preserved GDI backend.
- Native cloud builds, installer runs, Flatpak packaging and live OBS checks are pending.

Repeat the acceptance scenarios on Windows, Intel Mac, Apple silicon Mac, and
Ubuntu26 native OBS. Record architecture and platform-specific time-zone IDs.
