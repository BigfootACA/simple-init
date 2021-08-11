include_directories(libs/lodepng)
set(LODEPNG_SRC "${CMAKE_CURRENT_BINARY_DIR}/lodepng.c")
set(LODEPNG_DIST "${CMAKE_SOURCE_DIR}/libs/lodepng/lodepng.cpp")
add_custom_command(
	OUTPUT "${LODEPNG_SRC}"
	COMMAND ln -s "${LODEPNG_DIST}" "${LODEPNG_SRC}"
	DEPENDS "${LODEPNG_DIST}"
)
add_library(lodepng STATIC "${LODEPNG_SRC}")
