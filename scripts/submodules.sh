#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
ME="$(realpath "$0")"
git submodule init
git submodule sync --recursive
git submodule update --recursive --depth=1
for i in $(git submodule status|awk '{print $2}')
do	[ -f "${i}/.gitmodules" ]||continue
	pushd "${i}" >/dev/null
	bash "$ME"
	popd >/dev/null
done
popd >/dev/null