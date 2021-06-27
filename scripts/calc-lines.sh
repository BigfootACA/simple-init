#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
FOLDERS=(
	src
	include
	po
	scripts
)
find "${FOLDERS[@]}" -type f -print0 |\
	wc --files0-from=- --lines |\
	sort --numeric-sort |\
	tail -n 23
