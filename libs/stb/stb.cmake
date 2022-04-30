include_directories(libs/stb)
add_library(stb STATIC "${CMAKE_SOURCE_DIR}/libs/stb/stb.c")
target_compile_options(stb PRIVATE
	-Wno-sign-compare
	-Wno-unused-value
	-Wno-unused-variable
	-Wno-unused-function
)
