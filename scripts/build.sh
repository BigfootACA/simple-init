#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
touch root
cmake \
	-B build -S . \
	-G Ninja \
	-DENABLE_GTK=OFF \
	-DENABLE_SDL2=OFF \
	-DENABLE_ASAN=OFF \
	-DENABLE_STATIC=OFF \
	"${@}"
ninja -C build -j "$(nproc)"
popd >/dev/null
