include_directories(libs/stb)
add_library(stb STATIC libs/stb/stb.c)
target_compile_options(stb PRIVATE
	-Wno-sign-compare
	-Wno-unused-value
	-Wno-unused-variable
	-Wno-unused-function
)
if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(stb PRIVATE -Wno-self-assign)
endif()
