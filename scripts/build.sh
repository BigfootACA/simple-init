#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
touch root
cmake \
	-B build -S . \
	-G Ninja \
	"${@}"
ninja -C build -j "$(nproc)"
popd >/dev/null
