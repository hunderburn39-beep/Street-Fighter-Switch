#!/bin/sh
# Compiles switch_main.cpp (with __SWITCH__ and the GLES3 path) against stand-ins for the
# console-only headers (libnx), so interface/type errors surface in CI in seconds instead of
# after the multi-minute ARM64 build. Needs the EGL and GLES3 headers (libegl-dev libgles-dev).
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp "$root"/*.hpp "$root"/*.h "$root"/switch_main.cpp "$work"/
cp "$here"/switch_stubs/switch_*.hpp "$work"/        # shadow the libnx-backed headers
g++ -std=c++17 -Wall -Wextra -Werror -D__SWITCH__ -fsyntax-only -I"$here/switch_stubs" "$work/switch_main.cpp"
echo "switch_main.cpp: syntax and types OK"
