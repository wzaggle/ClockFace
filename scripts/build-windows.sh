#!/usr/bin/env bash
set -euo pipefail
# Requires MinGW-w64, gendef, official OBS source and Windows x64 OBS distribution.
: "${OBS_SOURCE_DIR:?Set official OBS source directory}"
: "${OBS_DLL:?Set path to official Windows x64 obs.dll}"
: "${OBS_CONFIG_DIR:?Set directory containing configured obsconfig.h}"
root=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$root/build-windows"
cd "$root/build-windows"
gendef "$OBS_DLL"
x86_64-w64-mingw32-dlltool -d obs.def -l libobs.a
x86_64-w64-mingw32-windres -I"$root/resources" "$root/resources/version.rc" -o version.o
x86_64-w64-mingw32-g++ -std=c++17 -shared -O2 -static -static-libgcc -static-libstdc++ \
 -I"$OBS_SOURCE_DIR/libobs" -I"$OBS_CONFIG_DIR" "$root/src/clock.cpp" libobs.a version.o -lgdi32 -luser32 -o ClockFace.dll
