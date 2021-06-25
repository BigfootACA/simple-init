#!/bin/bash
set -e
pushd "$(dirname "$0")/.." >/dev/null
source scripts/functions.sh.inc
source scripts/environments.sh.inc
if ! hash lv_font_conv 2>/dev/null
then	npm install lv_font_conv||true
	export PATH="${PWD}/node_modules/.bin:${PATH}"
fi
hash lv_font_conv
for i in "${FONTSIZE[@]}"
do	echo "convert ${i}"
	lv_font_conv \
		--bpp 4 \
		--size "${i}" \
		--font "${FONTPATH}" \
		-r 0x20-0x7f \
		--symbols "$(<fonts/chars.txt)" \
		--format lvgl \
		-o "fonts/lv_font_cjk_${i}.c" \
		--force-fast-kern-format
done
popd >/dev/null
