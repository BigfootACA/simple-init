include_directories(
	libs/libtsm/src/tsm
	libs/libtsm/src/shared
	libs/libtsm/external
)
add_library(libtsm STATIC
	libs/libtsm/src/shared/shl-htable.c
	libs/libtsm/src/shared/shl-pty.c
	libs/libtsm/src/shared/shl-ring.c
	libs/libtsm/src/tsm/tsm-vte.c
	libs/libtsm/src/tsm/tsm-render.c
	libs/libtsm/src/tsm/tsm-vte-charsets.c
	libs/libtsm/src/tsm/tsm-selection.c
	libs/libtsm/src/tsm/tsm-unicode.c
	libs/libtsm/src/tsm/tsm-screen.c
	libs/libtsm/external/wcwidth/wcwidth.c
)
target_compile_definitions(libtsm PRIVATE _GNU_SOURCE)
target_compile_options(libtsm PRIVATE -Wno-sign-compare)
if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(libtsm PRIVATE -Wno-old-style-declaration)
endif()
